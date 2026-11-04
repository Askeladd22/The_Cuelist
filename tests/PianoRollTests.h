#pragma once
#include "PianoRoll.h"
#include "EditorToolbarLayout.h"
inline void pianoRollTests() {
    test("MIDI pause GO remains available without an output device",[] {
        MainComponent ui(false); addColumn(ui.show,ColumnType::midi,{});
        ui.select(0,1);
        auto& row=ui.show.rows[0];
        // Legacy pause-resume remains valid in the saved model even though
        // the playback-mode chooser is no longer exposed in the Inspector.
        row.overrideBehaviour=true;
        row.behaviour=Behaviour::pauseResume;
        row.cells[1].midi=MidiClip{4,{{0,4,60,100,1}}};
        ui.engine.connected.store(true); ui.engine.publish(makeRenderState(ui.show,ui.clips));
        check(ui.engine.send(row.id,true),"Cannot queue initial GO"); ui.engine.processBlock(nullptr,0,1,100);
        check(ui.engine.getCellPlayback(row.cells[1].id).playing,"Test clip not playing");
        ui.triggerRow(0); ui.engine.collectRetired(); ui.engine.processBlock(nullptr,0,1,100);
        check(ui.engine.getCellPlayback(row.cells[1].id).paused,"Missing output blocked pause GO");
    });
    test("N7 MIDI Piano Roll renders restrained velocity-keyed notes at dock and float sizes",[] {
        MidiClip source{4,{{0,.25,60,80,1}}};
        PianoRollEditor editor(source,[](const MidiClip&,const MidiClip&,bool){});
        auto& canvas=editor.getCanvas();
        const auto note=canvas.createComponentSnapshot({64,1233,20,16});
        check(note.isValid() && note.getWidth()==20 && note.getHeight()==16,
              "N7 MIDI clip note surface cannot be rendered");
        check(note.getPixelAt(1,7)!=note.getPixelAt(7,7),
              "Flat MIDI note lost its velocity-weighted protocol rail");
        const auto unchanged=editor.getClip().notes.front();
        for (const auto size:std::initializer_list<juce::Point<int>>{
                 {940,620},{1180,720},{710,260}}) {
            editor.setSize(size.x,size.y);
            const auto img=editor.createComponentSnapshot(editor.getLocalBounds());
            check(img.isValid() && img.getWidth()==size.x &&
                  img.getHeight()==size.y,
                  "N7 Piano Roll did not render after dock/float resize");
        }
        const auto& actual=editor.getClip().notes.front();
        check(actual.beat==unchanged.beat && actual.pitch==unchanged.pitch &&
              actual.length==unchanged.length &&
              actual.velocity==unchanged.velocity,
              "Visual-only Piano Roll refresh changed MIDI notes");
    });
    test("N7.2 compact MIDI toolbar keeps all native edit and window actions visible",[] {
        using namespace cuelist;
        PianoRollEditor editor(MidiClip{},[](const MidiClip&,const MidiClip&,bool){});
        const auto find=[&](const char* name)->juce::Component* {
            for (auto* child:editor.getChildren())
                if (child->getName()==name) return child;
            return nullptr;
        };
        const std::array<const char*,10> names{
            "midiSnap","midiChannel","midiVelocity","midiLength",
            "midiDeleteNotes","midiZoomOut","midiZoomIn",
            "clipEditorUndo","clipEditorRedo","clipEditorDetach"};
        for (int width:{710,820,940,999,1000,1180}) {
            editor.setSize(width,300);
            const auto t=MidiEditorToolbarLayout::atWidth(width);
            const std::array<juce::Rectangle<int>,10> rects{
                t.snap,t.channel,t.velocity,t.length,t.remove,t.zoomOut,
                t.zoomIn,t.undo,t.redo,t.detach};
            for(size_t i=0;i<names.size();++i) {
                auto* control=find(names[i]);
                check(control && control->isVisible() &&
                      control->getBounds()==rects[i] &&
                      control->getX()>=0 && control->getRight()<=width,
                      "N7.2 MIDI toolbar moved, hid or clipped a working action");
            }
            for(size_t i=0;i<rects.size();++i)
                for(size_t j=i+1;j<rects.size();++j)
                    check(!rects[i].intersects(rects[j]),
                          "N7.2 compact MIDI toolbar controls overlap");
            if(t.compact) {
                check(!t.title.intersects(t.undo) &&
                      !t.detach.intersects(t.collapse) &&
                      !t.collapse.intersects(t.close) &&
                      t.close.getRight()<=width,
                      "Dock actions collide with MIDI toolbar or title");
                for(const auto& bounds:rects)
                    check(!bounds.intersects(t.collapse) &&
                          !bounds.intersects(t.close),
                          "Dock Collapse/Close would obscure a MIDI control");
            }
        }
        auto* detach=dynamic_cast<juce::TextButton*>(find("clipEditorDetach"));
        check(detach!=nullptr,"Existing detachable MIDI editor missing");
        editor.setFloating(true);
        check(detach->getButtonText()=="Reattach",
              "MIDI toolbar lost floating/reattach semantic state");
        editor.setFloating(false);
        check(detach->getButtonText()=="Detach",
              "Docked MIDI editor no longer offers Detach");
    });
    test("N7.2 parent MIDI dock actions use identical responsive hitboxes",[] {
        using namespace cuelist;
        MainComponent ui(false);
        addColumn(ui.show,ColumnType::midi,"MIDI");
        check(ui.select(0,1),"N7.2 cannot select MIDI cell");
        ui.editMidiClip();
        auto* editor=ui.activeClipEditor();
        auto* collapse=controlsV3::named(ui,"clipEditorCollapse");
        auto* close=controlsV3::named(ui,"clipEditorClose");
        check(editor && collapse && close,"N7.2 dock actions missing");
        for(int width:{1180,1280,1440,1640}) {
            ui.setSize(width,900);
            const auto t=MidiEditorToolbarLayout::atWidth(editor->getWidth());
            if(!t.compact) continue;
            const auto pos=editor->getPosition();
            check(collapse->getBounds()==t.collapse.translated(pos.x,pos.y).withWidth(28) &&
                  close->getBounds()==t.close.translated(pos.x,pos.y),
                  "MIDI dock controls do not follow the actual editor layout");
            for(const auto& action:{t.undo,t.redo,t.detach}) {
                const auto absolute=action.translated(pos.x,pos.y);
                check(!absolute.intersects(collapse->getBounds()) &&
                      !absolute.intersects(close->getBounds()),
                      "MIDI parent toolbar buttons cover editor window actions");
            }
        }
        auto* collapseButton=dynamic_cast<juce::Button*>(collapse);
        check(collapseButton!=nullptr && collapse->getWidth()==28,
              "MIDI dock edge-grip missing or not compact");
        collapseButton->triggerClick();
        auto* timeline=cueViewFixtures::find<Timeline>(ui);
        check(timeline && collapse->getBottom()<=timeline->getY() &&
              close->getBottom()<=timeline->getY(),
              "MIDI collapsed dock covers Timeline");
        collapseButton->triggerClick();
        check(editor->isVisible(),"MIDI toolbar did not restore docked editor");
    });
    test("piano roll creates snaps moves resizes and deletes notes",[] {
        PianoRollEditor editor(MidiClip{},[](const MidiClip&,const MidiClip&,bool){}); auto& canvas=editor.getCanvas();
        using namespace cueViewFixtures;
        const juce::Point<float> start{70,1240};
        canvas.mouseDown(event(canvas,start,start));
        check(editor.getClip().notes.size()==1,"Double click did not create note");
        near(editor.getClip().notes[0].beat,0); near(editor.getClip().notes[0].length,.25);
        check(editor.getClip().notes[0].pitch==60,"Wrong keyboard pitch");
        const juce::Point<float> moved{150,1222};
        canvas.mouseDown(event(canvas,start,start)); canvas.mouseDrag(event(canvas,moved,start,1,true)); canvas.mouseUp(event(canvas,moved,start,1,true));
        near(editor.getClip().notes[0].beat,1); check(editor.getClip().notes[0].pitch==61,"Pitch drag failed");
        const juce::Point<float> edge{162,1222}, extended{202,1222};
        canvas.mouseDown(event(canvas,edge,edge)); canvas.mouseDrag(event(canvas,extended,edge,1,true)); canvas.mouseUp(event(canvas,extended,edge,1,true));
        near(editor.getClip().notes[0].length,.75);
        check(canvas.keyPressed(juce::KeyPress(juce::KeyPress::deleteKey)),"Delete ignored");
        check(editor.getClip().notes.empty(),"Note not deleted");
    });
    test("piano roll writes velocity channel and length immediately without Apply",[] {
        MidiClip original{4,{{0,1,60,40,2}}}; int commits=0; MidiClip saved;
        PianoRollEditor editor(original,[&](const MidiClip&,const MidiClip& clip,bool complete){
            if (complete) { ++commits; saved=clip; }
        });
        auto& canvas=editor.getCanvas();
        cueViewFixtures::click(canvas,70,1240);
        for (auto* child:editor.getChildren()) {
            if (child->getName()=="midiChannel") dynamic_cast<juce::ComboBox*>(child)->setSelectedId(8,juce::sendNotificationSync);
            if (child->getName()=="midiVelocity") dynamic_cast<juce::Slider*>(child)->setValue(112,juce::sendNotificationSync);
            if (child->getName()=="midiLength") dynamic_cast<juce::Slider*>(child)->setValue(1,juce::sendNotificationSync);
        }
        check(commits>=2 && original.notes[0].velocity==40,"Live edits not emitted or original value mutated");
        check(saved.notes[0].channel==8 && saved.notes[0].velocity==112,"Live note edits lost");
        near(editor.getClip().lengthBeats,1);
        const auto folder=juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory().getChildFile("test-artifacts");
        folder.createDirectory(); auto out=folder.getChildFile("piano-roll-preview.png").createOutputStream();
        if (out) { out->setPosition(0); out->truncate(); juce::PNGImageFormat{}.writeImageToStream(editor.createComponentSnapshot(editor.getLocalBounds()),*out); }
    });
    test("piano roll right-button marquee selects moves and deletes multiple notes",[] {
        PianoRollEditor editor(MidiClip{4,{{0,.25,60,100,1},{1,.25,61,100,1},{2,.25,60,100,1}}},[](const MidiClip&,const MidiClip&,bool){});
        auto& canvas=editor.getCanvas(); const auto time=juce::Time::getCurrentTime();
        const juce::Point<float> a{60,1195},b{245,1260};
        auto right=[&](juce::Point<float> p,bool drag) {
            return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),p,
                juce::ModifierKeys(juce::ModifierKeys::rightButtonModifier),1,0,0,0,0,
                &canvas,&canvas,time,a,time,1,drag);
        };
        canvas.mouseDown(right(a,false)); canvas.mouseDrag(right(b,true)); canvas.mouseUp(right(b,true));
        const juce::Point<float> from{70,1240},to{90,1240};
        canvas.mouseDown(cueViewFixtures::event(canvas,from,from));
        canvas.mouseDrag(cueViewFixtures::event(canvas,to,from,1,true));
        canvas.mouseUp(cueViewFixtures::event(canvas,to,from,1,true));
        near(editor.getClip().notes[0].beat,.25);
        near(editor.getClip().notes[1].beat,1.25);
        near(editor.getClip().notes[2].beat,2.25);
        canvas.keyPressed(juce::KeyPress(juce::KeyPress::deleteKey));
        check(editor.getClip().notes.empty(),"Marquee did not select and delete all notes");
    });
    test("piano roll snap Off permits unsnapped placement and movement",[] {
        PianoRollEditor editor(MidiClip{},[](const MidiClip&,const MidiClip&,bool){});
        auto& canvas=editor.getCanvas();
        for (auto* child:editor.getChildren()) if (child->getName()=="midiSnap")
            dynamic_cast<juce::ComboBox*>(child)->setSelectedId(5,juce::sendNotificationSync);
        const juce::Point<float> from{94,1240};
        canvas.mouseDown(cueViewFixtures::event(canvas,from,from));
        near(editor.getClip().notes[0].beat,.375);
        const juce::Point<float> dragFrom{100,1240},dragTo{112,1240};
        canvas.mouseDown(cueViewFixtures::event(canvas,dragFrom,dragFrom));
        canvas.mouseDrag(cueViewFixtures::event(canvas,dragTo,dragFrom,1,true));
        canvas.mouseUp(cueViewFixtures::event(canvas,dragTo,dragFrom,1,true));
        near(editor.getClip().notes[0].beat,.525);
    });
    test("MIDI grid drag and timeline offset preserve embedded notes",[] {
        MainComponent ui(false); addColumn(ui.show,ColumnType::midi,{}); addRow(ui.show,{});
        ui.show.rows[0].cells[1].midi=MidiClip{4,{{0,1,60,100,1}}};
        const int id=ui.show.rows[0].cells[1].id; ui.select(0,1); ui.moveClip(0,1,1,1);
        check(ui.show.rows[1].cells[1].id==id && ui.hasClip(1,1),"MIDI clip identity lost");
        auto* timeline=cueViewFixtures::find<Timeline>(ui); timeline->setSize(800,300);
        const juce::Point<float> from{160,136},to{224,136};
        timeline->mouseDown(cueViewFixtures::event(*timeline,from,from));
        timeline->mouseDrag(cueViewFixtures::event(*timeline,to,from,1,true)); timeline->mouseUp(cueViewFixtures::event(*timeline,to,from,1,true));
        near(ui.show.rows[1].cells[1].startSeconds,1); near(ui.clipDuration(1,1),2);
    });
    test("shared Clip Editor docks, floats, writes immediately and supports undo redo",[] {
        MainComponent ui(false); addColumn(ui.show,ColumnType::midi,{});
        ui.select(0,1);
        ui.editMidiClip();
        auto* editor=ui.activeClipEditor();
        check(editor!=nullptr && !ui.isClipEditorFloating(),"Editor not docked");
        check(cueViewFixtures::find<PianoRollEditor>(ui)==editor,"Dock is not part of the main window");
        const int cellId=ui.show.rows[0].cells[1].id;
        auto& canvas=editor->getCanvas();
        const juce::Point<float> p{70,1240};
        canvas.mouseDown(cueViewFixtures::event(canvas,p,p));
        check(ui.show.rows[0].cells[1].midi && ui.show.rows[0].cells[1].midi->notes.size()==1,
              "MIDI was not committed immediately");
        check(ui.hasUnsavedChanges(),"Live edit not marked dirty");
        check(ui.undoEdit(),"Undo failed");
        check(ui.show.rows[0].cells[1].midi->notes.empty(),"Undo did not restore empty MIDI");
        check(ui.redoEdit(),"Redo failed");
        check(ui.show.rows[0].cells[1].midi->notes.size()==1,"Redo did not restore MIDI");
        ui.detachMidiClip();
        check(ui.isClipEditorFloating() && ui.activeClipEditor()==editor,"Floating editor was not the same instance");
        ui.dockMidiClip();
        check(!ui.isClipEditorFloating() && ui.activeClipEditor()==editor,"Editor could not reattach");
        check(ui.show.rows[0].cells[1].id==cellId,"Docking moved the wrong cell");
        addRow(ui.show,{}); ui.select(1,1);
        check(editor->getClip().notes.empty(),"Changing MIDI cell did not update the shared editor");
        ui.closeMidiClip();
        check(ui.activeClipEditor()==nullptr,"Editor failed to close");
    });
}
