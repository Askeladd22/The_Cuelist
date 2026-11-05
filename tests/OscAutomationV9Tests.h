#pragma once
#include "OscEditor.h"
#include "EditorToolbarLayout.h"
#include <cstring>

void oscAutomationV9Tests() {
    test("OSC right-drag marquee selects multiple messages instead of showing popup",[] {
        OscClip clip;clip.length=4;
        clip.events={{.5,"/a",{}},{1.0,"/b",{}}};
        OscClipEditor editor(clip,{{0,120,4,4}},
            [](const OscClip&,const OscClip&,bool){});
        editor.setSize(940,340);
        const auto stamp=juce::Time::getCurrentTime();
        const juce::Point<float> start{160.f,103.f},finish{310.f,144.f};
        const auto right=[&](juce::Point<float> at,bool dragged=false) {
            return juce::MouseEvent(
                juce::Desktop::getInstance().getMainMouseSource(),at,
                juce::ModifierKeys(juce::ModifierKeys::rightButtonModifier),
                1,0,0,0,0,&editor,&editor,stamp,start,stamp,1,dragged);
        };
        editor.mouseDown(right(start));
        editor.mouseDrag(right(finish,true));
        editor.mouseUp(right(finish,true));
        check(editor.getClip().events.size()==2,
              "Marquee must select, never remove or trigger OSC messages");
        editor.keyPressed(juce::KeyPress(juce::KeyPress::deleteKey));
        check(editor.getClip().events.empty(),
              "Right-drag did not select both OSC messages for bulk Delete");
    });
    test("OSC double-click inserts a validated automation point without Ramp dialog",[] {
        OscClip clip;clip.length=4;
        OscTrack track;track.message.address="/brightness";
        track.message.arguments={{OscValueType::decimal,0,0.f,{}}};
        track.argumentIndex=0;track.rangeMin=0;track.rangeMax=1;
        clip.tracks.push_back(track);
        OscAutomation ramp;ramp.message=track.message;ramp.argumentIndex=0;
        ramp.time=.5;ramp.duration=2.;
        ramp.points={{0.,0.},{2.,1.}};
        clip.automations.push_back(ramp);
        int changes=0;
        OscClipEditor editor(clip,{{0,120,4,4}},
            [&](const OscClip&,const OscClip&,bool done){if(done)++changes;});
        editor.setSize(940,340);
        const juce::Point<float> point{393.f,207.f};
        const auto click=cueViewFixtures::event(editor,point,point,2);
        editor.mouseDoubleClick(click);
        check(editor.getClip().automations[0].points.size()==3 &&
              validateOscClip(editor.getClip()).wasOk(),
              "Double-click did not insert a valid interior OSC control point");
        near(editor.getClip().automations[0].points[1].time,1.0);
        check(changes==1,"OSC double-click should create one Undo transaction");
        editor.mouseDoubleClick(click);
        check(editor.getClip().automations[0].points.size()==3,
              "Double-click on an existing point duplicated the point");
    });
    test("OSC track header edit synchronizes multi-argument target transactionally",[] {
        OscClip clip;clip.length=8;
        OscTrack t;t.message.address="/light";
        t.message.arguments={{OscValueType::integer,7,0,{}},
                             {OscValueType::decimal,0,.1f,{}}};
        t.argumentIndex=1;t.rangeMin=0;t.rangeMax=1;
        clip.tracks.push_back(t);
        for(int n=0;n<2;++n) {
            OscAutomation ramp;ramp.message=t.message;ramp.argumentIndex=1;
            ramp.time=n*2.0;ramp.duration=1;
            ramp.points={{0,.25},{1,.75}};
            clip.automations.push_back(ramp);
        }
        check(validateOscClip(clip).wasOk(),"Valid multi-argument OSC track rejected");
        auto changed=t;changed.message.address="/colour";
        changed.message.arguments[0].integer=12;
        changed.rangeMin=-1;changed.rangeMax=2;
        check(updateOscTrackSettings(clip,0,changed).wasOk(),"Track configuration edit rejected");
        check(clip.tracks[0].message.address=="/colour" &&
              clip.automations[0].message.address=="/colour" &&
              clip.automations[1].message.address=="/colour" &&
              clip.automations[0].message.arguments[0].integer==12 &&
              clip.automations[0].argumentIndex==1 &&
              clip.automations[1].points[1].value==.75,
              "Track update lost target identity, constants, or existing ramp values");
        auto invalid=changed;
        invalid.message.arguments[1].type=OscValueType::integer;
        const auto before=clip;
        check(updateOscTrackSettings(clip,0,invalid).failed(),
              "Fractional ramps allowed to become integer targets");
        check(clip.tracks[0].message.address==before.tracks[0].message.address &&
              clip.automations[0].message.arguments[1].type==OscValueType::decimal,
              "Invalid settings partially altered OSC clip");
        invalid=changed;invalid.rangeMax=.5;
        check(updateOscTrackSettings(clip,0,invalid).failed(),
              "Track accepted a range excluding an existing control point");
        check(validateOscClip(clip).wasOk(),"Failed configuration left invalid OSC data");
    });
    test("OSC Ctrl-selection moves two events without changing their relative time",[] {
        OscClip clip;clip.length=4;
        clip.events={{.5,"/a",{}},{1.0,"/b",{}}};
        int commits=0;
        OscClipEditor editor(clip,{{0,120,4,4}},
            [&](const OscClip&,const OscClip&,bool done) {if(done)++commits;});
        editor.setSize(940,340);
        const juce::Point<float> a{183.f,111.f},b{288.f,128.f};
        const auto normal=cueViewFixtures::event(editor,a,a);
        editor.mouseDown(normal);editor.mouseUp(normal);
        const auto time=juce::Time::getCurrentTime();
        auto control=[&](juce::Point<float> p) {
            return juce::MouseEvent(
                juce::Desktop::getInstance().getMainMouseSource(),p,
                juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier|
                                   juce::ModifierKeys::ctrlModifier),
                1,0,0,0,0,&editor,&editor,time,p,time,1,false);
        };
        editor.mouseDown(control(b));editor.mouseUp(control(b));
        const juce::Point<float> moved{236.f,111.f};
        editor.mouseDown(cueViewFixtures::event(editor,a,a));
        editor.mouseDrag(cueViewFixtures::event(editor,moved,a,1,true));
        editor.mouseUp(cueViewFixtures::event(editor,moved,a,1,true));
        const auto& result=editor.getClip();
        // Snap is enabled by default at 1/8 grid; both selected events
        // must shift together by the snapped delta, not the raw mouse delta.
        check(result.events[0].time>.5 &&
              result.events[1].time>1.0,
              "Multi-selection drag did not move both OSC events");
        near(result.events[1].time-result.events[0].time,.5);
        check(commits==1,"Grouped OSC gesture must commit one Undo transaction");
        check(validateOscClip(result).wasOk(),"Grouped OSC drag left an invalid message");
    });
    test("N7 OSC editor renders flat event and automation lanes without altering data",[] {
        OscClip source;
        source.length=4;
        source.events.push_back({.25,"/cue",{{OscValueType::integer,1,0,{}}}});
        OscAutomation ramp;
        ramp.message.address="/brightness";
        ramp.message.arguments={{OscValueType::decimal,0,0,{}}};
        ramp.time=1;ramp.duration=1;
        ramp.points={{0,0},{1,1}};
        source.automations.push_back(ramp);
        OscClipEditor editor(source,{{0,120,4,4}},
            [](const OscClip&,const OscClip&,bool){});
        for (const auto size:std::initializer_list<juce::Point<int>>{
                 {1100,320},{940,250},{710,250}}) {
            editor.setSize(size.x,size.y);
            const auto img=editor.createComponentSnapshot(editor.getLocalBounds());
            check(img.isValid() && img.getWidth()==size.x &&
                  img.getHeight()==size.y,
                  "N7 OSC editor failed to render native timeline");
        }
        const auto& actual=editor.getClip();
        check(actual.events.size()==1 && actual.automations.size()==1 &&
              actual.events[0].address=="/cue" &&
              actual.automations[0].time==1 &&
              actual.automations[0].duration==1 &&
              actual.automations[0].points.size()==2 &&
              validateOscClip(actual).wasOk(),
              "N7 visual OSC update mutated events or automation curves");
    });
    test("N7.2 OSC toolbar shares native geometry and preserves all commands",[] {
        using namespace cuelist;
        OscClipEditor editor(OscClip{},{{0,120,4,4}},
            [](const OscClip&,const OscClip&,bool){});
        const auto find=[&](const char* name)->juce::Component* {
            for(auto* child:editor.getChildren())
                if(child->getName()==name) return child;
            return nullptr;
        };
        const std::array<const char*,13> names{
            "oscAddAutomation","oscRemoveMessage","oscDuplicate",
            "oscGridVisible","oscSnapEnabled","oscGridResolution",
            "oscZoomOut","oscZoomIn","oscZoomFit",
            "oscTimeUnit","oscDetach","oscEditorUndo","oscEditorRedo"};
        for(int width:{710,820,940,949,950,1180}) {
            editor.setSize(width,280);
            const auto t=OscEditorToolbarLayout::atWidth(width);
            const std::array<juce::Rectangle<int>,13> bounds{
                t.add,t.remove,t.duplicate,t.grid,t.snap,t.resolution,
                t.zoomOut,t.zoomIn,t.zoomFit,t.unit,t.detach,t.undo,t.redo};
            for(size_t i=0;i<names.size();++i) {
                auto* c=find(names[i]);
                check(c && c->isVisible() &&
                      c->getBounds()==bounds[i] &&
                      c->getX()>=0 && c->getRight()<=width,
                      "N7.2 OSC toolbar moved, hid or clipped a working action");
            }
            for(size_t i=0;i<bounds.size();++i)
                for(size_t j=i+1;j<bounds.size();++j)
                    check(!bounds[i].intersects(bounds[j]),
                          "N7.2 OSC compact toolbar controls overlap");
            check(t.compact==(width<950),
                  "N7.2 OSC toolbar breakpoint drifted from dock profile");
            const auto midi=MidiEditorToolbarLayout::atWidth(width);
            check(t.collapse==midi.collapse &&
                  t.detach==midi.detach && t.close==midi.close &&
                  t.title.getY()==midi.title.getY() &&
                  t.undo.getHeight()==midi.undo.getHeight() &&
                  t.grid.getY()==midi.snap.getY() &&
                  t.add.getY()==midi.snap.getY() &&
                  t.zoomFit.getY()==midi.zoomIn.getY(),
                  "OSC/MIDI dock actions no longer share the toolbar hierarchy");
            check(!t.collapse.intersects(t.undo) &&
                  !t.close.intersects(t.redo) &&
                  !t.collapse.intersects(t.detach) &&
                  !t.detach.intersects(t.close),
                  "OSC history or window actions overlap");
        }
        auto* detach=dynamic_cast<juce::TextButton*>(find("oscDetach"));
        check(detach!=nullptr,"Original OSC Detach action missing");
        editor.setFloating(true);
        check(detach->getButtonText()=="Reattach",
              "OSC Reattach label disappeared");
        editor.setFloating(false);
        check(detach->getButtonText()=="Detach",
              "OSC Detach label disappeared");
    });
    test("OSC dock toolbar never overlaps at desktop and minimum window widths",[] {
        MainComponent ui(false);
        addColumn(ui.show,ColumnType::osc,"OSC");
        check(ui.select(0,1),"Cannot select OSC column.");
        ui.editOscClip();
        auto* editor=controlsV3::named(ui,"sharedOscEditor");
        auto* collapse=controlsV3::named(ui,"clipEditorCollapse");
        auto* close=controlsV3::named(ui,"clipEditorClose");
        check(editor && collapse && close,"Docked OSC editor actions missing.");
        for (int width : {1180,1440,1640}) {
            ui.setSize(width,900);
            auto* zoom=controlsV3::named(*editor,"oscZoomFit");
            auto* unit=controlsV3::named(*editor,"oscTimeUnit");
            auto* detach=controlsV3::named(*editor,"oscDetach");
            auto* snap=controlsV3::named(*editor,"oscSnapEnabled");
            auto* grid=controlsV3::named(*editor,"oscGridVisible");
            check(zoom && unit && detach && snap && grid,
                  "OSC toolbar items missing.");
            const auto localCollapse=collapse->getBounds().translated(
                -editor->getX(),-editor->getY());
            const auto localClose=close->getBounds().translated(
                -editor->getX(),-editor->getY());
            for (auto* control : {zoom,unit,detach,snap,grid})
                check(!localCollapse.intersects(control->getBounds()) &&
                      !localClose.intersects(control->getBounds()),
                      "Dock buttons overlap the OSC toolbar.");
            check(!localCollapse.intersects(localClose),
                  "Collapse and Close overlap each other.");
        }
        ui.setSize(1180,900);
        auto* collapseButton=dynamic_cast<juce::Button*>(collapse);
        check(collapseButton!=nullptr,"Collapse button unavailable.");
        collapseButton->triggerClick();
        auto* timeline=cueViewFixtures::find<Timeline>(ui);
        check(timeline && collapse->getBottom()<=timeline->getY() &&
              close->getBottom()<=timeline->getY(),
              "Collapsed editor buttons cover the Timeline.");
    });
    test("OSC editor footer remains clipped and automation geometry unchanged",[] {
        OscClip clip;clip.length=4;
        OscAutomation ramp;
        ramp.message.address="/brightness";
        ramp.message.arguments={{OscValueType::decimal,0,0,{}}};
        ramp.time=1;ramp.duration=1;
        ramp.points={{0,0},{1,1}};
        clip.automations.push_back(ramp);
        OscClipEditor editor(clip,{{0,120,4,4}},
            [](const OscClip&,const OscClip&,bool){});
        editor.setSize(940,250);
        check(editor.createComponentSnapshot(editor.getLocalBounds()).isValid(),
              "Short OSC dock failed to render.");
        const auto& prior=editor.getClip().automations.front();
        const double time=prior.time,duration=prior.duration;
        const auto points=prior.points;
        editor.setSize(710,250);
        check(editor.createComponentSnapshot(editor.getLocalBounds()).isValid(),
              "Compact OSC toolbar failed to render.");
        const auto& after=editor.getClip().automations.front();
        check(after.time==time && after.duration==duration &&
              after.points.size()==points.size() &&
              after.points.front().value==points.front().value &&
              after.points.back().value==points.back().value &&
              validateOscClip(editor.getClip()).wasOk(),
              "OSC visual resize should not change automation data.");
    });
    test("OSC 1.0 boolean T and F encode without payload bytes",[] {
        OscEvent e; e.address="/go";
        e.arguments={{OscValueType::boolean,0,0,{},true},
                     {OscValueType::boolean,0,0,{},false}};
        OscClip clip; clip.events={e};
        check(validateOscClip(clip).wasOk(),"OSC booleans rejected");
        const auto encoded=encodeOscMessage(e);
        check(encoded.size()==8,"OSC T/F carried an illegal binary payload");
        check(std::memcmp(encoded.data()+4,",TF",3)==0,"OSC boolean type tags are wrong");
    });
    test("v9 persists detailed curves, booleans and leaves v8 projects unchanged",[] {
        auto show=makeShow(); addColumn(show,ColumnType::osc,"Lighting");
        OscClip osc; osc.useBeats=true; osc.length=12;
        osc.events.push_back({1,"/enabled",{{OscValueType::boolean,0,0,{},true}}});
        OscAutomation a; a.message.address="/dimmer";
        a.message.arguments={{OscValueType::integer,2,0,{}},
                             {OscValueType::decimal,0,0.25f,{}}};
        a.argumentIndex=1; a.time=2; a.duration=5;
        a.points={{0,0.25},{1.5,0.4},{3.5,0.7},{5,1}};
        osc.automations={a}; show.rows[0].cells[1].osc=osc;
        const auto encoded=encodeShow(show);
        near((int)juce::JSON::parse(encoded)["version"],10);
        Show restored; check(decodeShow(encoded,restored).wasOk(),"v9 OSC decode failed");
        const auto& actual=*restored.rows[0].cells[1].osc;
        check(actual.useBeats && actual.automations.size()==1 && actual.events.size()==1,
              "v9 OSC content missing");
        check(actual.events[0].arguments[0].boolean,"Boolean lost in v9 roundtrip");
        near(actual.automations[0].points[2].time,3.5);
        near(actual.automations[0].points[2].value,.7);
        check(actual.automations[0].argumentIndex==1,"Automation index lost");
        auto legacy=juce::JSON::parse(encoded);
        legacy.getDynamicObject()->setProperty("version",8);
        for (auto& row:*legacy.getProperty("rows",{}).getArray())
            for (auto& cell:*row.getProperty("cells",{}).getArray()) {
                auto* o=cell.getDynamicObject();
                if (!o->getProperty("osc").isObject()) continue;
                auto oscObj=o->getProperty("osc");
                oscObj.getDynamicObject()->removeProperty("automations");
                for (auto& ev:*oscObj.getProperty("events",{}).getArray())
                    for (auto& arg:*ev.getProperty("args",{}).getArray())
                        arg.getDynamicObject()->removeProperty("boolean");
            }
        Show earlier;
        check(decodeShow(juce::JSON::toString(legacy),earlier).wasOk(),"v8 OSC show cannot be opened");
        check(earlier.rows[0].cells[1].osc->automations.empty(),"v8 unexpectedly grew curves");
    });
    test("bad automation geometry and targets are rejected with no partial show mutation",[] {
        OscClip clip; clip.length=5;
        OscAutomation a; a.time=1; a.duration=2;
        a.message.address="/gain";
        a.message.arguments={{OscValueType::decimal,0,0,{}}};
        a.points={{0,0},{1,.5},{2,1}};
        clip.automations.push_back(a);
        check(validateOscClip(clip).wasOk(),"Valid curve rejected");
        auto broken=clip; broken.automations[0].points[1].time=2.2;
        check(validateOscClip(broken).failed(),"Out-of-order points accepted");
        broken=clip; broken.automations[0].argumentIndex=9;
        check(validateOscClip(broken).failed(),"Out-of-range arg index accepted");
        broken=clip; broken.automations[0].message.arguments[0].type=OscValueType::text;
        check(validateOscClip(broken).failed(),"String automated illegally");
        broken=clip; broken.automations.push_back(a);
        check(validateOscClip(broken).failed(),"Two curves writing the same arg concurrently accepted");
        auto show=makeShow(); addColumn(show,ColumnType::osc,"OSC");
        show.rows[0].cells[1].osc=clip;
        auto invalid=juce::JSON::parse(encodeShow(show));
        auto rows=invalid.getProperty("rows",{}).getArray();
        auto cell=(*(*rows)[0].getProperty("cells",{}).getArray())[1];
        auto osc=cell.getProperty("osc",{}).getDynamicObject();
        (*osc->getProperty("automations").getArray())[0].getDynamicObject()->setProperty("argumentIndex",999);
        Show keep; keep.name="Keep";
        check(decodeShow(juce::JSON::toString(invalid),keep).failed(),"Corrupt automation accepted");
        check(keep.name=="Keep","Invalid v9 mutated caller state");
    });
    test("automation compiler interpolates a chosen float while preserving other args",[] {
        OscClip clip; clip.length=2;
        OscAutomation a; a.message.address="/dimmer";
        a.message.arguments={{OscValueType::integer,9,0,{}},
                             {OscValueType::decimal,0,0,{}}};
        a.argumentIndex=1; a.time=0; a.duration=1;
        a.points={{0,0},{.5,1},{1,0}};
        clip.automations={a};
        const auto events=prepareOscClip(clip,[](double t){return t;},[](double t){return t;});
        check(events.size()==31,"30 Hz, including initial and final, not prepared");
        const auto& start=events.front().bytes;
        check(std::memcmp(start.data(),"/dimmer",7)==0,"Wrong automation address");
        // /dimmer padded to 8, typetag ,if padded to 4, int starts at offset 12.
        check(start.size()==20,"OSC float/int packet unexpected size");
        check(start[15]==9 && start[16]==0 && start[17]==0,"Wrong initial value bytes");
        const auto& mid=events[15].bytes;
        check(mid[15]==9 && mid[16]==0x3f && mid[17]==0x80 &&
            mid[18]==0 && mid[19]==0,"Automation did not peak at midpoint");
        const auto& end=events.back().bytes;
        check(end[16]==0 && end[17]==0 && end[18]==0 && end[19]==0,
              "Final automation value not emitted");
    });
    test("automation sends its final value at natural completion and immediately before loop wrap",[] {
        struct Sink : OscEventSink {
            std::vector<std::vector<uint8_t>> messages;
            void emit(int,const uint8_t* bytes,unsigned n,int) noexcept override {
                messages.emplace_back(bytes,bytes+n);
            }
        } sink;
        auto show=makeShow(); addColumn(show,ColumnType::osc,"OSC");
        OscClip clip; clip.length=1;
        OscAutomation a; a.message.address="/fade";
        a.message.arguments={{OscValueType::integer,0,0,{}}};
        a.duration=1; a.points={{0,0},{1,127}};
        clip.automations.push_back(a);
        show.rows[0].cells[1].osc=clip;
        auto state=makeRenderState(show,{});
        auto& voice=state->voices[0]; voice.oscSink=&sink; voice.oscRoute=5;
        voice.oscStream=std::make_shared<OscStream>();
        float samples[1000]{}; float* channels[]{samples};
        state->trigger(voice.cellId,false);
        state->render(channels,1,1000,1000);
        check(!sink.messages.empty(),"Fade failed to emit");
        check(sink.messages.back().back()==127,"Final automation value never emitted");
        const auto count=sink.messages.size();
        state->render(channels,1,100,1000);
        check(sink.messages.size()==count,"Completed clip sent trailing OSC updates");
        show.rows[0].cells[1].loop=true;
        auto looping=makeRenderState(show,{});
        auto& v=looping->voices[0]; v.oscSink=&sink; v.oscRoute=5;
        v.oscStream=std::make_shared<OscStream>();
        looping->trigger(v.cellId,false);
        const auto begin=sink.messages.size();
        looping->render(channels,1,1000,1000);
        const auto afterFirst=sink.messages.size();
        check(afterFirst>begin && sink.messages.back().back()==127,"Loop end missed final value");
        looping->render(channels,1,1,1000);
        check(sink.messages.size()==afterFirst+1 && sink.messages.back().back()==0,
              "Loop should immediately re-send the ramp's initial value");
    });
    test("two automations on different args share an OSC address without overwriting each other",[] {
        OscClip clip; clip.length=2;
        OscAutomation a; a.message.address="/xy";
        a.message.arguments={{OscValueType::decimal,0,0,{}},
                             {OscValueType::decimal,0,0,{}}};
        a.time=0; a.duration=1; a.argumentIndex=0; a.points={{0,1},{1,2}};
        auto b=a; b.argumentIndex=1; b.points={{0,10},{1,20}};
        clip.automations={a,b};
        check(validateOscClip(clip).wasOk(),"Distinct automation args wrongly conflicted");
        const auto list=prepareOscClip(clip,[](double t){return t;},[](double t){return t;});
        check(list.size()>=32,"Composite automation packets missing");
        // At t0 each automation must send an initial packet with both values.
        for (int i=0;i<2;++i) {
            const auto& bytes=list[(size_t)i].bytes;
            // /xy padded to 4, type tags ,ff padded to 4, two floats at [8..15].
            check(bytes[8]==0x3f && bytes[9]==0x80 && bytes[12]==0x41 && bytes[13]==0x20,
                  "Composite OSC packet reset the other automated argument");
        }
    });
    test("Stop does not emit reset and retrigger repeats the explicit initial value",[] {
        struct Sink:OscEventSink {
            std::vector<std::vector<uint8_t>> packets;
            void emit(int,const uint8_t* bytes,unsigned size,int) noexcept override {
                packets.emplace_back(bytes,bytes+size);
            }
        } sink;
        auto show=makeShow(); addColumn(show,ColumnType::osc,"OSC");
        OscClip clip; clip.length=2;
        OscAutomation automation; automation.message.address="/level";
        automation.message.arguments={{OscValueType::integer,0,0,{}}};
        automation.time=0; automation.duration=1; automation.points={{0,10},{1,20}};
        clip.automations={automation}; show.rows[0].cells[1].osc=clip;
        auto state=makeRenderState(show,{});
        check(state->voices.size()==1,"OSC automation voice not prepared");
        auto& v=state->voices[0]; v.oscSink=&sink; v.oscRoute=1;
        v.oscStream=std::make_shared<OscStream>();
        float out[1000]{}; float* channels[]{out};
        state->trigger(v.cellId,false);
        state->render(channels,1,1,1000);
        check(sink.packets.size()==1,"Initial OSC value was delayed");
        const auto initial=sink.packets.front();
        state->render(channels,1,500,1000);
        check(sink.packets.size()>1,"Automation updates missing");
        state->trigger(v.cellId,false,true);
        const auto stopped=sink.packets.size();
        state->render(channels,1,500,1000);
        check(sink.packets.size()==stopped,"Stop sent a reset or extra OSC messages");
        state->trigger(v.cellId,false);
        state->render(channels,1,1,1000);
        check(sink.packets.size()==stopped+1,"Retrigger must re-send initial OSC value");
        check(sink.packets.back()==initial,"Retrigger initial OSC value changed");
    });
    test("OSC timeline edits participate in application-wide Undo and Redo",[] {
        MainComponent ui(false);
        addColumn(ui.show,ColumnType::osc,"OSC");
        check(ui.select(0,1),"Cannot select OSC column");
        ui.editOscClip();
        auto* editor=ui.activeOscEditor();
        check(editor!=nullptr,"OSC editor missing");
        editor->setSize(940,360);
        const juce::Point<float> point{230,118};
        editor->mouseDown(cueViewFixtures::event(*editor,point,point));
        check(ui.show.rows[0].cells[1].osc->events.size()==1,"Mouse click did not insert event");
        check(ui.undoEdit(),"OSC event Undo unavailable");
        check(ui.show.rows[0].cells[1].osc->events.empty(),"OSC Undo did not restore old clip");
        check(ui.redoEdit(),"OSC event Redo unavailable");
        check(ui.show.rows[0].cells[1].osc->events.size()==1,"OSC Redo did not restore inserted event");
    });
    test("OSC automation respects a tempo change inside a beat-synced cue",[] {
        const std::vector<TempoPoint> tempo={{0,120,4,4},{4,60,4,4}};
        OscClip clip; clip.useBeats=true; clip.length=8;
        OscAutomation a; a.message.address="/light";
        a.message.arguments={{OscValueType::decimal,0,0,{}}};
        a.time=3; a.duration=2; a.points={{0,0},{2,1}};
        clip.automations.push_back(a);
        const auto secondsAt=[&](double beat){return secondsAtBeat(tempo,beat);};
        const auto beatAt=[&](double seconds){
            double lo=0,hi=10;
            for(int n=0;n<50;++n) {
                const auto mid=(lo+hi)*.5;
                if(secondsAt(mid)<seconds)lo=mid;else hi=mid;
            }
            return (lo+hi)*.5;
        };
        const auto messages=prepareOscClip(clip,secondsAt,beatAt);
        check(!messages.empty(),"Beat-synced automation did not compile");
        near(messages.front().seconds,1.5); // Beat three at 120 BPM
        near(messages.back().seconds,3.0);  // Beat five after change to 60 BPM
        bool foundHalf=false;
        for(const auto& msg:messages) if(std::abs(msg.seconds-2.0)<.000001) {
            check(msg.bytes.size()==16,"Unexpected OSC float payload");
            // /light padded to 8; ,f padded to 4; 0.5f = 0x3f000000.
            check(msg.bytes[12]==0x3f && msg.bytes[13]==0x00,
                  "Musical interpolation did not reach halfway at BPM change");
            foundHalf=true;
        }
        check(foundHalf,"Tempo change point missing from the prepared automation timeline");
    });
    test("seek into an OSC ramp does not repeat its already elapsed initial value",[] {
        struct Sink : OscEventSink {
            std::vector<std::vector<uint8_t>> received;
            void emit(int,const uint8_t* data,unsigned size,int) noexcept override {
                received.emplace_back(data,data+size);
            }
        } sink;
        auto show=makeShow(); addColumn(show,ColumnType::osc,"OSC");
        OscClip clip; clip.length=2;
        OscAutomation au; au.message.address="/seek";
        au.message.arguments={{OscValueType::integer,0,0,{}}};
        au.points={{0,10},{1,40}}; au.duration=1;
        clip.automations={au};
        show.rows[0].cells[1].osc=clip;
        auto state=makeRenderState(show,{});
        auto& voice=state->voices[0]; voice.oscSink=&sink; voice.oscRoute=3;
        voice.oscStream=std::make_shared<OscStream>();
        state->trigger(show.rows[0].id,true);
        state->seekRow(show.rows[0].id,.5);
        float samples[100]{}; float* channels[]{samples};
        state->render(channels,1,100,1000);
        check(!sink.received.empty(),"Seek into ramp did not resume interpolation");
        const auto& bytes=sink.received.front();
        check(bytes.size()>=4 && bytes.back()!=10,"Seek unexpectedly replayed the initial value");
        const auto before=sink.received.size();
        state->trigger(show.rows[0].id,true,true);
        state->render(channels,1,100,1000);
        check(sink.received.size()==before,"Explicit Stop emitted an OSC reset");
    });
    test("double-clicking the empty OSC lane creates a one-second 0-to-1 ramp",[] {
        OscClipEditor editor(OscClip{},{{0,120,4,4}},
            [](const OscClip&,const OscClip&,bool){});
        editor.setSize(940,360);
        const juce::Point<float> point{300,204};
        editor.mouseDoubleClick(cueViewFixtures::event(editor,point,point));
        check(editor.getClip().automations.size()==1,"Double click did not create the ramp");
        const auto& a=editor.getClip().automations[0];
        near(a.duration,1); near(a.points[0].value,0); near(a.points[1].value,1);
        check(a.message.address=="/parameter","Default address not assigned");
        check(juce::ModalComponentManager::getInstance()->getNumModalComponents()==0,
              "Double click unexpectedly opened a ramp modal");
        check(validateOscClip(editor.getClip()).wasOk(),"Created ramp invalid");
    });
    test("one-second OSC ramp uses true seconds even on a beat timeline",[] {
        OscClip clip; clip.useBeats=true; clip.length=8;
        OscClipEditor editor(clip,{{0,120,4,4}},
            [](const OscClip&,const OscClip&,bool){});
        editor.setSize(940,360);
        editor.mouseDoubleClick(cueViewFixtures::event(editor,{300,204},{300,204}));
        check(editor.getClip().automations.size()==1,"Beat timeline ramp not created");
        near(editor.getClip().automations[0].duration,2); // two beats at 120 BPM = 1 s
        check(validateOscClip(editor.getClip()).wasOk(),"Beat-mode ramp invalid");
    });
    test("OSC ramp can be moved and resized with timeline snapping",[] {
        OscClipEditor editor(OscClip{},{{0,120,4,4}},
            [](const OscClip&,const OscClip&,bool){});
        editor.setSize(940,360);
        const juce::Point<float> create{190,204};
        editor.mouseDoubleClick(cueViewFixtures::event(editor,create,create));
        const juce::Point<float> moveFrom{290,205},moveTo{500,205};
        editor.mouseDown(cueViewFixtures::event(editor,moveFrom,moveFrom));
        editor.mouseDrag(cueViewFixtures::event(editor,moveTo,moveFrom,1,true));
        editor.mouseUp(cueViewFixtures::event(editor,moveTo,moveFrom,1,true));
        near(editor.getClip().automations[0].time,1.5);
        const juce::Point<float> edgeFrom{603,205},edgeTo{813,205};
        editor.mouseDown(cueViewFixtures::event(editor,edgeFrom,edgeFrom));
        editor.mouseDrag(cueViewFixtures::event(editor,edgeTo,edgeFrom,1,true));
        editor.mouseUp(cueViewFixtures::event(editor,edgeTo,edgeFrom,1,true));
        near(editor.getClip().automations[0].duration,2);
        near(editor.getClip().automations[0].points.back().time,2);
        check(validateOscClip(editor.getClip()).wasOk(),"Dragged automation invalid");
    });
    test("OSC editor creates timeline events and automation ramps with separate grid and snap",[] {
        OscClipEditor editor(OscClip{},{{0,120,4,4}},
            [](const OscClip&,const OscClip&,bool){});
        editor.setSize(940,360);
        check(controlsV3::named(editor,"oscGridVisible") &&
              controlsV3::named(editor,"oscSnapEnabled") &&
              controlsV3::named(editor,"oscAddAutomation"),"OSC piano-roll controls absent");
        editor.addAutomation();
        check(editor.getClip().tracks.size()==1 && editor.getClip().automations.empty(),
              "Plus should add an empty automation track, not a ramp");
        editor.mouseDoubleClick(cueViewFixtures::event(editor,{190,204},{190,204}));
        check(editor.getClip().automations.size()==1 &&
              editor.getClip().automations[0].points.size()==2,
              "Double-click failed to create the ramp within the track");
        check(juce::ModalComponentManager::getInstance()->getNumModalComponents()==0,
              "Creating a ramp must not open any configuration dialog");
        check(validateOscClip(editor.getClip()).wasOk(),"Editor created invalid OSC automation");
    });
    test("OSC editor persists empty tracks and groups separate segments by address",[] {
        OscClipEditor editor(OscClip{},{{0,120,4,4}},
            [](const OscClip&,const OscClip&,bool){});
        editor.setSize(940,500);
        editor.addAutomation();
        check(editor.getClip().tracks.size()==1 && editor.getClip().automations.empty(),
              "Plus failed to add an empty OSC track");
        editor.mouseDoubleClick(cueViewFixtures::event(editor,{150,204},{150,204}));
        editor.mouseDoubleClick(cueViewFixtures::event(editor,{590,204},{590,204}));
        check(editor.getClip().tracks.size()==1 && editor.getClip().automations.size()==2,
              "Two separate OSC segments must share one track");
        check(editor.getClip().automations[0].message.address==
              editor.getClip().automations[1].message.address,
              "Segments lost their shared address");
        auto show=makeShow(); addColumn(show,ColumnType::osc,"OSC");
        show.rows[0].cells[1].osc=editor.getClip();
        Show loaded;
        check(decodeShow(encodeShow(show),loaded).wasOk(),"OSC track project roundtrip failed");
        check(loaded.rows[0].cells[1].osc->tracks.size()==1 &&
              loaded.rows[0].cells[1].osc->automations.size()==2,
              "OSC track metadata or segments lost after save");
    });
    test("OSC endpoint remains independent and resize preserves intermediate point timing",[] {
        OscClip clip;
        OscAutomation a;
        a.message.address="/curve";
        a.message.arguments={{OscValueType::decimal,0,0,{}}};
        a.duration=2; a.points={{0,0},{1,1},{2,0}};
        clip.automations.push_back(a);
        OscClipEditor editor(clip,{{0,120,4,4}},
            [](const OscClip&,const OscClip&,bool){});
        editor.setSize(940,420);
        check(editor.getClip().tracks.size()==1,"Legacy ramp has no track");
        // At 940px the initial axis is 840px for a four-second clip;
        // the ramp extends from x=78 to x=498.
        const juce::Point<float> edge{498,211}, extended{708,211};
        editor.mouseDown(cueViewFixtures::event(editor,edge,edge));
        editor.mouseDrag(cueViewFixtures::event(editor,extended,edge,1,true));
        editor.mouseUp(cueViewFixtures::event(editor,extended,edge,1,true));
        const auto& result=editor.getClip().automations.front();
        near(result.duration,3);
        near(result.points[1].time,1);
        near(result.points.back().time,3);
        near(result.points.front().value,0);
        near(result.points[1].value,1);
        near(result.points.back().value,0);
        check(validateOscClip(editor.getClip()).wasOk(),"0 to 1 to 0 curve invalid");
    });
    test("OSC shrinking cannot silently remove intermediate points",[] {
        OscClip clip;
        OscAutomation a; a.message.address="/param";
        a.message.arguments={{OscValueType::decimal,0,0,{}}};
        a.duration=3; a.points={{0,0},{2.5,1},{3,0}};
        clip.automations.push_back(a);
        OscClipEditor editor(clip,{{0,120,4,4}},[](const OscClip&,const OscClip&,bool){});
        editor.setSize(940,420);
        const juce::Point<float> edge{708,210},shorter{498,210};
        editor.mouseDown(cueViewFixtures::event(editor,edge,edge));
        editor.mouseDrag(cueViewFixtures::event(editor,shorter,edge,1,true));
        editor.mouseUp(cueViewFixtures::event(editor,shorter,edge,1,true));
        check(editor.getClip().automations.front().points.size()==3,
              "Shrinking removed a control point");
        near(editor.getClip().automations.front().duration,3);
        near(editor.getClip().automations.front().points[1].time,2.5);
    });
    test("OSC zoom and vertical scrolling keep segment coordinates consistent",[] {
        OscClipEditor editor(OscClip{},{{0,120,4,4}},
            [](const OscClip&,const OscClip&,bool){});
        editor.setSize(940,350);
        auto* in=dynamic_cast<juce::TextButton*>(controlsV3::named(editor,"oscZoomIn"));
        auto* fit=dynamic_cast<juce::TextButton*>(controlsV3::named(editor,"oscZoomFit"));
        check(in && fit,"OSC timeline zoom controls missing");
        in->triggerClick(); fit->triggerClick();
        check(validateOscClip(editor.getClip()).wasOk(),"Zoom modified OSC data");
        for (int i=0;i<5;++i) editor.addAutomation();
        check(editor.getClip().tracks.size()==5,"OSC plus lost tracks");
    });

    test("OSC segments on one address use temporal priority instead of insertion order",[] {
        OscClip clip; clip.length=4;
        OscAutomation later; later.message.address="/same";
        later.message.arguments={{OscValueType::decimal,0,0,{}}};
        later.time=2; later.duration=1; later.points={{0,0},{1,1}};
        auto earlier=later; earlier.time=0;
        earlier.points={{0,0},{1,0.25}};
        clip.automations={later,earlier}; // intentionally reversed
        check(validateOscClip(clip).wasOk(),"Nonoverlapping segments rejected");
        const auto packets=prepareOscClip(clip,
            [](double time){return time;},[](double seconds){return seconds;});
        OscEvent expected; expected.address="/same";
        expected.arguments={{OscValueType::decimal,0,1.0f,{}}};
        const auto encoded=encodeOscMessage(expected);
        bool found=false;
        for (const auto& packet:packets)
            if (std::abs(packet.seconds-3.0)<1e-7 && packet.bytes==encoded)
                found=true;
        check(found,"Earlier inserted segment reset the value at the end of a later segment");
    });
    test("OSC editing removes an interior point without deleting its ramp",[] {
        OscClip clip; OscAutomation a; a.message.address="/shape";
        a.message.arguments={{OscValueType::decimal,0,0,{}}};
        a.duration=2; a.points={{0,0},{1,1},{2,0}};
        clip.automations.push_back(a);
        int edits=0;
        OscClipEditor editor(clip,{{0,120,4,4}},
            [&](const OscClip&,const OscClip&,bool){++edits;});
        editor.setSize(940,420);
        // The middle point is at 1 second: x=288, y=185 in the 0..1 lane.
        const juce::Point<float> middle{288,185};
        editor.mouseDown(cueViewFixtures::event(editor,middle,middle));
        check(editor.keyPressed(juce::KeyPress(juce::KeyPress::deleteKey)),
              "Delete was not handled");
        check(editor.getClip().automations.size()==1 &&
              editor.getClip().automations[0].points.size()==2,
              "Point deletion removed the entire OSC ramp instead");
        check(edits>0 && validateOscClip(editor.getClip()).wasOk(),
              "OSC point deletion did not commit a valid undoable edit");
    });

    test("OSC vertical scrolling moves events and track hitboxes together",[] {
        OscClip clip; clip.events={{0.5,"/keep",{{OscValueType::integer,3,0,{}}}}};
        OscClipEditor editor(clip,{{0,120,4,4}},
            [](const OscClip&,const OscClip&,bool){});
        editor.setSize(940,260);
        editor.addAutomation(); editor.addAutomation(); editor.addAutomation();
        // A single scrolling offset pushes EVENTS out of view and exposes
        // the second persisted track at y=204, where the first track used to be.
        juce::MouseWheelDetails wheel;
        wheel.deltaY=-1.0f;
        const juce::Point<float> scroll{350,204};
        editor.mouseWheelMove(cueViewFixtures::event(editor,scroll,scroll),wheel);
        editor.mouseDoubleClick(cueViewFixtures::event(editor,scroll,scroll));
        check(editor.getClip().events.size()==1,"Scroll redirected a track gesture to EVENTS");
        check(editor.getClip().tracks.size()==3 &&
              editor.getClip().automations.size()==1 &&
              editor.getClip().automations[0].message.address==editor.getClip().tracks[1].message.address,
              "Scrolled automation track was not reachable at its displayed position");
        check(validateOscClip(editor.getClip()).wasOk(),"Scrolled edit damaged OSC data");
    });

}
