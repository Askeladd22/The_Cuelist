#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>

namespace cuelist {

// Canonical hitbox geometry for the existing (fully functioning) MIDI and OSC
// clip editors. Painting, native controls and regression tests use one source.
// The compact MIDI profile keeps every action available when docked at 710px.
struct MidiEditorToolbarLayout {
    bool compact=false;
    juce::Rectangle<int> title,undo,redo,detach,collapse,close;
    juce::Rectangle<int> snapLabel,snap,channelLabel,channel;
    juce::Rectangle<int> velocityLabel,velocity,beatLabel,length;
    juce::Rectangle<int> remove,zoomOut,zoomIn;
    static MidiEditorToolbarLayout atWidth(int w) {
        MidiEditorToolbarLayout t;
        t.compact=w<1000;
        if(t.compact) {
            // Five independent actions share the FIRST row: Undo, Redo,
            // Detach, Collapse and Close. The latter two belong to the
            // parent dock, so MainComponent must use the same coordinates.
            t.title={16,6,185,25};
            t.undo={210,4,66,26};
            t.redo={282,4,66,26};
            t.detach={354,4,80,26};
            t.collapse={444,4,86,26};
            t.close={536,4,82,26};
            // Labels and controls are packed into the second row without
            // hiding Delete, MIDI note length, or either zoom direction.
            t.snapLabel={10,38,36,22}; t.snap={50,35,67,26};
            t.channelLabel={124,38,43,22}; t.channel={170,35,52,26};
            t.velocityLabel={230,38,47,22}; t.velocity={280,35,122,26};
            t.beatLabel={408,38,32,22}; t.length={444,35,88,26};
            t.remove={540,35,73,26};
            t.zoomOut={w-82,35,30,26}; t.zoomIn={w-44,35,30,26};
        } else {
            t.title={16,6,290,25};
            t.undo={w-185,4,74,26}; t.redo={w-269,4,74,26};
            t.detach={w-96,4,86,26};
            t.snapLabel={16,38,40,22}; t.snap={54,35,80,26};
            t.channelLabel={142,38,48,22}; t.channel={190,35,67,26};
            t.velocityLabel={270,38,50,22}; t.velocity={325,35,166,26};
            t.beatLabel={504,38,42,22}; t.length={546,35,96,26};
            t.remove={std::max(650,w-333),35,89,26};
            t.zoomOut={w-90,35,32,26}; t.zoomIn={w-50,35,32,26};
        }
        return t;
    }
};

struct OscEditorToolbarLayout {
    bool compact=false;
    juce::Rectangle<int> add,remove,duplicate,grid,snap,resolution;
    juce::Rectangle<int> zoomOut,zoomIn,zoomFit,unit,detach,undo,redo;
    static OscEditorToolbarLayout atWidth(int w) {
        OscEditorToolbarLayout t;
        t.compact=w<950;
        t.add={13,7,35,26};
        t.remove={53,7,70,26};
        t.duplicate={128,7,83,26};
        t.grid={219,7,69,26};
        t.snap={291,7,60,26};
        t.resolution={353,8,78,24};
        t.unit={w-228,7,110,26};
        t.detach={w-105,7,95,27};
        t.zoomOut={t.compact ? 13 : 435,t.compact ? 39 : 7,29,26};
        t.zoomIn={t.compact ? 46 : 468,t.compact ? 39 : 7,29,26};
        t.zoomFit={t.compact ? 79 : 501,t.compact ? 39 : 7,30,26};
        t.undo={t.compact ? 116 : 540,t.compact ? 39 : 7,30,26};
        t.redo={t.compact ? 151 : 575,t.compact ? 39 : 7,30,26};
        return t;
    }
};
} // namespace cuelist
