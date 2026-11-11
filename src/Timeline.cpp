#include "MainComponent.h"
#include <cmath>
#include <algorithm>

namespace cuelist {
namespace {
constexpr int timelineStart=140, tracksTop=106, trackHeight=42;
constexpr int markerLaneTop=79, markerLaneBottom=105;
// The small Timeline cannot fit its toolbar, ruler, tracks and footer at
// once. These visibility thresholds match the actual content's space budget.
constexpr int detailedToolbarHeight=120;
constexpr int completeTimelineHeight=170;
// Invert the monotonic tempo map to place grid lines in musical time,
// including cues with multiple BPM changes.
double beatAtSeconds(const std::vector<TempoPoint>& tempo,double seconds) {
    if (seconds<=0) return 0;
    double low=0,high=std::max(1.0,seconds*2);
    while (secondsAtBeat(tempo,high)<seconds && high<1.0e7) high*=2;
    for (int i=0;i<48;++i) {
        const double middle=(low+high)*.5;
        if (secondsAtBeat(tempo,middle)<seconds) low=middle;
        else high=middle;
    }
    return (low+high)*.5;
}
juce::String timeText(double seconds) {
    const int whole=(int)seconds;
    return juce::String(whole/60).paddedLeft('0',2)+":"+juce::String(whole%60).paddedLeft('0',2)+"."+juce::String((int)(seconds*10)%10);
}
juce::Rectangle<int> zoomButton(int width,int index) {
    return {width-140+34*index,10,index==2 ? 52 : 28,25};
}
}
void Timeline::MagnetToggle::paintButton(juce::Graphics& g,bool hovered,bool pressed) {
    const bool enabled=getToggleState();
    const auto bounds=getLocalBounds().toFloat().reduced(2.0f);
    const IconButtonState state{isEnabled(),hovered,pressed,enabled,
                                hasKeyboardFocus(false)};
    drawIconButtonSurface(g,bounds,palette::teal,state,5.0f);

    // The Unicode magnet U+1F9F2 is absent from our embedded Symbols 2
    // revision. Render a precise magnet *icon* instead of a missing glyph
    // or a platform-dependent colour emoji. Centre it in the 41x25 control.
    const float cx=bounds.getCentreX(),cy=bounds.getCentreY();
    const float x=cx-9.0f,y=cy-9.0f;
    juce::Path magnet;
    magnet.startNewSubPath(x+3.0f,y+2.5f);
    magnet.lineTo(x+3.0f,y+11.3f);
    magnet.cubicTo(x+3.0f,y+20.7f,x+15.0f,y+20.7f,x+15.0f,y+11.3f);
    magnet.lineTo(x+15.0f,y+2.5f);
    g.setColour(enabled ? palette::teal : palette::muted);
    g.strokePath(magnet,juce::PathStrokeType(
        2.9f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    g.setColour(enabled ? palette::red : palette::muted);
    g.fillRoundedRectangle(x+1.6f,y+1.1f,3.0f,5.0f,1.0f);
    g.setColour(enabled ? palette::text : palette::muted);
    g.fillRoundedRectangle(x+13.4f,y+1.1f,3.0f,5.0f,1.0f);
    if (!enabled) {
        // A clear slash differentiates OFF from a merely dimmed ON icon.
        g.setColour(palette::panel);
        g.drawLine(x+1.5f,y+1.5f,x+17.5f,y+18.0f,4.0f);
        g.setColour(palette::red);
        g.drawLine(x+1.5f,y+1.5f,x+17.5f,y+18.0f,1.8f);
    }
}

Timeline::Timeline(MainComponent& p) : owner(p) {
    addAndMakeVisible(horizontalScroll);
    addChildComponent(markerEditor);
    markerEditor.setName("timelineMarkerInlineEditor");
    markerEditor.setMultiLine(false);
    markerEditor.setReturnKeyStartsNewLine(false);
    markerEditor.setEscapeAndReturnKeysConsumed(true);
    markerEditor.setSelectAllWhenFocused(true);
    markerEditor.setFont(font(12.0f));
    markerEditor.setColour(juce::TextEditor::backgroundColourId,palette::input);
    markerEditor.setColour(juce::TextEditor::textColourId,palette::text);
    markerEditor.setColour(juce::TextEditor::focusedOutlineColourId,palette::teal);
    markerEditor.onReturnKey=[this] { finishMarkerEdit(true); };
    markerEditor.onEscapeKey=[this] { finishMarkerEdit(false); };
    markerEditor.onFocusLost=[this] {
        if (markerEdit!=MarkerEdit::none) finishMarkerEdit(true);
    };
    horizontalScroll.setAutoHide(false);
    horizontalScroll.addListener(this);
    for (juce::Component* item : std::initializer_list<juce::Component*>{
         &gridToggle,&gridDivision,&snapToggle,&snapDivision}) addAndMakeVisible(item);
    gridToggle.setName("timelineGridVisibility");
    gridDivision.setName("timelineGridDivision");
    snapToggle.setName("timelineSnapToggle");
    snapDivision.setName("timelineSnapDivision");
    for (auto* combo : {&gridDivision,&snapDivision})
        combo->addItemList({"1/4","1/8","1/16","1/32"},1);
    gridDivision.setSelectedId(3,juce::dontSendNotification);
    snapDivision.setSelectedId(3,juce::dontSendNotification);
    gridToggle.setToggleState(true,juce::dontSendNotification);
    gridToggle.setTooltip("Grid ON: show BPM grid lines without changing snap.");
    gridDivision.setTooltip("Displayed musical grid division, independent of snap.");
    snapDivision.setTooltip("Division used to snap clips, even when the grid is hidden.");
    snapToggle.setButtonText("Snap ON");
    snapToggle.setToggleState(true,juce::dontSendNotification);
    snapToggle.setTooltip("Snap ON: clips align to beats. Click to disable.");
    gridToggle.onClick=[this] {
        gridVisible=!gridVisible;
        gridToggle.setToggleState(gridVisible,juce::dontSendNotification);
        gridToggle.setTooltip(gridVisible ?
            "Grid ON: hide BPM grid lines." : "Grid OFF: show BPM grid lines.");
        repaint();
    };
    snapToggle.onClick=[this] {
        snapEnabled=!snapEnabled;
        snapToggle.setToggleState(snapEnabled,juce::dontSendNotification);
        snapToggle.setButtonText(snapEnabled ? "Snap ON" : "Snap OFF");
        snapToggle.setTooltip(snapEnabled
            ? "Snap ON: clips align to beats. Click to disable."
            : "Snap OFF: move clips freely. Click to enable.");
        repaint();
    };
    gridDivision.onChange=[this] { repaint(); };
    snapDivision.onChange=[this] { repaint(); };
}
void Timeline::resized() {
    // JUCE normally clips children to their parent bounds, but a short
    // Timeline cannot physically fit its footer/scrollbar and ruler together.
    // Hide those controls rather than placing them over tracks or the header.
    const bool showToolbar=getHeight()>=detailedToolbarHeight;
    const bool showFooter=getHeight()>=completeTimelineHeight;
    for (auto* c:std::initializer_list<juce::Component*>{
         &gridToggle,&gridDivision,&snapToggle,&snapDivision})
        c->setVisible(showToolbar);
    horizontalScroll.setVisible(showFooter);
    horizontalScroll.setBounds(timelineStart,std::max(0,getHeight()-43),
                               std::max(0,getWidth()-timelineStart-20),12);
    gridToggle.setBounds(196,10,36,25);
    gridDivision.setBounds(239,10,80,25);
    snapToggle.setBounds(378,10,41,25);
    snapDivision.setBounds(427,10,80,25);
    syncViewport();
}
double Timeline::gridStepBeats() const {
    return std::pow(.5,std::max(0,gridDivision.getSelectedId()-1));
}
double Timeline::snapStepBeats() const {
    return std::pow(.5,std::max(0,snapDivision.getSelectedId()-1));
}
double Timeline::snapSeconds(double seconds) const {
    if (!snapEnabled || owner.selectedRow<0 || owner.selectedRow>=(int)owner.show.rows.size()) return seconds;
    const auto& tempo=owner.show.rows[(size_t)owner.selectedRow].tempo;
    const double step=snapStepBeats();
    return secondsAtBeat(tempo,std::round(beatAtSeconds(tempo,seconds)/step)*step);
}
double Timeline::fullLength() const {
    double length=10;
    if (owner.selectedRow<0 || owner.selectedRow>=(int)owner.show.rows.size()) return length;
    const auto& row=owner.show.rows[owner.selectedRow];
    for (int c=0;c<(int)row.cells.size();++c) if (owner.hasClip(owner.selectedRow,c))
        length=std::max(length,(row.cells[c].startSeconds+owner.clipDuration(owner.selectedRow,c))*1.1);
    for (const auto& marker : row.markers)
        length=std::max(length,marker.seconds*1.1);
    return length;
}
double Timeline::visibleLength() const { return dragColumn>=0 || seeking ? dragLength : fullLength()/zoom; }
void Timeline::syncViewport() {
    const int id=owner.selectedRow>=0 && owner.selectedRow<(int)owner.show.rows.size() ? owner.show.rows[owner.selectedRow].id : -1;
    if (id!=viewportRowId) {
        viewportRowId=id; zoom=1; viewStart=0; trackScroll=0; dragColumn=-1; seeking=false;
        draggingMarkerId=-1; markerEdit=MarkerEdit::none; editingMarkerId=-1;
        markerEditor.setVisible(false);
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
    const double length=visibleLength();
    if (dragColumn<0 && !seeking) viewStart=juce::jlimit(0.0,std::max(0.0,fullLength()-length),viewStart);
    horizontalScroll.setRangeLimits(0,std::max(fullLength(),viewStart+length),juce::dontSendNotification);
    horizontalScroll.setCurrentRange(viewStart,length,juce::dontSendNotification);
}
void Timeline::scrollBarMoved(juce::ScrollBar*,double start) {
    syncViewport();
    if (dragColumn>=0 || seeking) return;
    viewStart=juce::jlimit(0.0,std::max(0.0,fullLength()-visibleLength()),start); syncViewport(); repaint();
}
void Timeline::changeZoom(double factor,double anchor) {
    syncViewport();
    if (dragColumn>=0 || seeking) return;
    anchor=juce::jlimit(0.0,1.0,anchor);
    const double point=viewStart+anchor*visibleLength();
    zoom=juce::jlimit(1.0,32.0,zoom*factor);
    viewStart=point-anchor*visibleLength(); syncViewport(); repaint();
}
void Timeline::paint(juce::Graphics& g) {
    // All ruler, track and footer artwork belongs strictly to this component.
    // Explicitly clip it even during repeated splitter drags and repainting.
    g.reduceClipRegion(getLocalBounds());
    syncViewport();g.fillAll(palette::panel);
    // Flat ruler chassis: colour belongs to real tracks and playback, not
    // an ornamental left-edge accent or a generic dashboard toolbar.
    g.setColour(palette::toolbar.withAlpha(.73f));
    g.fillRect(0,0,getWidth(),45);
    if (owner.selectedRow<0 || owner.selectedRow>=(int)owner.show.rows.size()) return;
    const auto& row=owner.show.rows[owner.selectedRow];
    label(g,"TIMELINE",{49,11,88,23},14,palette::text,true); // fold icon at x=12
    if (getHeight()>=detailedToolbarHeight) {
        label(g,"GRID",{143,11,52,23},11,palette::muted,true);
        label(g,"SNAP",{337,11,35,23},11,palette::muted,true);
    }
    if (getHeight()>=detailedToolbarHeight) for (int i=0;i<3;++i) {
        const auto box=zoomButton(getWidth(),i);
        // These are genuine native Timeline mouse targets. Draw a plain
        // technical toolbar control, not another filled decorative pill.
        g.setColour(palette::input);
        g.fillRect(box);
        g.setColour(palette::line.withAlpha(.77f));
        g.drawRect(box,1);
        label(g,i==0 ? "-" : i==1 ? "+" : "Fit",box,12,palette::text,true,
              juce::Justification::centred);
    }
    g.setColour(palette::line); g.drawHorizontalLine(45,0,(float)getWidth());
    // At minimum height even the ruler is outside the supported content
    // viewport. In the intermediate state show the ruler but not overlapping
    // playback lanes, scrollbar or bottom status/player caption.
    if (getHeight()<detailedToolbarHeight) return;
    const double length=visibleLength(); const int area=std::max(1,getWidth()-timelineStart-20), bottom=getHeight()-48;
    const int audioTracks=(int)owner.show.columns.size();
    trackScroll=juce::jlimit(0,std::max(0,audioTracks*trackHeight-std::max(1,bottom-tracksTop)),trackScroll);
    for (int i=0;i<=4;++i) {
        const int x=timelineStart+area*i/4;
        label(g,timeText(viewStart+length*i/4),{x-10,53,75,20},12,palette::muted);
        // A short ruler has no track area below y=76; avoid a reversed or
        // negative-height vertical line on 120-124px Timeline windows.
        if (bottom>76) {
            g.setColour(palette::line.withAlpha(.8f));
            g.drawVerticalLine(x,(float)tracksTop,(float)bottom);
        }
    }
    // Dedicated marker lane; visible even when the Timeline is too short
    // to fit media tracks, so resizing cannot silently hide marker edits.
    g.setColour(palette::input);
    g.fillRect(timelineStart,markerLaneTop,area,markerLaneBottom-markerLaneTop);
    g.setColour(palette::line);
    g.drawHorizontalLine(markerLaneBottom,0,(float)getWidth());
    label(g,"MARKERS",{16,markerLaneTop+3,115,18},11,palette::muted,true);
    g.saveState();
    g.reduceClipRegion(timelineStart,markerLaneTop,area,markerLaneBottom-markerLaneTop);
    for (const auto& m : row.markers) {
        const double pos=(m.id==draggingMarkerId && row.id==draggingMarkerRowId)
            ? previewMarkerSeconds : m.seconds;
        const int x=timelineStart+(int)std::round(area*(pos-viewStart)/length);
        if (x<timelineStart-10 || x>timelineStart+area+10) continue;
        const auto ink=m.changesTempo ? palette::teal : palette::selection;
        g.setColour(ink);
        if (m.changesTempo) {
            juce::Path triangle;
            triangle.addTriangle((float)x-5,83.0f,(float)x+5,83.0f,(float)x,92.0f);
            g.fillPath(triangle);
        } else g.fillEllipse((float)x-4,84.0f,8.0f,8.0f);
        label(g,m.name+(m.changesTempo ? "  "+juce::String(m.bpm,1)+" BPM" : ""),
              {x+8,markerLaneTop+4,170,17},11,palette::text);
    }
    g.restoreState();
    if (getHeight()<completeTimelineHeight) return;
    // Musical grid follows secondsAtBeat across all tempo changes, but its
    // visibility/resolution never changes how clip dragging is quantized.
    if (gridVisible) {
        const double startBeat=beatAtSeconds(row.tempo,viewStart);
        const double endBeat=beatAtSeconds(row.tempo,viewStart+length);
        const double subdivision=gridStepBeats();
        const int skip=std::max(1,(int)std::ceil((endBeat-startBeat)/subdivision/250.0));
        const double renderStep=subdivision*skip;
        const int firstGrid=std::max(0,(int)std::ceil(startBeat/renderStep));
        const int lastGrid=(int)std::floor(endBeat/renderStep);
        g.saveState();
        g.reduceClipRegion(timelineStart,tracksTop,area,std::max(0,bottom-tracksTop));
        for (int i=firstGrid;i<=lastGrid && i<firstGrid+260;++i) {
            const double beat=i*renderStep;
            const double seconds=secondsAtBeat(row.tempo,beat);
            const int x=timelineStart+(int)std::round(area*(seconds-viewStart)/length);
            const bool bar=std::abs(beat/4-std::round(beat/4))<.0001;
            const bool quarter=std::abs(beat-std::round(beat))<.0001;
            // Grid is a timing reference, NOT a playing-state signal.
            // Neutral graduations let the amber playhead and type rails
            // stand out, including at 125–150% Windows DPI.
            g.setColour(palette::muted.withAlpha(bar ? .26f :
                                                 quarter ? .16f : .075f));
            g.drawVerticalLine(x,(float)tracksTop,(float)bottom);
        }
        g.restoreState();
    }
    int track=0, loaded=0;
    g.saveState(); g.reduceClipRegion(0,tracksTop,getWidth(),std::max(0,bottom-tracksTop));
    for (int c=0;c<(int)row.cells.size();++c) {
        const int y=tracksTop+track++*trackHeight-trackScroll;
        const bool present=owner.hasClip(owner.selectedRow,c); if (present) ++loaded;
        if (y+32<tracksTop || y>=bottom) continue;
        // Alternating console lanes and an unambiguous selected marker.
        // Never colour-wash the entire track: controls and time markers
        // need contrast against a stable neutral field.
        g.setColour(c==owner.selectedColumn
            ? palette::raised.withAlpha(.30f)
            : palette::text.withAlpha(c%2==0 ? .025f : .008f));
        g.fillRect(0,y,getWidth(),trackHeight-1);
        if (c==owner.selectedColumn) {
            g.setColour(palette::selection);
            g.fillRect(0,y,2,trackHeight-1);
        }
        const auto& name=owner.show.columns[c].name;
        label(g,name.isEmpty() ? (owner.show.columns[c].type==ColumnType::midi ? "MIDI " :
            owner.show.columns[c].type==ColumnType::control ? "Control " :
            owner.show.columns[c].type==ColumnType::osc ? "OSC " : "Audio ")+juce::String(c+1) : name,{16,y,115,30},13,c==owner.selectedColumn ? palette::text : palette::muted);
        if (owner.show.columns[c].type==ColumnType::control) {
            const auto& column=owner.show.columns[(size_t)c];
            const auto& cell=row.cells[(size_t)c];
            const ControlSet* set=column.controlLocked ? &column.controlDefaultSet :
                (cell.controlSet ? &*cell.controlSet : nullptr);
            g.setColour(palette::input);
            g.fillRect(timelineStart,y,area,31);
            g.setColour(palette::type((int)ColumnType::control).withAlpha(.83f));
            g.fillRect(timelineStart,y,3,31);
            const auto controlLabel=column.controlLocked
                ? "LOCKED DEFAULT / ALL CUES" :
                  set && !set->bindings.empty() ?
                  juce::String((int)set->bindings.size())+" control mappings" :
                  "No Control Set";
            label(g,controlLabel,
                  {timelineStart+10,y+5,std::max(1,area-18),21},
                  12,palette::muted,true);
            continue;
        }
        if (!present) continue;
        const auto& cell=row.cells[c]; const double seconds=owner.clipDuration(owner.selectedRow,c);
        const auto colour=palette::type((int)owner.show.columns[c].type);
        const float x=(float)(timelineStart+area*(cell.startSeconds-viewStart)/length), width=std::max(4.0f,(float)(area*seconds/length));
        g.saveState(); g.reduceClipRegion(timelineStart,tracksTop,area,std::max(0,bottom-tracksTop));
        const auto clipBounds=juce::Rectangle<float>(x,(float)y,width,31.f);
        // The fill follows each clip's existing type colour, rather than
        // replacing it with the orange selection colour. Legacy retains its
        // historical neutral body. Hover is a paint-only state.
        const bool chosen=c==owner.selectedColumn;
        const bool hovered=c==hoveredClipColumn;
        const auto surface=palette::activeAppearance==palette::Appearance::legacy ?
            palette::input : palette::input.interpolatedWith(
                colour,chosen ? .46f : hovered ? .43f : .37f);
        const auto contentInk=palette::activeAppearance==palette::Appearance::legacy ?
            palette::text : palette::readableInk(surface);
        g.setColour(surface);
        g.fillRect(clipBounds);
        g.setColour(colour.interpolatedWith(palette::text,.14f)
                            .withAlpha(chosen ? .97f : hovered ? .88f : .78f));
        g.drawRect(clipBounds,hovered ? 1.1f : .85f);
        if(chosen) {
            // An external accent ring marks selection, leaving the clip's
            // semantic hue intact even for an orange custom cue.
            g.setColour(palette::selection.withAlpha(.96f));
            g.drawRect(clipBounds.expanded(1.f),1.4f);
        }
        g.setColour(colour.withAlpha(.91f));
        g.fillRect(x,(float)y,3.f,31.f);
        g.setColour(contentInk.withAlpha(.84f));
        if (cell.osc) {
            for (const auto& event:cell.osc->events) {
                const double moment=cell.osc->useBeats ? secondsAtBeat(row.tempo,event.time) : event.time;
                const float dx=x+(float)(area*moment/length);
                g.fillRect(dx,(float)y+6,2.0f,19.0f);
            }
            for (const auto& automation:cell.osc->automations) {
                const double start=cell.osc->useBeats ? secondsAtBeat(row.tempo,automation.time) : automation.time;
                const double end=cell.osc->useBeats
                    ? secondsAtBeat(row.tempo,automation.time+automation.duration)
                    : automation.time+automation.duration;
                const auto from=x+(float)(area*start/length),to=x+(float)(area*end/length);
                g.setColour(colour.withAlpha(.86f));
                g.fillRect(from,(float)y+25,std::max(2.0f,to-from),3.f);
                g.setColour(colour.withAlpha(.52f));
            }
        } else if (cell.midi) {
            for (const auto& note:cell.midi->notes) {
                const double begin=secondsAtBeat(row.tempo,note.beat), end=secondsAtBeat(row.tempo,note.beat+note.length);
                g.fillRect(x+(float)(area*begin/length),(float)y+3+(127-note.pitch)*.18f,std::max(1.0f,(float)(area*(end-begin)/length)),2.0f);
            }
        } else {
        const auto& audio=owner.clips.at(cell.id)->samples;
        if (audio.getNumSamples()>0 && audio.getNumChannels()>0) for (int px=std::max(0,(int)(timelineStart-x));px<(int)width;px+=3) {
            if (x+px>getWidth()-20) break;
            const int sample=juce::jlimit(0,audio.getNumSamples()-1,(int)((double)px/width*audio.getNumSamples()));
            const float amp=std::min(1.0f,std::abs(audio.getSample(0,sample)))*24;
            g.fillRect(x+px,(float)y+15-amp/2,1.0f,std::max(1.0f,amp));
        }
        }
        if (cell.midi || cell.osc) {
            // Explicit resize grip at the right edge of MIDI and OSC clips.
            g.setColour(palette::text.withAlpha(.67f));
            g.fillRect(x+width-7,(float)y+5,3.f,21.f);
        }
        label(g,(cell.name.isEmpty() ? (cell.midi ? "MIDI" : cell.osc ? "OSC" : "AUDIO") : cell.name)+"  +"+juce::String(cell.startSeconds,2)+"s",{(int)x+8,y+5,std::max(1,(int)width-16),20},12,contentInk);
        g.restoreState();
    }
    g.restoreState();
    if (loaded==0)
        label(g,"Import audio or create a MIDI clip to see it here.",
              {timelineStart,tracksTop+1,getWidth()-160,32},13,palette::muted);
    if (owner.engine.monitoredRowId.load()==row.id) {
        const auto seconds=owner.engine.playheadSeconds.load(); const auto x=(float)(timelineStart+area*(seconds-viewStart)/length);
        if (x>=timelineStart && x<=getWidth()-20) {
            g.setColour(palette::selection); g.drawLine(x,72,x,(float)bottom,1.5f);
            juce::Path marker; marker.addTriangle(x-5,71,x+5,71,x,78); g.fillPath(marker);
        }
    }
    // Operator data first; lengthy usage instructions belong in tooltips.
    // The BPM ruler/grid and clip length editor retain all their functions.
    label(g,"Q"+juce::String(owner.selectedRow+1)+"  |  "+
          row.name+"  |  "+juce::String(row.tempo.front().bpm,1)+
          " BPM  |  "+juce::String(zoom,1)+"x",
          {16,getHeight()-27,getWidth()-30,20},11,palette::muted);
    const int available=std::max(1,bottom-tracksTop), total=track*trackHeight;
    if (total>available) {
        const float thumb=std::max(12.0f,(float)available*available/total);
        const float y=tracksTop+(available-thumb)*trackScroll/std::max(1,total-available);
        g.setColour(palette::muted.withAlpha(.45f)); g.fillRoundedRectangle((float)getWidth()-7,y,3,thumb,1.5f);
    }
}
void Timeline::mouseDown(const juce::MouseEvent& e) {
    syncViewport();
    if (e.mods.isRightButtonDown() && e.y>=markerLaneTop &&
        e.y<markerLaneBottom && e.x>=timelineStart && e.x<getWidth()-20) {
        if (const int id=markerHit(e.x); id>0)
            showMarkerContextMenu(id,e.getScreenPosition());
        return;
    }
    if (!e.mods.isLeftButtonDown()) return;
    owner.grabKeyboardFocus(); dragColumn=-1; seeking=false; resizingClip=false;
    draggingMarkerId=-1;
    // Compact layouts still expose the marker lane even if track editing
    // is hidden; do not block its mouse target along with the media tracks.
    const bool markerArea=getHeight()>=detailedToolbarHeight &&
        e.y>=markerLaneTop && e.y<markerLaneBottom;
    if (getHeight()<completeTimelineHeight && e.y>=45 && !markerArea) return;
    for (int i=0;i<3 && getHeight()>=detailedToolbarHeight;++i)
        if (zoomButton(getWidth(),i).contains(e.getPosition())) {
        if (i==2) { zoom=1; viewStart=0; syncViewport(); repaint(); }
        else changeZoom(i==0 ? .5 : 2,.5);
        return;
    }
    if (owner.selectedRow<0 || owner.selectedRow>=(int)owner.show.rows.size() || e.x<timelineStart || e.x>getWidth()-20) return;
    const int area=std::max(1,getWidth()-timelineStart-20);
    dragLength=visibleLength(); dragViewStart=viewStart; dragPixels=area; dragRowId=owner.show.rows[owner.selectedRow].id;
    if (e.y>=markerLaneTop && e.y<markerLaneBottom) {
        draggingMarkerId=markerHit(e.x);
        if (draggingMarkerId>0) {
            draggingMarkerRowId=dragRowId;
            const auto& markers=owner.show.rows[(size_t)owner.selectedRow].markers;
            for (const auto& marker:markers) if (marker.id==draggingMarkerId) {
                draggingMarkerSeconds=previewMarkerSeconds=marker.seconds;
                break;
            }
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        }
        return;
    }
    if (e.y>=46 && e.y<markerLaneTop) {
        seeking=true; owner.seekTimeline(juce::jlimit(0.0,86400.0,dragViewStart+(e.x-timelineStart)*dragLength/area)); return;
    }
    if (e.y<tracksTop || e.y>=getHeight()-48 || (e.y-tracksTop+trackScroll)%trackHeight>=32) return;
    const int wanted=(e.y-tracksTop+trackScroll)/trackHeight; int track=0;
    const auto& row=owner.show.rows[owner.selectedRow];
    for (int c=0;c<(int)row.cells.size();++c) {
        if (track++!=wanted) continue;
        if (owner.show.columns[c].collapsed) return;
        if (owner.show.columns[c].type==ColumnType::control) {
            owner.editControlSet(owner.selectedRow,c); return;
        }
        const auto& cell=row.cells[c]; if (!owner.hasClip(owner.selectedRow,c)) return;
        const double position=dragViewStart+(e.x-timelineStart)*dragLength/area, end=cell.startSeconds+owner.clipDuration(owner.selectedRow,c);
        const auto rowId=row.id; const auto start=cell.startSeconds;
        if (position<start || position>end || !owner.select(owner.selectedRow,c)) return;
        dragColumn=c; dragRowId=rowId; dragStartSeconds=start;
        dragStartDuration=owner.clipDuration(owner.selectedRow,c);
        const int edgeX=timelineStart+(int)std::round((end-dragViewStart)*area/dragLength);
        resizingClip=(cell.midi || cell.osc) && std::abs(e.x-edgeX)<=10;
        if (!resizingClip) {
            if (owner.show.columns[c].type==ColumnType::midi) owner.editMidiClip();
            else if (owner.show.columns[c].type==ColumnType::osc) owner.editOscClip();
        }
        setMouseCursor(juce::MouseCursor::DraggingHandCursor); return;
    }
}
void Timeline::mouseDoubleClick(const juce::MouseEvent& e) {
    if (!e.mods.isLeftButtonDown()) return;
    syncViewport();
    if (owner.selectedRow<0 || owner.selectedRow>=(int)owner.show.rows.size()
        || e.x<timelineStart || e.x>getWidth()-20 || e.y<markerLaneTop) return;
    if (e.y>=markerLaneBottom && e.y>=getHeight()-48) return;
    if (e.y<markerLaneBottom) {
        if (const int id=markerHit(e.x); id>0) {
            beginMarkerEdit(id,MarkerEdit::name);
        } else {
            const int rowId=owner.show.rows[(size_t)owner.selectedRow].id;
            const int area=std::max(1,getWidth()-timelineStart-20);
            const auto seconds=snapSeconds(juce::jlimit(0.0,86400.0,
                viewStart+(e.x-timelineStart)*visibleLength()/area));
            int createdId=0;
            if (mutateMarkers([&](Row& row) {
                    if (row.id!=rowId || row.markers.size()>=2048) return false;
                    for (const auto& m:row.markers) createdId=std::max(createdId,m.id);
                    ++createdId;
                    row.markers.push_back({createdId,"Marker "+juce::String(createdId),
                        seconds,false,row.tempo.front().bpm,4,4});
                    return true;
                },false,"Generic Marker added.")) beginMarkerEdit(createdId,MarkerEdit::name);
        }
        return;
    }
    if (e.y<tracksTop || (e.y-tracksTop+trackScroll)%trackHeight>=32) return;
    const int wanted=(e.y-tracksTop+trackScroll)/trackHeight;
    int track=0;
    const auto& row=owner.show.rows[(size_t)owner.selectedRow];
    const int area=std::max(1,getWidth()-timelineStart-20);
    const double position=viewStart+(e.x-timelineStart)*visibleLength()/area;
    for (int c=0;c<(int)row.cells.size();++c) {
        if (track++!=wanted) continue;
        if (owner.show.columns[c].collapsed) return;
        const auto& cell=row.cells[(size_t)c];
        if (owner.show.columns[c].type==ColumnType::control) {
            owner.editControlSet(owner.selectedRow,c); return;
        }
        if ((cell.midi || cell.osc)
            && position>=cell.startSeconds && position<=cell.startSeconds+owner.clipDuration(owner.selectedRow,c)
            && owner.select(owner.selectedRow,c)) {
            if (cell.midi) owner.editMidiClip(); else owner.editOscClip();
        }
        return;
    }
}
void Timeline::mouseDrag(const juce::MouseEvent& e) {
    syncViewport();
    if (owner.selectedRow<0 || owner.selectedRow>=(int)owner.show.rows.size() || owner.show.rows[owner.selectedRow].id!=dragRowId) return;
    if (draggingMarkerId>0 && draggingMarkerRowId==dragRowId) {
        previewMarkerSeconds=snapSeconds(juce::jlimit(0.0,86400.0,
            draggingMarkerSeconds+e.getDistanceFromDragStartX()*dragLength/dragPixels));
        repaint();
        return;
    }
    if (seeking) owner.seekTimeline(juce::jlimit(0.0,86400.0,dragViewStart+juce::jlimit(0.0,dragPixels,(double)e.x-timelineStart)*dragLength/dragPixels));
    else if (dragColumn>=0 && dragColumn<(int)owner.show.rows[owner.selectedRow].cells.size()) {
        const double delta=e.getDistanceFromDragStartX()*dragLength/dragPixels;
        if (resizingClip) {
            const double end=snapSeconds(juce::jlimit(0.0,86400.0,dragStartSeconds+dragStartDuration+delta));
            owner.setClipDuration(dragColumn,end-dragStartSeconds);
        } else owner.setClipStart(dragColumn,juce::jlimit(0.0,86400.0,
                snapSeconds(juce::jlimit(0.0,86400.0,dragStartSeconds+delta))));
    }
    repaint();
}
void Timeline::mouseUp(const juce::MouseEvent&) {
    if (draggingMarkerId>0) {
        const int id=draggingMarkerId;
        const double destination=previewMarkerSeconds;
        const bool changedPosition=std::abs(destination-draggingMarkerSeconds)>0.000001;
        bool tempo=false;
        if (owner.selectedRow>=0 && owner.selectedRow<(int)owner.show.rows.size())
            for (const auto& m:owner.show.rows[(size_t)owner.selectedRow].markers)
                if (m.id==id) tempo=m.changesTempo;
        draggingMarkerId=-1;
        if (changedPosition)
            mutateMarkers([id,destination](Row& row) {
                for (auto& m:row.markers) if (m.id==id) {m.seconds=destination; return true;}
                return false;
            },tempo,tempo ? "Tempo Marker moved. Playback continues." :
                            "Generic Marker moved.");
    }
    dragColumn=-1; seeking=false; resizingClip=false;
    syncViewport(); setMouseCursor(juce::MouseCursor::NormalCursor); repaint();
}

int Timeline::markerHit(int x) const {
    if (owner.selectedRow<0 || owner.selectedRow>=(int)owner.show.rows.size()) return -1;
    const int area=std::max(1,getWidth()-timelineStart-20);
    const auto& markers=owner.show.rows[(size_t)owner.selectedRow].markers;
    // Prefer the glyph over any text; clicking the visible marker *name*
    // must also select it instead of unexpectedly creating a new marker.
    int result=-1, best=11;
    for (const auto& marker:markers) {
        const int markerX=timelineStart+(int)std::round(
            area*(marker.seconds-viewStart)/visibleLength());
        const int distance=std::abs(markerX-x);
        if (distance<best) { best=distance; result=marker.id; }
    }
    if (result>0) return result;
    for (const auto& marker:markers) {
        const int markerX=timelineStart+(int)std::round(
            area*(marker.seconds-viewStart)/visibleLength());
        const int letters=(int)marker.name.length()+
            (marker.changesTempo ? 11 : 0);
        const int labelWidth=std::min(170,7*letters+5);
        if (x>=markerX+8 && x<markerX+8+labelWidth &&
            markerX>=timelineStart && markerX<=timelineStart+area)
            return marker.id;
    }
    return -1;
}
bool Timeline::mutateMarkers(const std::function<bool(Row&)>& mutation,
                             bool affectsTempo,const juce::String& labelText) {
    if (owner.selectedRow<0 || owner.selectedRow>=(int)owner.show.rows.size()
        || !owner.applyInspector()) return false;
    auto candidate=owner.show;
    auto& row=candidate.rows[(size_t)owner.selectedRow];
    importLegacyTempoMarkers(row);
    if (!mutation(row) || !rebuildTempoFromMarkers(row)) {
        owner.message("Invalid marker change: conflicting or out-of-range time.");
        return false;
    }
    if (auto result=validateShow(candidate); result.failed()) {
        owner.message(result.getErrorMessage());
        return false;
    }
    owner.show=std::move(candidate);
    // A Tempo Marker edit must never stop or rewind the LIVE transport.
    owner.changed(labelText,false,affectsTempo);
    return true;
}
void Timeline::beginMarkerEdit(int id,MarkerEdit kind) {
    if (owner.selectedRow<0 || owner.selectedRow>=(int)owner.show.rows.size()) return;
    const auto& row=owner.show.rows[(size_t)owner.selectedRow];
    for (const auto& marker:row.markers) if (marker.id==id) {
        markerEdit=kind; editingMarkerId=id; editingMarkerRowId=row.id;
        const bool bpm=kind==MarkerEdit::tempo || kind==MarkerEdit::conversion;
        double initialBpm=marker.bpm;
        if (kind==MarkerEdit::conversion) {
            initialBpm=row.tempo.front().bpm;
            for (const auto& point:row.tempo)
                if (secondsAtBeat(row.tempo,point.beat)<=marker.seconds+0.0000001)
                    initialBpm=point.bpm;
        }
        markerEditor.setInputRestrictions(bpm ? 7 : 80,
                                          bpm ? "0123456789." : "");
        markerEditor.setText(bpm ? juce::String(initialBpm,1) : marker.name,
                             juce::dontSendNotification);
        markerEditor.setTooltip(bpm ?
            "Tempo Marker BPM (20-400). Enter to save, Esc to cancel." :
            "Rename Marker. Enter to save, Esc to cancel.");
        const int area=std::max(1,getWidth()-timelineStart-20);
        const int x=timelineStart+(int)std::round(area*(marker.seconds-viewStart)/visibleLength());
        markerEditor.setBounds(juce::jlimit(timelineStart,std::max(timelineStart,getWidth()-195),
            x+7),markerLaneTop+2,175,22);
        markerEditor.setVisible(true);
        markerEditor.toFront(false);
        markerEditor.grabKeyboardFocus();
        markerEditor.selectAll();
        return;
    }
}
void Timeline::finishMarkerEdit(bool accept) {
    if (markerEdit==MarkerEdit::none) return;
    const auto mode=markerEdit;
    const int id=editingMarkerId,rowId=editingMarkerRowId;
    const auto value=markerEditor.getText().trim();
    markerEdit=MarkerEdit::none; editingMarkerId=-1; editingMarkerRowId=-1;
    markerEditor.setVisible(false);
    if (!accept || owner.selectedRow<0 || owner.selectedRow>=(int)owner.show.rows.size()
        || owner.show.rows[(size_t)owner.selectedRow].id!=rowId) return;
    const bool tempo=mode==MarkerEdit::tempo || mode==MarkerEdit::conversion;
    if (tempo) {
        int decimals=0,digits=0;
        for (auto c:value) {
            if (c=='.') ++decimals;
            else if (c>='0' && c<='9') ++digits;
            else { owner.message("Invalid BPM. Use 20-400."); return; }
        }
        const double number=value.getDoubleValue();
        if (digits==0 || decimals>1 || !std::isfinite(number) ||
            number<20 || number>400) { owner.message("BPM must be 20-400."); return; }
        mutateMarkers([id,number](Row& row) {
            for (auto& marker:row.markers) if (marker.id==id) {
                if (marker.seconds<=0) return false;
                marker.changesTempo=true;
                marker.bpm=number;
                return true;
            }
            return false;
        },true,"Tempo Marker BPM updated. Playback continues.");
    } else if (value.isNotEmpty() && value.length()<=80) {
        mutateMarkers([id,value](Row& row) {
            for (auto& marker:row.markers) if (marker.id==id) {
                marker.name=value;
                return true;
            }
            return false;
        },false,"Marker renamed.");
    }
}
void Timeline::showMarkerContextMenu(int markerId,const juce::Point<int>& screenPoint) {
    if (owner.selectedRow<0 || owner.selectedRow>=(int)owner.show.rows.size()) return;
    const auto& row=owner.show.rows[(size_t)owner.selectedRow];
    const auto it=std::find_if(row.markers.begin(),row.markers.end(),
        [markerId](const TimelineMarker& m) {return m.id==markerId;});
    if (it==row.markers.end()) return;
    const int rowId=row.id;
    const bool isTempo=it->changesTempo;
    juce::PopupMenu menu;
    menu.addItem(1,"Rename marker");
    if (isTempo) {
        menu.addItem(2,"Edit BPM");
        menu.addItem(3,"Convert to Generic Marker");
    } else menu.addItem(4,"Convert to Tempo Marker");
    menu.addSeparator();
    menu.addItem(5,"Delete marker");
    auto safe=juce::Component::SafePointer<Timeline>(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(
        {screenPoint.x,screenPoint.y,1,1}),
        [safe,rowId,markerId,isTempo](int choice) {
            if (!safe || choice==0 ||
                safe->owner.selectedRow<0 ||
                safe->owner.selectedRow>=(int)safe->owner.show.rows.size() ||
                safe->owner.show.rows[(size_t)safe->owner.selectedRow].id!=rowId)
                return;
            if (choice==1) safe->beginMarkerEdit(markerId,MarkerEdit::name);
            else if (choice==2) safe->beginMarkerEdit(markerId,MarkerEdit::tempo);
            else if (choice==4) safe->beginMarkerEdit(markerId,MarkerEdit::conversion);
            else if (choice==3)
                safe->mutateMarkers([markerId](Row& r) {
                    for (auto& marker:r.markers) if (marker.id==markerId) {
                        marker.changesTempo=false; return true;
                    }
                    return false;
                },true,"Tempo Marker converted to Generic. Playback continues.");
            else if (choice==5)
                safe->mutateMarkers([markerId](Row& r) {
                    const auto it=std::remove_if(r.markers.begin(),r.markers.end(),
                        [markerId](const TimelineMarker& m){return m.id==markerId;});
                    if (it==r.markers.end()) return false;
                    r.markers.erase(it,r.markers.end());
                    return true;
                },isTempo,isTempo ? "Tempo Marker deleted. Playback continues." :
                                  "Generic Marker deleted.");
        });
}
void Timeline::mouseMove(const juce::MouseEvent& e) {
    // Paint-only hover. Do not call select(), change focused row or edit data.
    int candidate=-1;
    if(owner.selectedRow>=0 &&
       owner.selectedRow<(int)owner.show.rows.size() &&
       e.y>=tracksTop && e.y<getHeight()-48 &&
       e.x>=timelineStart && e.x<getWidth()-20) {
        const int track=(e.y-tracksTop+trackScroll)/trackHeight;
        if((e.y-tracksTop+trackScroll)%trackHeight<31 &&
           track>=0 && track<(int)owner.show.columns.size() &&
           owner.show.columns[(size_t)track].type!=ColumnType::control &&
           owner.hasClip(owner.selectedRow,track)) {
            const auto& cell=owner.show.rows[(size_t)owner.selectedRow]
                                        .cells[(size_t)track];
            const int area=std::max(1,getWidth()-timelineStart-20);
            const double t=viewStart+
                (e.x-timelineStart)*visibleLength()/area;
            if(t>=cell.startSeconds &&
               t<=cell.startSeconds+owner.clipDuration(owner.selectedRow,track))
                candidate=track;
        }
    }
    if(candidate!=hoveredClipColumn) {
        hoveredClipColumn=candidate;
        repaint();
    }
}
void Timeline::mouseExit(const juce::MouseEvent&) {
    if(hoveredClipColumn>=0) {hoveredClipColumn=-1;repaint();}
}
void Timeline::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    syncViewport();
    if (e.mods.isCtrlDown()) {
        changeZoom(std::pow(2.0,wheel.deltaY*3),(double)(e.x-timelineStart)/std::max(1,getWidth()-timelineStart-20)); return;
    }
    const int tracks=(int)owner.show.columns.size();
    const int maxScroll=std::max(0,tracks*trackHeight-std::max(1,getHeight()-48-tracksTop));
    trackScroll=juce::jlimit(0,maxScroll,trackScroll-(int)std::round(wheel.deltaY*trackHeight*3)); repaint();
}
}

