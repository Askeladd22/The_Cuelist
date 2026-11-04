#include "OscEditor.h"
#include "EditorToolbarLayout.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>

namespace cuelist {
namespace {
bool parseFiniteNumber(const juce::String& text,double& value) {
    const auto trimmed=text.trim();
    if (trimmed.isEmpty()) return false;
    const char* start=trimmed.toRawUTF8();
    char* end=nullptr;
    value=std::strtod(start,&end);
    return end!=start && *end=='\0' && std::isfinite(value);
}
}
OscClipEditor::OscClipEditor(OscClip value,std::vector<TempoPoint> points,
             std::function<void(const OscClip&,const OscClip&,bool)> commit)
    : clip(std::move(value)),tempo(std::move(points)),onEdit(std::move(commit)) {
    setLookAndFeel(&theme); setName("sharedOscEditor"); setWantsKeyboardFocus(true);
    for (juce::Component* component:std::initializer_list<juce::Component*>{
        &unit,&resolution,&grid,&snap,&add,&remove,&duplicate,&detach,
        &zoomOut,&zoomIn,&zoomFit,&undo,&redo})
        addAndMakeVisible(component);
    unit.setName("oscTimeUnit");
    unit.addItem("Seconds",1); unit.addItem("Beat",2);
    unit.setSelectedId(clip.useBeats ? 2 : 1,juce::dontSendNotification);
    unit.onChange=[this] {
        const bool beats=unit.getSelectedId()==2;
        if (clip.useBeats==beats) return;
        const auto before=clip;
        const auto convert=[&](double x) {
            return beats ? beatFromSeconds(tempo,x) : secondsAtBeat(tempo,x);
        };
        for (auto& e:clip.events) e.time=convert(e.time);
        for (auto& a:clip.automations) {
            const double original=a.time,begin=convert(original);
            const double end=convert(original+a.duration);
            for (auto& p:a.points) p.time=convert(original+p.time)-begin;
            a.time=begin; a.duration=end-begin;
            a.points.front().time=0; a.points.back().time=a.duration;
        }
        clip.length=convert(clip.length);
        clip.useBeats=beats;
        if (validateOscClip(clip).failed()) { clip=before; unit.setSelectedId(before.useBeats?2:1,juce::dontSendNotification); return; }
        notify(before);
    };
    resolution.setName("oscGridResolution");
    for (int i=0;i<4;++i) resolution.addItem("1/"+juce::String(4<<i),i+1);
    resolution.setSelectedId(2,juce::dontSendNotification);
    resolution.setTooltip("Musical snap resolution or fraction of a second.");
    grid.setName("oscGridVisible"); grid.setToggleState(true,juce::dontSendNotification);
    grid.onClick=[this] { repaint(); };
    snap.setName("oscSnapEnabled"); snap.setToggleState(true,juce::dontSendNotification);
    snap.onClick=[this] { repaint(); };
    add.setName("oscAddAutomation"); add.setTooltip("Add a new OSC automation track.");
    zoomOut.setName("oscZoomOut"); zoomIn.setName("oscZoomIn"); zoomFit.setName("oscZoomFit");
    undo.setName("oscEditorUndo"); redo.setName("oscEditorRedo");
    undo.setTooltip("Undo the previous OSC edit (Ctrl+Z).");
    redo.setTooltip("Redo the OSC edit (Ctrl+Y / Ctrl+Shift+Z).");
    remove.setName("oscRemoveMessage"); duplicate.setName("oscDuplicate");
    remove.setTooltip("Delete the selected OSC event, automation, or point.");
    duplicate.setTooltip("Duplicate the selected OSC event or automation.");
    grid.setTooltip("Show or hide timing grid lines; snapping is independent.");
    snap.setTooltip("Enable timing snapping independently of grid visibility.");
    unit.setTooltip("OSC time values in seconds or musical beats.");
    zoomOut.setTooltip("Zoom the OSC timeline out.");
    zoomIn.setTooltip("Zoom the OSC timeline in.");
    zoomFit.setTooltip("Fit the OSC clip to the current timeline width.");
    add.onClick=[this] { createTrack(); };
    zoomOut.onClick=[this] { zoomAround(zoom/1.5f,axisLeft()+axisWidth()*.5f); };
    zoomIn.onClick=[this] { zoomAround(zoom*1.5f,axisLeft()+axisWidth()*.5f); };
    zoomFit.onClick=[this] { zoom=1; viewStart=0; repaint(); };
    remove.onClick=[this] { deleteSelection(); };
    duplicate.onClick=[this] { duplicateSelection(); };
    detach.setName("oscDetach");
    detach.setTooltip("Detach the OSC editor into a floating window, or reattach it.");
    detach.onClick=[this] { if (onDetach) onDetach(); };
    undo.onClick=[this] { if (onUndo) onUndo(); };
    redo.onClick=[this] { if (onRedo) onRedo(); };
    for (juce::TextButton* button:std::initializer_list<juce::TextButton*>{&add,&remove,&duplicate,&detach,&zoomOut,&zoomIn,
                       &zoomFit,&undo,&redo}) button->setWantsKeyboardFocus(false);
    ensureTracks();
    setSize(940,320);
}
OscClipEditor::~OscClipEditor() { setLookAndFeel(nullptr); }
void OscClipEditor::setFloating(bool floating) { detach.setButtonText(floating?"Reattach":"Detach"); }
void OscClipEditor::loadClip(const OscClip& next) {
    ++revision; clip=next; selectedEvent=selectedTrack=selectedAutomation=selectedPoint=-1;
    selectedEvents.clear();selectedAutomations.clear();marqueeSelecting=false;
    rightMarqueePending=rightMarqueeDragged=false;
    ensureTracks();
    dragging=false; rampDrag=RampDrag::none; laneScroll=0; zoom=1; viewStart=0;
    unit.setSelectedId(clip.useBeats ? 2 : 1,juce::dontSendNotification);
    repaint();
}
void OscClipEditor::resized() {
    // All OSC edit/zoom/detach commands keep their original callbacks
    // and real hitboxes. Geometry is now a shared testable contract.
    const auto layout=OscEditorToolbarLayout::atWidth(getWidth());
    detach.setBounds(layout.detach); unit.setBounds(layout.unit);
    add.setBounds(layout.add);
    remove.setBounds(layout.remove); duplicate.setBounds(layout.duplicate);
    grid.setBounds(layout.grid); snap.setBounds(layout.snap);
    resolution.setBounds(layout.resolution);
    zoomOut.setBounds(layout.zoomOut);
    zoomIn.setBounds(layout.zoomIn); zoomFit.setBounds(layout.zoomFit);
    undo.setBounds(layout.undo); redo.setBounds(layout.redo);
    repaint();
}
float OscClipEditor::xForTime(double time) const {
    return (float)axisLeft()+(float)((time-viewStart)/(clip.length/zoom))*axisWidth();
}
double OscClipEditor::timeForX(float x) const {
    return juce::jlimit(0.0,std::max(0.0,clip.length-.00001),
        viewStart+(double)(x-axisLeft())/axisWidth()*(clip.length/zoom));
}
double OscClipEditor::quantize(double t) const {
    if (!snap.getToggleState()) return t;
    const double step=1.0/(1<<juce::jlimit(0,3,resolution.getSelectedId()-1));
    return juce::jlimit(0.0,std::max(0.0,clip.length-.00001),std::round(t/step)*step);
}
void OscClipEditor::zoomAround(double factor,float anchorX) {
    const double anchor=timeForX(anchorX);
    const double proportion=juce::jlimit(0.0,1.0,(double)(anchorX-axisLeft())/axisWidth());
    zoom=juce::jlimit(1.0,64.0,factor);
    viewStart=juce::jlimit(0.0,std::max(0.0,clip.length-clip.length/zoom),
        anchor-proportion*clip.length/zoom);
    repaint();
}
OscTrack OscClipEditor::defaultTrack() const {
    OscTrack track;
    track.message.address="/parameter/"+juce::String((int)clip.tracks.size()+1);
    if (clip.tracks.empty()) track.message.address="/parameter";
    track.message.arguments={{OscValueType::decimal,0,0,juce::String()}};
    return track;
}
void OscClipEditor::ensureTracks() {
    for (const auto& au:clip.automations) {
        const auto found=std::find_if(clip.tracks.begin(),clip.tracks.end(),
            [&](const OscTrack& t) {
                return t.message.address==au.message.address &&
                    t.argumentIndex==au.argumentIndex &&
                    t.message.arguments.size()==au.message.arguments.size() &&
                    std::equal(t.message.arguments.begin(),t.message.arguments.end(),
                        au.message.arguments.begin(),[](const OscValue& x,const OscValue& y){
                            return x.type==y.type;
                        });
            });
        if (found!=clip.tracks.end() || clip.tracks.size()>=16) continue;
        OscTrack t; t.message=au.message; t.argumentIndex=au.argumentIndex;
        for (const auto& p:au.points) {
            t.rangeMin=std::min(t.rangeMin,p.value);
            t.rangeMax=std::max(t.rangeMax,p.value);
        }
        clip.tracks.push_back(std::move(t));
    }
}
int OscClipEditor::trackForAutomation(int index) const {
    if (index<0 || index>=(int)clip.automations.size()) return -1;
    const auto& au=clip.automations[(size_t)index];
    for (int i=0;i<(int)clip.tracks.size();++i) {
        const auto& t=clip.tracks[(size_t)i];
        if (t.message.address==au.message.address && t.argumentIndex==au.argumentIndex
            && t.message.arguments.size()==au.message.arguments.size()
            && std::equal(t.message.arguments.begin(),t.message.arguments.end(),
                au.message.arguments.begin(),[](const OscValue& x,const OscValue& y){
                    return x.type==y.type;
                })) return i;
    }
    return -1;
}
int OscClipEditor::trackAt(float y) const {
    if (y>=getHeight()-26) return -1; // reserved footer is never a lane
    for (int i=0;i<=(int)clip.tracks.size();++i)
        if (y>=laneTop(i) && y<laneTop(i)+laneHeight-4) return i;
    return -1;
}
int OscClipEditor::automationAt(float x,float y) const {
    const int track=trackAt(y);
    if (track<0 || track>=(int)clip.tracks.size()) return -1;
    for (int i=(int)clip.automations.size()-1;i>=0;--i) {
        const auto& a=clip.automations[(size_t)i];
        if (trackForAutomation(i)==track && x>=xForTime(a.time)-6
            && x<=xForTime(a.time+a.duration)+6 && y>=laneTop(track)+19) return i;
    }
    return -1;
}
float OscClipEditor::yForValue(int index,double value) const {
    const int track=trackForAutomation(index);
    if (track<0) return (float)eventLaneTop();
    const auto& t=clip.tracks[(size_t)track];
    const double frac=juce::jlimit(0.0,1.0,(value-t.rangeMin)/(t.rangeMax-t.rangeMin));
    return laneTop(track)+laneHeight-13-(float)frac*(laneHeight-39);
}
double OscClipEditor::valueForY(int index,float y) const {
    const int track=trackForAutomation(index);
    if (track<0) return 0;
    const auto& t=clip.tracks[(size_t)track];
    const double frac=juce::jlimit(0.0,1.0,
        (double)(laneTop(track)+laneHeight-13-y)/(laneHeight-39));
    return t.rangeMin+(t.rangeMax-t.rangeMin)*frac;
}
void OscClipEditor::createTrack() {
    ensureTracks();
    if (clip.tracks.size()>=16) return;
    const auto before=clip;
    clip.tracks.push_back(defaultTrack());
    selectedTrack=(int)clip.tracks.size()-1;
    selectedAutomation=selectedPoint=selectedEvent=-1;
    notify(before);
}
void OscClipEditor::paint(juce::Graphics& g) {
    g.fillAll(palette::panel);
    const bool compact=getWidth()<950;
    // Keep the fixed controls and their tested compact layout. Remove
    // instructional banner clutter and distinguish OSC with one type rule.
    label(g,"OSC  /  AUTOMATION",{compact ? 120 : 13,38,
          compact ? 166 : 235,24},13,palette::text,true);
    g.setColour(palette::purple.withAlpha(.82f));
    g.fillRect(compact ? 120 : 13,63,compact ? 52 : 64,2);
    if (!compact)
        label(g,"EVENTS  /  CURVES",{255,38,
              std::max(1,getWidth()-275),24},11,palette::muted,true);
    const int left=axisLeft(),right=left+axisWidth();
    const auto ruler=juce::Rectangle<float>((float)left,rulerTop,(float)axisWidth(),rulerBottom-rulerTop);
    g.setColour(palette::background); g.fillRect(ruler);
    const double step=1.0/(1<<juce::jlimit(0,3,resolution.getSelectedId()-1));
    const double visible=clip.length/zoom;
    const double stride=std::max(1.0,std::ceil(visible/(70*step)));
    if (grid.getToggleState()) {
        g.setColour(palette::line.withAlpha(.7f));
        const double first=std::ceil(viewStart/(step*stride))*(step*stride);
        for (double t=first;t<=std::min(clip.length,viewStart+visible)+1e-7;t+=step*stride)
            g.drawVerticalLine((int)xForTime(t),rulerTop,(float)getHeight());
    }
    for (int i=0;i<=8;++i) {
        const double t=viewStart+visible*i/8.0;
        const int tick=(int)xForTime(t);
        const int labelX=juce::jlimit(left,right-58,tick-29);
        label(g,juce::String(t,2),
              {labelX,rulerTop+3,58,19},11,palette::muted,
              false,juce::Justification::centred);
    }
    // Ruler and toolbar remain fixed. Events, their labels and every track
    // share one clip region and one vertical offset.
    g.saveState();
    // Keep the footer band outside the drawing region for every track.
    const int laneBottom=getHeight()-26;
    g.reduceClipRegion(0,rulerBottom,getWidth(),
                       std::max(0,laneBottom-rulerBottom));
    const int eventsY=eventLaneTop();
    // Event lane is a real chronological track, not a raised card.
    g.setColour(palette::input);
    g.fillRect(left,eventsY,axisWidth(),eventHeight);
    g.setColour(palette::line.withAlpha(.58f));
    g.drawHorizontalLine(eventsY,0.f,(float)right);
    label(g,"EVENTS",{8,eventsY+6,68,19},12,palette::muted,true);
    for (int i=0;i<(int)clip.events.size();++i) {
        const auto& event=clip.events[(size_t)i];
        const float x=xForTime(event.time);
        const int y=eventsY+9+(i%2)*17;
        if (x<left-12 || x>right+12) continue;
        g.setColour(selectedEvents.count(i) ? palette::teal : palette::purple);
        g.fillRect(x-4,(float)y,8.f,14.f);
        const auto arguments=describeArguments(event.arguments);
        const auto summary=event.address+(arguments.isNotEmpty() ? "   "+arguments : juce::String());
        if (selectedEvents.count(i) || clip.events.size()<12)
            label(g,summary,{(int)x+7,y-2,
                std::max(1,std::min(300,right-(int)x-9)),18},10,palette::text);
    }
    for (int track=0;track<=(int)clip.tracks.size();++track) {
        const bool placeholder=track==(int)clip.tracks.size();
        if (placeholder && clip.tracks.size()>=16) break;
        const int y=laneTop(track);
        if (y>=laneBottom || y+laneHeight<rulerBottom ||
            (placeholder && y+25>laneBottom)) continue;
        g.setColour(track%2 ? palette::raised : palette::background);
        g.fillRect(left,y,axisWidth(),laneHeight-4);
        const bool selected=track==selectedTrack;
        label(g,placeholder ? "+ TRACK" : "OSC "+juce::String(track+1),
              {6,y+5,70,20},12,selected ? palette::selection : palette::muted,true);
        const auto& address=placeholder ? juce::String("Double-click to add a ramp")
            : clip.tracks[(size_t)track].message.address;
        label(g,placeholder ? address : address+"  /  double-click header to configure",
              {left+9,y+3,std::max(1,axisWidth()-18),20},
              12,palette::muted);
        if (placeholder) continue;
        const auto& lane=clip.tracks[(size_t)track];
        label(g,juce::String(lane.rangeMax,2),{4,y+27,68,17},11,palette::muted);
        label(g,juce::String(lane.rangeMin,2),{4,y+laneHeight-29,68,17},11,palette::muted);
        for (int i=0;i<(int)clip.automations.size();++i) {
            if (trackForAutomation(i)!=track) continue;
            const auto& a=clip.automations[(size_t)i];
            const float x0=xForTime(a.time),x1=xForTime(a.time+a.duration);
            if (x1<left || x0>right) continue;
            const bool active=selectedAutomations.count(i)>0;
            // Flat automation segment: subdued lane, explicit selected
            // outline and real end resize grip; no changes to drag geometry.
            const auto segment=juce::Rectangle<float>(
                x0,(float)y+20,std::max(1.0f,x1-x0),(float)laneHeight-24);
            g.setColour(palette::purple.withAlpha(active ? .18f : .10f));
            g.fillRect(segment);
            g.setColour((active ? palette::selection : palette::purple)
                .withAlpha(active ? .95f : .48f));
            g.drawRect(segment,active ? 1.2f : .65f);
            g.setColour((active ? palette::selection : palette::text)
                .withAlpha(.8f));
            g.fillRect(x1-2,(float)y+laneHeight-37,4.f,18.f);
            juce::Path curve;
            for (int p=0;p<(int)a.points.size();++p) {
                const auto& point=a.points[(size_t)p];
                const float px=xForTime(a.time+point.time),py=yForValue(i,point.value);
                if (p==0) curve.startNewSubPath(px,py);
                else curve.lineTo(px,py);
                g.setColour(active && p==selectedPoint ? palette::selection : palette::purple);
                g.fillEllipse(px-4,py-4,8,8);
            }
            g.setColour(palette::purple);
            g.strokePath(curve,juce::PathStrokeType(1.7f));
        }
    }
    if (marqueeSelecting) {
        const auto selection=juce::Rectangle<float>(
            std::min(marqueeStart.x,marqueeEnd.x),
            std::min(marqueeStart.y,marqueeEnd.y),
            std::abs(marqueeEnd.x-marqueeStart.x),
            std::abs(marqueeEnd.y-marqueeStart.y));
        g.setColour(palette::teal.withAlpha(.12f));
        g.fillRect(selection);
        g.setColour(palette::teal);
        g.drawRect(selection,1.f);
    }
    g.restoreState();
    g.setColour(palette::panel);
    g.fillRect(0,std::max(0,getHeight()-26),getWidth(),26);
    g.setColour(palette::line.withAlpha(.65f));
    g.drawHorizontalLine(std::max(0,getHeight()-26),0,(float)getWidth());
    g.setColour(palette::selection.withAlpha(.85f));
    g.drawVerticalLine((int)xForTime(cursor),(float)rulerTop,
                       (float)std::max(rulerTop,getHeight()-26));
    if (editHint.isNotEmpty())
        label(g,editHint,{13,getHeight()-24,std::max(1,getWidth()-26),20},
              11,palette::standby);
    else
        label(g,"OSC  |  "+juce::String((int)clip.events.size())+
              " events  /  "+juce::String((int)clip.tracks.size())+" tracks",
              {13,getHeight()-24,std::max(1,getWidth()-26),20},
              11,palette::muted);
}
void OscClipEditor::chooseEvent(int index) {
    if (!selectedEvents.count(index)) {
        selectedEvents.clear();selectedEvents.insert(index);
    }
    selectedAutomations.clear();
    selectedEvent=index; selectedTrack=selectedAutomation=selectedPoint=-1; repaint();
}
void OscClipEditor::chooseAutomation(int index) {
    if (!selectedAutomations.count(index)) {
        selectedAutomations.clear();selectedAutomations.insert(index);
    }
    selectedEvents.clear();
    selectedAutomation=index; selectedTrack=trackForAutomation(index);
    selectedEvent=selectedPoint=-1; repaint();
}
void OscClipEditor::notify(const OscClip& before,bool complete) {
    if (validateOscClip(clip).failed()) { clip=before; repaint(); return; }
    if (onEdit) onEdit(before,clip,complete);
    repaint();
}
int OscClipEditor::pointAt(int automationIndex,float x,float y) const {
    if (automationIndex<0 || automationIndex>=(int)clip.automations.size()) return -1;
    const auto& a=clip.automations[(size_t)automationIndex];
    for (int p=(int)a.points.size()-1;p>=0;--p) {
        const auto& pt=a.points[(size_t)p];
        if (std::abs(xForTime(a.time+pt.time)-x)<=8 &&
            std::abs(yForValue(automationIndex,pt.value)-y)<=9) return p;
    }
    return -1;
}
void OscClipEditor::mouseDown(const juce::MouseEvent& e) {
    grabKeyboardFocus();
    editHint.clear();
    if (e.mods.isPopupMenu() || e.mods.isRightButtonDown()) {
        // A click opens the existing context menu; a genuine right-drag
        // becomes a multi-selection rectangle instead of opening a popup.
        rightMarqueePending=true;
        rightMarqueeDragged=false;
        rightMarqueeStart=e.position;
        marqueeSelecting=false;
        return;
    }
    if (!e.mods.isLeftButtonDown()) return;
    const float x=e.position.x,y=e.position.y;
    if (x<axisLeft() || x>axisLeft()+axisWidth() || y<rulerTop) return;
    cursor=quantize(timeForX(x));
    dragEvent=dragAutomation=dragPoint=-1; rampDrag=RampDrag::none;
    dragging=false; changedDuringDrag=false; dragBefore=clip;
    marqueeSelecting=false;
    dragStartTime=timeForX(x);
    if (y>=rulerTop && y<rulerBottom) { repaint(); return; }
    if (y>=eventLaneTop() && y<eventLaneTop()+eventHeight) {
        for (int i=(int)clip.events.size()-1;i>=0;--i) {
            if (std::abs(xForTime(clip.events[(size_t)i].time)-x)<9) {
                if (e.mods.isShiftDown() || e.mods.isCommandDown()) {
                    if (!selectedEvents.insert(i).second) selectedEvents.erase(i);
                    selectedAutomations.clear();
                    selectedEvent=selectedEvents.empty() ? -1 : *selectedEvents.rbegin();
                    selectedAutomation=selectedPoint=selectedTrack=-1;
                    repaint();return;
                }
                chooseEvent(i);
                dragEvent=i; dragStartTime=clip.events[(size_t)i].time;
                setTooltip(clip.events[(size_t)i].address+"  "+describeArguments(clip.events[(size_t)i].arguments));
                return;
            }
        }
        if (e.mods.isCommandDown() || e.mods.isShiftDown()) {
            marqueeSelecting=true;marqueeStart=marqueeEnd=e.position;
            selectedEvents.clear();selectedAutomations.clear();
            selectedEvent=selectedAutomation=selectedPoint=-1;
            repaint();return;
        }
        if (e.getNumberOfClicks()==1 && clip.events.size()<256) {
            const auto before=clip;
            clip.events.push_back({cursor,"/cue",{{OscValueType::decimal,0,0,juce::String()}}});
            chooseEvent((int)clip.events.size()-1);
            notify(before);
        }
        return;
    }
    const int track=trackAt(y);
    if (track<0 || track>=(int)clip.tracks.size()) return;
    selectedTrack=track; selectedEvent=-1;
    const int index=automationAt(x,y);
    if (index<0) {
        if(e.mods.isCommandDown()) {
            marqueeSelecting=true;marqueeStart=marqueeEnd=e.position;
            selectedEvents.clear();selectedAutomations.clear();
        } else {
            selectedEvents.clear();selectedAutomations.clear();
        }
        selectedAutomation=selectedPoint=-1; repaint(); return;
    }
    // Ctrl/Cmd+click toggles ramp selection; Shift+click still inserts a
    // control point, retaining the original automation editing gesture.
    if(e.mods.isCommandDown()) {
        if(!selectedAutomations.insert(index).second)
            selectedAutomations.erase(index);
        selectedEvents.clear();
        selectedAutomation=selectedAutomations.empty() ?
            -1 : *selectedAutomations.rbegin();
        selectedTrack=selectedAutomation>=0 ?
            trackForAutomation(selectedAutomation) : -1;
        selectedPoint=-1;repaint();return;
    }
    chooseAutomation(index);
    const auto& a=clip.automations[(size_t)index];
    const float end=xForTime(a.time+a.duration);
    if (y<laneTop(track)+20) return;
    dragAutomation=index;
    // Shift+click adds a control point without requiring an extra dialog.
    // Check before the edge handle so the established Shift gesture remains valid.
    if (e.mods.isShiftDown() && a.points.size()<128) {
        const double relative=cursor-a.time;
        if (relative>.0001 && relative<a.duration-.0001) {
            auto before=clip;
            auto& points=clip.automations[(size_t)index].points;
            if (std::none_of(points.begin(),points.end(),
                [&](const OscControlPoint& p){return std::abs(p.time-relative)<.0001;})) {
                double value=valueForY(index,y);
                if (a.message.arguments[(size_t)a.argumentIndex].type==OscValueType::integer)
                    value=std::round(value);
                points.push_back({relative,value});
                std::sort(points.begin(),points.end(),[](const auto& x,const auto& z){return x.time<z.time;});
                selectedPoint=(int)(std::find_if(points.begin(),points.end(),
                    [&](const auto& p){return std::abs(p.time-relative)<.0001;})-points.begin());
                notify(before);
            }
        }
        dragAutomation=-1; return;
    }
    const int point=pointAt(index,x,y);
    if (point>=0) {
        selectedPoint=dragPoint=point;
        rampDrag=RampDrag::point; repaint(); return;
    }
    if (std::abs(x-end)<=9) { rampDrag=RampDrag::resize; return; }
    rampDrag=RampDrag::move;
}
void OscClipEditor::updateMarqueeSelection(juce::Point<float> end) {
    marqueeEnd=end;
    const auto rect=juce::Rectangle<float>(
        std::min(marqueeStart.x,marqueeEnd.x),
        std::min(marqueeStart.y,marqueeEnd.y),
        std::abs(marqueeEnd.x-marqueeStart.x),
        std::abs(marqueeEnd.y-marqueeStart.y));
    selectedEvents.clear();selectedAutomations.clear();
    for(int i=0;i<(int)clip.events.size();++i) {
        const float px=xForTime(clip.events[(size_t)i].time);
        const int py=eventLaneTop()+9+(i%2)*17;
        if(rect.intersects(juce::Rectangle<float>(px-5.f,(float)py,10.f,15.f)))
            selectedEvents.insert(i);
    }
    for(int i=0;i<(int)clip.automations.size();++i) {
        const auto& ramp=clip.automations[(size_t)i];
        const int track=trackForAutomation(i);
        if(track<0)continue;
        const auto box=juce::Rectangle<float>(
            xForTime(ramp.time),(float)laneTop(track)+20,
            std::max(1.f,xForTime(ramp.time+ramp.duration)-xForTime(ramp.time)),
            (float)laneHeight-24);
        if(rect.intersects(box))selectedAutomations.insert(i);
    }
    selectedEvent=selectedEvents.empty()?-1:*selectedEvents.rbegin();
    selectedAutomation=selectedAutomations.empty()?-1:*selectedAutomations.rbegin();
    selectedTrack=selectedAutomation>=0 ? trackForAutomation(selectedAutomation) : -1;
    selectedPoint=-1;
    repaint();
}
void OscClipEditor::mouseDrag(const juce::MouseEvent& e) {
    if(rightMarqueePending && !rightMarqueeDragged) {
        if(e.position.getDistanceFrom(rightMarqueeStart)<4.f)return;
        rightMarqueeDragged=true;
        const bool inCanvas=rightMarqueeStart.x>=axisLeft() &&
            rightMarqueeStart.x<=axisLeft()+axisWidth() &&
            rightMarqueeStart.y>=eventLaneTop() &&
            rightMarqueeStart.y<getHeight()-26;
        if(inCanvas) {
            marqueeSelecting=true;
            marqueeStart=rightMarqueeStart;
        }
    }
    if (marqueeSelecting) {
        updateMarqueeSelection(e.position);
        return;
    }
    if (rightMarqueePending) return;
    if (dragEvent>=0 && dragEvent<(int)clip.events.size()) {
        const auto before=clip;
        const double shift=quantize(timeForX(e.position.x))-
                           dragBefore.events[(size_t)dragEvent].time;
        double earliest=clip.length,latest=0;
        for(int index:selectedEvents) {
            earliest=std::min(earliest,dragBefore.events[(size_t)index].time);
            latest=std::max(latest,dragBefore.events[(size_t)index].time);
        }
        const double delta=juce::jlimit(-earliest,clip.length-.0001-latest,shift);
        for(int index:selectedEvents)
            clip.events[(size_t)index].time=
                dragBefore.events[(size_t)index].time+delta;
        if(clip.events[(size_t)dragEvent].time!=before.events[(size_t)dragEvent].time) {
            notify(before,false);
            changedDuringDrag=true;
        }
        return;
    }
    if (dragAutomation<0 || dragAutomation>=(int)clip.automations.size()
        || rampDrag==RampDrag::none) return;
    const auto before=clip;
    auto& a=clip.automations[(size_t)dragAutomation];
    const auto& original=dragBefore.automations[(size_t)dragAutomation];
    if (rampDrag==RampDrag::move) {
        if(selectedAutomations.size()>1) {
            double first=clip.length,last=0;
            for(int index:selectedAutomations) {
                const auto& ramp=dragBefore.automations[(size_t)index];
                first=std::min(first,ramp.time);
                last=std::max(last,ramp.time+ramp.duration);
            }
            const double raw=quantize(timeForX(e.position.x))-
                             quantize(dragStartTime);
            const double shift=juce::jlimit(-first,clip.length-last,raw);
            for(int index:selectedAutomations)
                clip.automations[(size_t)index].time=
                    dragBefore.automations[(size_t)index].time+shift;
        } else {
            const double delta=timeForX(e.position.x)-dragStartTime;
            a.time=juce::jlimit(0.0,std::max(0.0,clip.length-original.duration),
                               quantize(original.time+delta));
        }
    } else if (rampDrag==RampDrag::resize ||
               (rampDrag==RampDrag::point && dragPoint==(int)a.points.size()-1)) {
        const double raw=juce::jlimit(0.0,clip.length,
            viewStart+(double)(e.position.x-axisLeft())/axisWidth()*clip.length/zoom);
        const double step=1.0/(1<<juce::jlimit(0,3,resolution.getSelectedId()-1));
        const double end=snap.getToggleState() ? std::round(raw/step)*step : raw;
        const double next=end-original.time;
        const double latestInterior=original.points.size()>2 ?
            original.points[original.points.size()-2].time : 0;
        if (next<=latestInterior+.0001 || next<.01 ||
            original.time+next>clip.length+1e-7) {
            editHint="Cannot shorten: an intermediate point would be outside the ramp.";
            repaint(); return;
        }
        a.duration=next;
        a.points.back().time=next;
        // Intermediate points remain at their original offsets; no implicit stretch.
        if (rampDrag==RampDrag::point)
            a.points.back().value=valueForY(dragAutomation,e.position.y);
    } else if (rampDrag==RampDrag::point && dragPoint>=0) {
        auto& pt=a.points[(size_t)dragPoint];
        if (dragPoint>0) {
            const double relative=quantize(timeForX(e.position.x))-a.time;
            pt.time=juce::jlimit(a.points[(size_t)dragPoint-1].time+.0001,
                a.points[(size_t)dragPoint+1].time-.0001,relative);
        }
        pt.value=valueForY(dragAutomation,e.position.y);
    }
    if (rampDrag==RampDrag::point && dragPoint>=0 &&
        a.message.arguments[(size_t)a.argumentIndex].type==OscValueType::integer)
        a.points[(size_t)dragPoint].value=std::round(a.points[(size_t)dragPoint].value);
    const auto& prior=before.automations[(size_t)dragAutomation];
    bool changed=a.time!=prior.time || a.duration!=prior.duration || a.points.size()!=prior.points.size();
    if (rampDrag==RampDrag::point && dragPoint>=0)
        changed=changed || a.points[(size_t)dragPoint].time!=prior.points[(size_t)dragPoint].time ||
            a.points[(size_t)dragPoint].value!=prior.points[(size_t)dragPoint].value;
    if (changed) {
        if (validateOscClip(clip).failed()) { clip=before; repaint(); return; }
        editHint.clear(); notify(before,false); changedDuringDrag=true;
    }
}
void OscClipEditor::mouseUp(const juce::MouseEvent& e) {
    if(rightMarqueePending) {
        const bool click=!rightMarqueeDragged;
        rightMarqueePending=rightMarqueeDragged=false;
        marqueeSelecting=false;
        if(click) showContext(e); // ordinary right-click remains unchanged
        else repaint();
        return;
    }
    if(marqueeSelecting) {
        marqueeSelecting=false;repaint();return;
    }
    if (changedDuringDrag && onEdit) onEdit(dragBefore,clip,true);
    dragEvent=dragAutomation=dragPoint=-1;
    rampDrag=RampDrag::none; changedDuringDrag=false; repaint();
}
void OscClipEditor::mouseDoubleClick(const juce::MouseEvent& e) {
    if (e.position.y>=eventLaneTop() && e.position.y<eventLaneTop()+eventHeight) {
        for (int i=(int)clip.events.size()-1;i>=0;--i)
            if (std::abs(xForTime(clip.events[(size_t)i].time)-e.position.x)<10) {
                editEvent(i); return;
            }
    }
    const int track=trackAt(e.position.y);
    if (track<0) return;
    if (track<(int)clip.tracks.size() &&
        (e.position.x<axisLeft() || e.position.y<laneTop(track)+20)) {
        editTrackSettings(track); return;
    }
    if (e.position.x<axisLeft() || e.position.x>axisLeft()+axisWidth()) return;
    const int index=automationAt(e.position.x,e.position.y);
    if (index>=0) editAutomation(index);
    else createAutomationAt(quantize(timeForX(e.position.x)),track);
}
void OscClipEditor::mouseWheelMove(const juce::MouseEvent& e,const juce::MouseWheelDetails& wheel) {
    if (e.mods.isCommandDown() || e.mods.isCtrlDown()) {
        zoomAround(zoom*std::pow(1.25,(double)wheel.deltaY*4),e.position.x);
    } else if (e.mods.isShiftDown()) {
        viewStart=juce::jlimit(0.0,std::max(0.0,clip.length-clip.length/zoom),
            viewStart-wheel.deltaY*clip.length/zoom*.2);
        repaint();
    } else {
        const int bottom=laneTop((int)clip.tracks.size()+1)+laneHeight;
        laneScroll=juce::jlimit(0,std::max(0,bottom+laneScroll-getHeight()+26),
            laneScroll-(int)(wheel.deltaY*100));
        repaint();
    }
}
bool OscClipEditor::keyPressed(const juce::KeyPress& k) {
    if(k.getModifiers().isCommandDown() &&
       (k.getKeyCode()=='A' || k.getKeyCode()=='a')) {
        selectedEvents.clear();selectedAutomations.clear();
        for(int i=0;i<(int)clip.events.size();++i) selectedEvents.insert(i);
        for(int i=0;i<(int)clip.automations.size();++i)selectedAutomations.insert(i);
        selectedEvent=selectedEvents.empty()?-1:*selectedEvents.rbegin();
        selectedAutomation=selectedAutomations.empty()?-1:*selectedAutomations.rbegin();
        selectedPoint=-1;
        repaint();return true;
    }
    if (k.getKeyCode()==juce::KeyPress::deleteKey || k.getKeyCode()==juce::KeyPress::backspaceKey) {
        deleteSelection(); return true;
    }
    if (k.getModifiers().isCommandDown() && (k.getTextCharacter()=='d'||k.getTextCharacter()=='D')) {
        duplicateSelection(); return true;
    }
    if (k.getModifiers().isCommandDown() && (k.getTextCharacter()=='z'||k.getTextCharacter()=='Z')) {
        if (k.getModifiers().isShiftDown()) { if (onRedo) onRedo(); }
        else if (onUndo) onUndo();
        return true;
    }
    return false;
}
void OscClipEditor::removeSelectedPoint() {
    if (selectedAutomation<0 || selectedAutomation>=(int)clip.automations.size()) return;
    auto& points=clip.automations[(size_t)selectedAutomation].points;
    if (selectedPoint<=0 || selectedPoint>=(int)points.size()-1) {
        editHint="The first and last control points cannot be deleted.";
        repaint(); return;
    }
    const auto before=clip;
    points.erase(points.begin()+selectedPoint);
    selectedPoint=-1;
    notify(before);
}
void OscClipEditor::showContext(const juce::MouseEvent& e) {
    if (e.position.y>=eventLaneTop() && e.position.y<eventLaneTop()+eventHeight) {
        for (int i=(int)clip.events.size()-1;i>=0;--i)
            if (std::abs(xForTime(clip.events[(size_t)i].time)-e.position.x)<10)
                {chooseEvent(i); break;}
    } else {
        const int hit=automationAt(e.position.x,e.position.y);
        if (hit>=0) {
            chooseAutomation(hit);
            const int p=pointAt(hit,e.position.x,e.position.y);
            if (p>=0) {
                selectedPoint=p;
                juce::PopupMenu pointMenu;
                pointMenu.setLookAndFeel(&getLookAndFeel());
                pointMenu.addItem(10,"Delete point",p>0 && p+1<(int)clip.automations[(size_t)hit].points.size());
                auto safe=juce::Component::SafePointer<OscClipEditor>(this);
                pointMenu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
                    [safe](int action) {if (safe && action==10) safe->removeSelectedPoint();});
                repaint(); return;
            }
        }
        const int track=trackAt(e.position.y);
        if (track>=0 && track<(int)clip.tracks.size()) selectedTrack=track;
    }
    juce::PopupMenu menu;
    menu.setLookAndFeel(&getLookAndFeel());
    menu.addItem(1,"Edit",selectedEvent>=0 || selectedAutomation>=0);
    menu.addItem(2,"Duplicate",selectedEvent>=0 || selectedAutomation>=0);
    menu.addItem(3,"Delete",selectedEvent>=0 || selectedAutomation>=0);
    menu.addItem(4,"OSC track settings",selectedTrack>=0);
    auto safe=juce::Component::SafePointer<OscClipEditor>(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),[safe](int item) {
        if (!safe) return;
        if (item==1) {
            if (safe->selectedEvent>=0) safe->editEvent(safe->selectedEvent);
            else if (safe->selectedAutomation>=0) safe->editAutomation(safe->selectedAutomation);
        } else if (item==2) safe->duplicateSelection();
        else if (item==3) safe->deleteSelection();
        else if (item==4 && safe->selectedTrack>=0)
            safe->editTrackSettings(safe->selectedTrack);
    });
}
void OscClipEditor::addEvent() {
    if (clip.events.size()>=256) return;
    const auto before=clip;
    // The message must have a typed argument. Historically this button
    // created an empty typetag list: receivers saw only its OSC address.
    clip.events.push_back({cursor,"/cue",{{OscValueType::decimal,0,0.0f,juce::String()}}});
    chooseEvent((int)clip.events.size()-1);
    notify(before); editEvent(selectedEvent);
}
void OscClipEditor::addAutomation() { createTrack(); }
void OscClipEditor::createAutomationAt(double start,int trackIndex) {
    if (clip.automations.size()>=16 || !std::isfinite(start) ||
        trackIndex<0 || trackIndex>(int)clip.tracks.size()) return;
    const auto before=clip;
    if (trackIndex==(int)clip.tracks.size()) clip.tracks.push_back(defaultTrack());
    const auto& track=clip.tracks[(size_t)trackIndex];
    OscAutomation au;
    au.time=juce::jlimit(0.0,clip.length,start);
    au.duration=clip.useBeats
        ? beatFromSeconds(tempo,secondsAtBeat(tempo,au.time)+1.0)-au.time
        : 1.0;
    if (!(au.duration>0) || au.time+au.duration>86400) {clip=before;return;}
    au.message=track.message;
    au.argumentIndex=track.argumentIndex;
    au.points={{0,0},{au.duration,1}};
    clip.length=std::max(clip.length,au.time+au.duration);
    clip.automations.push_back(au);
    if (validateOscClip(clip).failed()) {
        clip=before; editHint="Cannot add an overlapping OSC segment."; repaint(); return;
    }
    chooseAutomation((int)clip.automations.size()-1);
    notify(before);
}
void OscClipEditor::editTrackSettings(int index) {
    if (index<0 || index>=(int)clip.tracks.size()) return;
    const auto original=clip.tracks[(size_t)index];
    auto* dialog=new juce::AlertWindow(
        "OSC track settings",
        "Shared by every automation segment on this track. "
        "Edit address, typed arguments, numeric target and range here.",
        juce::MessageBoxIconType::NoIcon,this);
    dialog->addTextEditor("address",original.message.address,"OSC address");
    dialog->addTextEditor("args",describeArguments(original.message.arguments),
                          "Ordered arguments: i:7; f:0.5; s:scene");
    dialog->addTextEditor("argumentIndex",juce::String(original.argumentIndex+1),
                          "Automated argument (1-based)");
    dialog->addTextEditor("rangeMin",juce::String(original.rangeMin,3),"Minimum");
    dialog->addTextEditor("rangeMax",juce::String(original.rangeMax,3),"Maximum");
    dialog->addButton("Save",1);dialog->addButton("Cancel",0);
    auto safe=juce::Component::SafePointer<OscClipEditor>(this);
    const auto generation=revision;
    dialog->enterModalState(true,juce::ModalCallbackFunction::create(
        [safe,dialog,index,generation](int response) {
            if (!safe || response!=1 || generation!=safe->revision ||
                index>=(int)safe->clip.tracks.size()) return;
            auto settings=safe->clip.tracks[(size_t)index];
            settings.message.address=
                dialog->getTextEditorContents("address").trim();
            std::vector<OscValue> args;
            auto parse=OscClipEditor::parseArguments(
                dialog->getTextEditorContents("args"),args);
            const auto indexText=
                dialog->getTextEditorContents("argumentIndex").trim();
            double minimum=0,maximum=0;
            if (parse.failed() || indexText.isEmpty() ||
                indexText.retainCharacters("0123456789")!=indexText ||
                !parseFiniteNumber(dialog->getTextEditorContents("rangeMin"),minimum) ||
                !parseFiniteNumber(dialog->getTextEditorContents("rangeMax"),maximum)) {
                safe->editHint="Invalid track settings. Check OSC types and numeric range.";
                safe->repaint();return;
            }
            settings.message.arguments=std::move(args);
            settings.argumentIndex=indexText.getIntValue()-1;
            settings.rangeMin=minimum;settings.rangeMax=maximum;
            auto modified=safe->clip;
            if(auto result=updateOscTrackSettings(modified,index,settings);result.failed()) {
                safe->editHint=result.getErrorMessage();
                safe->repaint();return;
            }
            const auto before=safe->clip;
            safe->clip=std::move(modified);
            safe->selectedTrack=index;
            safe->editHint.clear();
            safe->notify(before);
        }),true);
}
void OscClipEditor::deleteSelection() {
    if(selectedEvents.empty() && selectedAutomations.size()==1 &&
       selectedPoint>=0) {
        removeSelectedPoint();return;
    }
    const auto before=clip;
    bool changed=false;
    for(auto it=selectedEvents.rbegin();it!=selectedEvents.rend();++it)
        if(*it>=0 && *it<(int)clip.events.size()) {
            clip.events.erase(clip.events.begin()+*it);
            changed=true;
        }
    for(auto it=selectedAutomations.rbegin();it!=selectedAutomations.rend();++it)
        if(*it>=0 && *it<(int)clip.automations.size()) {
            clip.automations.erase(clip.automations.begin()+*it);
            changed=true;
        }
    selectedEvents.clear();selectedAutomations.clear();
    selectedEvent=selectedAutomation=selectedPoint=-1;
    if(!changed && selectedTrack>=0 &&
       selectedTrack<(int)clip.tracks.size()) {
        for(int i=(int)clip.automations.size()-1;i>=0;--i)
            if(trackForAutomation(i)==selectedTrack)
                clip.automations.erase(clip.automations.begin()+i);
        clip.tracks.erase(clip.tracks.begin()+selectedTrack);
        selectedTrack=-1;
        changed=true;
    }
    if(changed) notify(before);
}
void OscClipEditor::duplicateSelection() {
    const auto before=clip;
    bool changed=false;
    if(!selectedEvents.empty() &&
       clip.events.size()+selectedEvents.size()<=256) {
        std::set<int> copies;
        for(int index:selectedEvents) {
            if(index<0 || index>=(int)before.events.size())continue;
            auto item=before.events[(size_t)index];
            item.time=juce::jlimit(0.0,clip.length-.0001,item.time+.25);
            copies.insert((int)clip.events.size());
            clip.events.push_back(std::move(item));
            changed=true;
        }
        selectedEvents=std::move(copies);
        selectedEvent=selectedEvents.empty()?-1:*selectedEvents.rbegin();
    }
    if(!selectedAutomations.empty() &&
       clip.automations.size()+selectedAutomations.size()<=16) {
        double first=clip.length;
        for(int index:selectedAutomations)
            if(index>=0 && index<(int)before.automations.size())
                first=std::min(first,before.automations[(size_t)index].time);
        const double shift=selectedAutomations.size()>1 ?
            clip.length-first+.25 : 0;
        std::set<int> copies;
        for(int index:selectedAutomations) {
            if(index<0 || index>=(int)before.automations.size())continue;
            auto item=before.automations[(size_t)index];
            item.time+=selectedAutomations.size()>1 ? shift :
                       item.duration+.25;
            if(item.time+item.duration>86400) {
                clip=before;editHint="Duplicate exceeds the OSC clip limit.";
                repaint();return;
            }
            clip.length=std::max(clip.length,item.time+item.duration);
            copies.insert((int)clip.automations.size());
            clip.automations.push_back(std::move(item));
            changed=true;
        }
        selectedAutomations=std::move(copies);
        selectedAutomation=selectedAutomations.empty()?
            -1:*selectedAutomations.rbegin();
    }
    if(!changed)return;
    if(validateOscClip(clip).failed()) {
        clip=before;editHint="Duplicated OSC items conflict with existing events.";
        selectedEvent=selectedAutomation=-1;
        selectedEvents.clear();selectedAutomations.clear();
        repaint();return;
    }
    notify(before);
}
void OscClipEditor::editEvent(int index) {
    if (index<0 || index>=(int)clip.events.size()) return;
    chooseEvent(index);
    const auto event=clip.events[(size_t)index];
    auto* dialog=new juce::AlertWindow("OSC message",
        "Time, address and ordered arguments: i:7; f:0.5; s:scene; b:true",
        juce::MessageBoxIconType::NoIcon,this);
    dialog->addTextEditor("time",juce::String(event.time,3),clip.useBeats?"Beat":"Seconds");
    dialog->addTextEditor("address",event.address,"OSC address");
    dialog->addTextEditor("args",describeArguments(event.arguments),"Arguments");
    dialog->addButton("Save",1); dialog->addButton("Cancel",0);
    auto safe=juce::Component::SafePointer<OscClipEditor>(this); const auto generation=revision;
    dialog->enterModalState(true,juce::ModalCallbackFunction::create(
        [safe,dialog,index,generation](int answer) {
            if (!safe || answer!=1 || generation!=safe->revision || index>=(int)safe->clip.events.size()) return;
            const auto t=dialog->getTextEditorContents("time").trim();
            if (t.isEmpty() || t.retainCharacters("0123456789.")!=t) return;
            auto modified=safe->clip;
            auto& e=modified.events[(size_t)index];
            e.time=t.getDoubleValue(); e.address=dialog->getTextEditorContents("address").trim();
            if (parseArguments(dialog->getTextEditorContents("args"),e.arguments).failed()) return;
            if (e.time>=modified.length) return;
            if (validateOscClip(modified).failed()) return;
            const auto before=safe->clip;
            safe->clip=std::move(modified);
            safe->notify(before);
        }),true);
}
void OscClipEditor::editAutomation(int index) {
    if (index<0 || index>=(int)clip.automations.size()) return;
    chooseAutomation(index);
    const auto original=clip.automations[(size_t)index];
    const int trackIndex=trackForAutomation(index);
    if(trackIndex<0) return;
    const auto& track=clip.tracks[(size_t)trackIndex];
    const auto type=original.message.arguments[(size_t)original.argumentIndex].type;
    auto* dialog=new juce::AlertWindow(
        "OSC ramp values",
        "Change only the start and end values of this segment. "
        "Double-click the track header to edit address, arguments, "
        "target type and parameter range.",
        juce::MessageBoxIconType::NoIcon,this);
    dialog->addTextEditor("start",juce::String(original.points.front().value,3),"Start value");
    dialog->addTextEditor("end",juce::String(original.points.back().value,3),"End value");
    dialog->addButton("Save",1);dialog->addButton("Cancel",0);
    auto safe=juce::Component::SafePointer<OscClipEditor>(this);
    const auto generation=revision;
    dialog->enterModalState(true,juce::ModalCallbackFunction::create(
        [safe,dialog,index,trackIndex,type,generation](int response) {
            if(!safe || response!=1 || generation!=safe->revision ||
               index>=(int)safe->clip.automations.size() ||
               trackIndex>=(int)safe->clip.tracks.size()) return;
            double start=0,end=0;
            if(!parseFiniteNumber(dialog->getTextEditorContents("start"),start) ||
               !parseFiniteNumber(dialog->getTextEditorContents("end"),end)) {
                safe->editHint="Enter valid numeric start and end values.";
                safe->repaint();return;
            }
            const auto& track=safe->clip.tracks[(size_t)trackIndex];
            if(start<track.rangeMin || start>track.rangeMax ||
               end<track.rangeMin || end>track.rangeMax ||
               (type==OscValueType::integer &&
                   (std::floor(start)!=start || std::floor(end)!=end))) {
                safe->editHint="Ramp values must fit the track range and numeric type.";
                safe->repaint();return;
            }
            auto modified=safe->clip;
            auto& ramp=modified.automations[(size_t)index];
            ramp.points.front().value=start;
            ramp.points.back().value=end;
            if(auto result=validateOscClip(modified);result.failed()) {
                safe->editHint=result.getErrorMessage();
                safe->repaint();return;
            }
            const auto before=safe->clip;
            safe->clip=std::move(modified);
            safe->selectedTrack=trackIndex;
            safe->editHint.clear();
            safe->notify(before);
        }),true);
}
juce::Result OscClipEditor::parseArguments(const juce::String& text,std::vector<OscValue>& output) {
    output.clear();
    if (text.trim().isEmpty()) return juce::Result::ok();
    for (auto item:juce::StringArray::fromTokens(text,";","")) {
        const auto part=item.trim();
        if (part.length()<2 || part[1]!=':') return juce::Result::fail("Argument has no type.");
        const auto v=part.substring(2).trim();
        OscValue a;
        if (part[0]=='i') {
            if (v.isEmpty() || v.retainCharacters("-0123456789")!=v) return juce::Result::fail("Invalid OSC integer.");
            const auto n=v.getLargeIntValue();
            if (n < -2147483648LL || n>2147483647LL) return juce::Result::fail("OSC integer out of range.");
            a.type=OscValueType::integer; a.integer=(int)n;
        } else if (part[0]=='f') {
            if (v.isEmpty() || v.retainCharacters("-0123456789.eE+")!=v) return juce::Result::fail("Invalid OSC float.");
            a.type=OscValueType::decimal; a.decimal=(float)v.getDoubleValue();
            if (!std::isfinite(a.decimal)) return juce::Result::fail("Invalid OSC float.");
        } else if (part[0]=='s') { a.type=OscValueType::text; a.text=v; }
        else if (part[0]=='b') {
            if (v!="true" && v!="false") return juce::Result::fail("OSC boolean must be true or false.");
            a.type=OscValueType::boolean; a.boolean=v=="true";
        } else return juce::Result::fail("Unknown OSC argument type.");
        output.push_back(std::move(a));
        if (output.size()>16) return juce::Result::fail("Maximum 16 arguments.");
    }
    return juce::Result::ok();
}
juce::String OscClipEditor::describeArguments(const std::vector<OscValue>& values) {
    juce::StringArray items;
    for (const auto& a:values)
        items.add(a.type==OscValueType::integer ? "i:"+juce::String(a.integer)
            : a.type==OscValueType::decimal ? "f:"+juce::String(a.decimal)
            : a.type==OscValueType::boolean ? "b:"+(a.boolean?juce::String("true"):juce::String("false"))
            : "s:"+a.text);
    return items.joinIntoString("; ");
}
juce::String OscClipEditor::describePoints(const std::vector<OscControlPoint>& points) {
    juce::StringArray tokens;
    for (const auto& p:points) tokens.add(juce::String(p.time,3)+":"+juce::String(p.value,3));
    return tokens.joinIntoString("; ");
}
juce::Result OscClipEditor::parsePoints(const juce::String& text,std::vector<OscControlPoint>& output) {
    output.clear();
    for (auto token:juce::StringArray::fromTokens(text,";","")) {
        const auto v=token.trim();
        const int colon=v.indexOfChar(':');
        if (colon<=0) return juce::Result::fail("Use time:value; time:value.");
        const auto t=v.substring(0,colon).trim(),x=v.substring(colon+1).trim();
        if (t.isEmpty() || x.isEmpty() ||
            t.retainCharacters("0123456789.")!=t ||
            x.retainCharacters("-0123456789.eE+")!=x) return juce::Result::fail("Invalid point.");
        output.push_back({t.getDoubleValue(),x.getDoubleValue()});
        if (output.size()>128) return juce::Result::fail("Too many points.");
    }
    return juce::Result::ok();
}
double OscClipEditor::beatFromSeconds(const std::vector<TempoPoint>& points,double seconds) {
    double lo=0,hi=std::max(1.0,seconds*4);
    while (secondsAtBeat(points,hi)<seconds && hi<1e7) hi*=2;
    for (int i=0;i<48;++i) {
        const auto mid=(lo+hi)*.5;
        if (secondsAtBeat(points,mid)<seconds) lo=mid; else hi=mid;
    }
    return (lo+hi)*.5;
}
}
