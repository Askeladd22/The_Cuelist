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
            // First row: title and history, then the three compact window
            // controls at the RIGHT edge (Collapse / Detach / Close).
            // MainComponent owns Collapse and Close; PianoRoll owns Detach.
            t.title={16,6,185,25};
            t.undo={210,4,28,26};
            t.redo={244,4,28,26};
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
            t.undo={312,4,28,26}; t.redo={346,4,28,26};
            t.snapLabel={16,38,40,22}; t.snap={54,35,80,26};
            t.channelLabel={142,38,48,22}; t.channel={190,35,67,26};
            t.velocityLabel={270,38,50,22}; t.velocity={325,35,166,26};
            t.beatLabel={504,38,42,22}; t.length={546,35,96,26};
            t.remove={std::max(650,w-333),35,89,26};
            t.zoomOut={w-90,35,32,26}; t.zoomIn={w-50,35,32,26};
        }
        // Consistent 28×26 hitboxes, 4 px gaps, 12 px right margin.
        // Closes to the right of Detach, and Collapse remains next to it.
        t.collapse={w-104,4,28,26};
        t.detach={w-72,4,28,26};
        t.close={w-40,4,28,26};
        return t;
    }
};

struct OscEditorToolbarLayout {
    bool compact=false;
    juce::Rectangle<int> title,add,remove,duplicate,grid,snap,resolution;
    juce::Rectangle<int> zoomOut,zoomIn,zoomFit,unit,detach,collapse,close,undo,redo;
    static OscEditorToolbarLayout atWidth(int w) {
        OscEditorToolbarLayout t;
        t.compact=w<950;
        // Same two-row hierarchy and 28x26 top-right window controls
        // as the MIDI editor. No OSC command is dropped or covered.
        t.title={16,6,230,25};
        t.undo={256,4,28,26}; t.redo={290,4,28,26};
        t.collapse={w-104,4,28,26};
        t.detach={w-72,4,28,26};
        t.close={w-40,4,28,26};
        t.add={13,38,32,26};
        t.remove={50,38,64,26};
        t.duplicate={119,38,77,26};
        t.grid={202,38,58,26};
        t.snap={265,38,56,26};
        t.resolution={326,38,75,26};
        t.unit={w-267,38,100,26};
        t.zoomOut={w-159,38,28,26};
        t.zoomIn={w-127,38,28,26};
        t.zoomFit={w-95,38,39,26};
        return t;
    }
};
} // namespace cuelist
