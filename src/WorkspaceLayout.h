#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "CueGridGeometry.h"

namespace cuelist {

// Input facts, not UI components: the calculation does not mutate state or
// access a renderer, an audio engine or an operating-system window.
struct WorkspaceLayoutInput {
    int width=1440;
    int height=900;
    int activeDockHeight=0;
    int columnEndX=467;   // grid.columnX(number of columns)
    int rowCount=1;
    int rowHeaderHeight=CueGridGeometry::headerHeight;
    int rowHeight=CueGridGeometry::rowHeight;
    int pinnedMetricsWidth=CueGridGeometry::pinnedMetricsWidth;
    int pinnedHeaderWidth=CueGridGeometry::nameWidth;
    int timelineHeightAdjustment=0; // user drag, positive enlarges Timeline
    bool inspectorVisible=true; // collapsed Inspector leaves a 26px reopen rail
    bool timelineVisible=true; // independent View toggle; no persisted Show change
};

// N4.2: one geometry contract for native Inspector labels and widgets.
// All rectangles use MainComponent coordinates; no paint-only affine
// transformation or second independent component scaling is required.
struct InspectorContentLayout {
    juce::Rectangle<int> inspectorTitle, cueIdentity, cellType;
    juce::Rectangle<int> cueSection, cueNameCaption, cueNameField;
    juce::Rectangle<int> bpmCaption, bpmField, rowLoop;
    juce::Rectangle<int> cellSection, selectedColumn, cellNameCaption;
    juce::Rectangle<int> cellNameField, gainCaption, gainField, dbSuffix;
    juce::Rectangle<int> followToggle, loopToggle, editAction, importAction;
    juce::Rectangle<int> oscDestinationHint, notesTitle, notesField;
    int cueDividerY=0, cellDividerY=0, notesDividerY=0;
};
// R2: the live transport status is a read-only sidebar surface.
// Pixel bounds share one immutable, testable geometry source; GO/PANIC remain
// real controls with their original hitboxes and callbacks.
struct SidebarOperatorLayout {
    int recentBottom=0;
    juce::Rectangle<int> card;
    juce::Rectangle<int> liveHeading,liveLocator,liveName;
    juce::Rectangle<int> nextHeading,nextValue;
    juce::Rectangle<int> viewHeading,viewLocator,viewName;
    int dividerY=0;
};
struct WorkspaceLayout {
    int editorX=0;
    int inspectorLeft=0;
    int inspectorInnerWidth=236;
    int dockY=0;
    int effectiveDockHeight=0; // the visible dock size after fitting the window
    int effectiveTimelineAdjustment=0;
    float inspectorScale=1.0f;

    juce::Rectangle<int> viewport;
    juce::Rectangle<int> utilities;
    juce::Rectangle<int> timeline;
    juce::Rectangle<int> dockEditor;
    juce::Rectangle<int> timelineResize;
    juce::Rectangle<int> dockResize;
    juce::Rectangle<int> dockCollapse;
    juce::Rectangle<int> dockClose;
    juce::Rectangle<int> metrics;
    juce::Rectangle<int> cueHeaders;
    juce::Rectangle<int> inspectorPanel;

    // The Inspector cards are in *unscaled* painting coordinates; the same
    // vertical transformation is applied to labels and component bounds.
    juce::Rectangle<int> inspectorCueCard;
    juce::Rectangle<int> inspectorCellCard;
    InspectorContentLayout inspector;
    SidebarOperatorLayout sidebarOperator;
    juce::Point<int> gridSize;

    juce::AffineTransform inspectorTransform() const noexcept;
    juce::Rectangle<int> scaleInspectorControl(juce::Rectangle<int>) const noexcept;
};

// Keep the approved 0.9 geometry when panels fit. On compact windows, fit
// the dock without concealing the minimum timeline lane or bottom status area.
// Input dock height is a preference (0 / 36 / dockHeight): the returned
// effectiveDockHeight is the amount actually visible on this window.
WorkspaceLayout calculateWorkspaceLayout(const WorkspaceLayoutInput& input);

} // namespace cuelist
