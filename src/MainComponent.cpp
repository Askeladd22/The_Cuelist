#include "MainComponent.h"
#include "HeaderTelemetry.h"
#include "OperatorReportDialog.h"
#include "HelpGuide.h"
#include "MidiSessionPolicy.h"
#include "ProjectSafety.h"
#include "ProjectMediaLoader.h"
#include "LtcGenerator.h"
#include "DeviceIdentity.h"
#include "OperatorTextPrompt.h"
#include "PianoRoll.h"
#include "EditorToolbarLayout.h"
#include "OscEditor.h"
#include "MidiClip.h"
#include "BinaryData.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <cstdlib>
#if JUCE_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace cuelist {
namespace {
// Shared warning for both startup and appearance changes. A neutral editing
// button still needs to disclose that importing can interrupt live playback.
constexpr const char* audioImportTooltip=
    "Mono/stereo WAV and MP3. Importing stops all playback.";
juce::String typeName(ColumnType t) { return t==ColumnType::audio ? "AUDIO" : t==ColumnType::midi ? "MIDI" : t==ColumnType::osc ? "OSC" : "CONTROL"; }
juce::String duration(double seconds) {
    int s=(int)seconds; return juce::String(s/60).paddedLeft('0',2)+":"+juce::String(s%60).paddedLeft('0',2);
}
}

MainComponent::MainComponent(bool startAudio) {
    setLookAndFeel(&theme); setWantsKeyboardFocus(true); setSize(1440,900);
    for (juce::Component* c : std::initializer_list<juce::Component*>{&openButton,&saveButton,&newButton,&addRowButton,&addColumnButton,
         &goButton,&panicButton,&deviceButton,&controlButton,&collapseAllButton,&inspectorSideTab,
         &cuelistOverflowButton,&addCuelistButton,&cuelistMoveLeft,&cuelistMoveRight,&undoButton,&redoButton,&globalMidiLearn,&globalOscLearn,&quickControlTargets,&quickLearnMapSelector,&cancelQuickLearn,&goLearnHotspot,&panicLearnHotspot,&helpButton,&dockResize,&timelineResize,&importButton,&midiEditButton,&oscEditButton,&playCellButton,&stopCellButton,&clearButton,&applyButton,
         &cellTab,&rowTab,&rowGain,&rowBehaviour,&rowMute,&rowLoop,&rowName,&cellName,&bpm,&cueNotes,&behaviour,&follow,&loop,&cellAutoStop,&gainDb,&viewport,&timeline,&timelineCollapseButton,&utilities,
         &clipCollapseButton,&clipCloseButton,&editorDivider,&cueHeaders,&columnHeaders,&cueMetrics,&preflightButton,&diagnosticsButton,&preferencesButton,&projectNameLabel,&inlineNameEditor}) addAndMakeVisible(c);
    // Static native accessibility metadata. Do not make the entire painted
    // column strip a single focusable button or steal Space from global GO.
    columnHeaders.setTitle("Cue column headers");
    columnHeaders.setDescription("Select a column; Shift+F10 shows non-modal keyboard actions. Escape PANIC; Space GO.");
    for (juce::TextButton* b : std::initializer_list<juce::TextButton*>{&openButton,&saveButton,&newButton,&addRowButton,&addColumnButton,&goButton,&panicButton,&deviceButton,&controlButton,&collapseAllButton,&inspectorSideTab,
                    &cuelistOverflowButton,&addCuelistButton,&cuelistMoveLeft,&cuelistMoveRight,&undoButton,&redoButton,&globalMidiLearn,&globalOscLearn,&helpButton,
                    &importButton,&midiEditButton,&oscEditButton,&playCellButton,&stopCellButton,&clearButton,&applyButton,
                    &preflightButton,&diagnosticsButton,&preferencesButton}) b->setWantsKeyboardFocus(false);
    // Native V2 menus reuse the original operator handlers and shortcuts.
    for (int i=0;i<=OperatorNavigationLayout::overflowIndex;++i) {
        operatorMenus[(size_t)i]=std::make_unique<OperatorNavigationButton>(
            OperatorNavigationLayout::title(i));
        auto& button=*operatorMenus[(size_t)i];
        button.setName("operatorMenu"+juce::String(
            OperatorNavigationLayout::title(i)));
        button.setTooltip("Show "+button.getButtonText()+" actions");
        button.onClick=[this,i] { showOperatorMenu(i); };
        addAndMakeVisible(button);
    }
    viewport.setViewedComponent(&grid,false); viewport.setScrollBarsShown(true,true);
    // Keep drag scrolling available, but place the vertical bar at the far
    // left edge. The right edge must meet BPM/Auto Stop without a dark gutter.
    viewport.setScrollBarThickness(9);
    viewport.setScrollBarPosition(false,true);
    // Immediately redraw pinned headers on wheel, scrollbar drag, resize or
    // keyboard scrolling. The grid body remains the only scrolled component.
    viewport.onScroll=[this] {
        // A pinned transport hover belongs to the row under the pointer;
        // scrolling must not leave its accent on a different cue.
        grid.clearHeaderHover();
        cueHeaders.setTooltip({});
        cueHeaders.repaint();
        columnHeaders.repaint();
        cueMetrics.repaint();
        cueMetrics.syncBpmEditor();
        utilities.syncScroll(viewport.getViewPositionX());
        positionInlineNameEditor();
    };
    goButton.setColour(juce::TextButton::buttonColourId,palette::go);
    // White on the new vivid green falls below WCAG 4.5:1; use near-black
    // on green action fills, retaining white on the terracotta PANIC fill.
    goButton.setColour(juce::TextButton::textColourOffId,palette::background);
    panicButton.setColour(juce::TextButton::buttonColourId,palette::panic);
    panicButton.setColour(juce::TextButton::textColourOffId,palette::background);
    // Import is an editing action, never a transport GO. It can stop
    // playback (see tooltip), but must not look like the green GO action.
    importButton.setName("importAudioMedia");
    behaviour.addItemList({"Retrigger","Pause / Restart","Pause / Resume","Continue"},1);
    rowGain.setSliderStyle(juce::Slider::LinearHorizontal);
    rowGain.setTextBoxStyle(juce::Slider::TextBoxRight,false,72,24);
    rowGain.setRange(-60,12,0.1); rowGain.setNumDecimalPlacesToDisplay(1);
    rowGain.setTextValueSuffix(" dB"); rowGain.setDoubleClickReturnValue(true,0);
    rowGain.setName("rowGainDb");
    // The Inspector edits the same linear Cell gain as before, but no longer
    // exposes an additional fader beside the dedicated audio strip faders.
    gainDb.setName("cellGainDb");
    gainDb.setInputRestrictions(8,"0123456789.-");
    gainDb.setSelectAllWhenFocused(true);
    gainDb.setJustification(juce::Justification::centredRight);
    gainDb.setFont(font(13.5f));
    gainDb.setTooltip("Cell Gain (dB). Enter a value from -60.0 to +12.0; press Enter to apply.");
    gainDb.setEscapeAndReturnKeysConsumed(false);
    gainDb.onReturnKey=[this] { if (applyInspector()) grabKeyboardFocus(); };
    gainDb.onFocusLost=[this] { if (gainDb.isVisible() && inspectorDiffers()) applyInspector(); };
    masterSlider.setName("masterAudioDb");
    masterSlider.setTooltip("MASTER AUDIO: PCM output fader and L/R meter. Double-click: 0 dB.");
    utilities.addAndMakeVisible(masterSlider);
    masterSlider.onGainChanged=[this](float db) {
        setMasterAudio(GainMeterFader::toLinear(db));
    };
    configureMixerPan(masterPan);
    masterPan.setName("masterPan");
    masterPan.setTooltip("Overall PCM output stereo balance. Double-click: center.");
    utilities.addAndMakeVisible(masterPan);
    masterPan.onValueChange=[this] { setMasterPan((float)masterPan.getValue()); };
    masterMute.setName("masterMute");
    masterMute.setTooltip("Mute the PCM master without stopping clips or MIDI/OSC messages.");
    masterMute.setWantsKeyboardFocus(false);
    utilities.addAndMakeVisible(masterMute);
    masterMute.onClick=[this] { setMasterMute(masterMute.getToggleState()); };
    masterOutput.setName("masterMainOutput");
    masterOutput.setTooltip("Stereo physical output for the Master Bus only. Direct outputs are unaffected.");
    masterOutput.onChange=[this] {
        if(masterOutput.getSelectedId()>0) setMasterOutput(masterOutput.getSelectedId()-1);
    };
    utilities.addAndMakeVisible(masterOutput);
    safetyLimiter.setName("masterSafetyLimiter");
    safetyLimiter.setTooltip("Fixed transparent -1 dBFS peak limiter (instant attack, 150 ms release). Master Bus only.");
    safetyLimiter.setToggleState(show.safetyLimiterEnabled,juce::dontSendNotification);
    safetyLimiter.setWantsKeyboardFocus(false);
    safetyLimiter.onClick=[this] {setSafetyLimiter(safetyLimiter.getToggleState());};
    utilities.addAndMakeVisible(safetyLimiter);
    rowBehaviour.addItemList({"Use cell settings","Retrigger","Pause / Restart","Pause / Resume","Continue"},1);
    cellTab.setWantsKeyboardFocus(false); rowTab.setWantsKeyboardFocus(false);
    // The Inspector displays Entire Q and Cell simultaneously; legacy tabs
    // are retained as non-visible compatibility members only.
    cellTab.setVisible(false);
    rowTab.setVisible(false);
    rowName.setSelectAllWhenFocused(true); cellName.setSelectAllWhenFocused(true); bpm.setInputRestrictions(6,"0123456789.");
    rowName.setName("cueName"); cellName.setName("cellName"); bpm.setName("cueBpm");
    cueNotes.setName("cueNotes");
    cueNotes.setMultiLine(true,true);
    // Escape must bubble to MainComponent::keyPressed so Panic works even
    // when the operator is typing in the multiline cue notepad.
    cueNotes.setEscapeAndReturnKeysConsumed(false);
    cueNotes.setReturnKeyStartsNewLine(true);
    cueNotes.setScrollbarsShown(true);
    cueNotes.setInputRestrictions(8192);
    cueNotes.setFont(font(12.5f));
    cueNotes.setTextToShowWhenEmpty("Technical notes, instructions, stage reminders...",palette::muted);
    cueNotes.setTooltip("Operational notes saved with this cue. Focus change commits your edits.");
    cueNotes.onFocusLost=[this] { if (inspectorDiffers()) applyInspector(); };
    inspectorSideTab.setName("inspectorSideToggle");
    inspectorSideTab.setTooltip("Collapse Inspector to the right and expand the cue workspace.");
    inspectorSideTab.setColour(juce::TextButton::buttonColourId,palette::raised);
    inspectorSideTab.setColour(juce::TextButton::textColourOffId,palette::teal);
    inspectorSideTab.onClick=[this] {
        // Commit all pending edits (including cue notepad) before hiding.
        // Invalid edits keep the Inspector open so the operator can fix them.
        if (inspectorVisible && !applyInspector()) return;
        inspectorVisible=!inspectorVisible;
        refreshInspector();
        resized();
        repaint();
    };
    collapseAllButton.setName("toggleAllColumns");
    // The button already says "Collapse all" or "Expand all". A hover tooltip
    // would obscure the first cue's transport controls during a live show.
    collapseAllButton.onClick=[this] {
        const bool allCollapsed=std::all_of(show.columns.begin(),show.columns.end(),
            [](const Column& col){return col.collapsed;});
        setAllColumnsCollapsed(!allCollapsed);
    };
    for (auto* editor : {&rowName,&cellName,&bpm}) editor->setEscapeAndReturnKeysConsumed(false);
    for (auto* editor : {&rowName,&cellName,&bpm})
        editor->setFont(font(14.0f));
    bpm.setSelectAllWhenFocused(true);
    for (auto* editor : {&rowName,&cellName,&bpm}) editor->onReturnKey=[this] { if (applyInspector()) grabKeyboardFocus(); };
    rowGain.onValueChange=[this] { applyInspector(); };
    for (auto* combo : {&behaviour,&rowBehaviour}) combo->onChange=[this] { applyInspector(); };
    for (auto* toggle : {&follow,&loop,&rowMute,&rowLoop}) toggle->onClick=[this] { applyInspector(); };
    cellAutoStop.setName("cellAutoStop");
    cellAutoStop.setTooltip("The cell stops on another cue's GO only if its row's Auto Stop is enabled.");
    cellAutoStop.onClick=[this] { setCellAutoStop(selectedRow,selectedColumn,cellAutoStop.getToggleState()); };
    dockResize.setName("clipEditorResizeHandle");
    dockResize.setTooltip("Drag up or down to resize the docked Clip Editor.");
    clipCollapseButton.setName("clipEditorCollapse");
    clipCollapseButton.setTooltip("Collapse the docked MIDI/OSC editor; click again to expand.");
    clipCloseButton.setName("clipEditorClose");
    clipCollapseButton.setVisible(false); clipCloseButton.setVisible(false);
    clipCollapseButton.onClick=[this] {
        clipCollapsed=!clipCollapsed;
        clipCollapseButton.setButtonText(clipCollapsed ? "Expand" : "Collapse");
        clipCollapseButton.setCollapsed(clipCollapsed);
        resized(); repaint();
    };
    clipCloseButton.onClick=[this] { closeMidiClip(); closeOscClip(); };
    clipCloseButton.setTooltip("Close the docked Clip Editor.");
    timelineCollapseButton.setName("timelineCollapse");
    timelineCollapseButton.setWantsKeyboardFocus(false);
    timelineCollapseButton.setTooltip("Collapse Timeline; click again to expand.");
    timelineCollapseButton.onClick=[this] {setTimelineVisible(!timelineVisible);};
    // Edit the displayed name directly, never through a rename dialog.
    inlineNameEditor.setName("inlineNameEditor");
    inlineNameEditor.setVisible(false);
    inlineNameEditor.setMultiLine(false);
    inlineNameEditor.setReturnKeyStartsNewLine(false);
    inlineNameEditor.setEscapeAndReturnKeysConsumed(true);
    inlineNameEditor.setSelectAllWhenFocused(true);
    inlineNameEditor.setJustification(juce::Justification::centredLeft);
    inlineNameEditor.setFont(font(16.0f));
    inlineNameEditor.setColour(juce::TextEditor::backgroundColourId,palette::input);
    inlineNameEditor.setColour(juce::TextEditor::textColourId,palette::text);
    inlineNameEditor.setColour(juce::TextEditor::outlineColourId,palette::teal);
    inlineNameEditor.setColour(juce::TextEditor::focusedOutlineColourId,palette::teal);
    inlineNameEditor.onReturnKey=[this] {
        if (commitInlineRename()) grabKeyboardFocus();
    };
    inlineNameEditor.onEscapeKey=[this] {
        // Escape cancels pending text, but must still mean PANIC during LIVE.
        // The TextEditor consumes Escape, so the root handler won't see it.
        cancelInlineRename();
        grabKeyboardFocus();
        stopAll();
    };
    inlineNameEditor.onFocusLost=[this] {
        if (inlineRenameKind!=InlineRenameKind::none && !commitInlineRename())
            cancelInlineRename();
    };
    projectNameLabel.setName("projectName");
    projectNameLabel.setEditable(false,true,false);
    projectNameLabel.onEscapePanic=[this] { stopAll(); };
    projectNameLabel.setColour(juce::Label::textColourId,palette::text);
    projectNameLabel.setFont(font(16.0f,true));
    projectNameLabel.setTooltip(
        "Double-click to rename the project. Enter saves; Esc cancels and "
        "activates PANIC. The file path is unchanged.");
    projectNameLabel.onTextChange=[this] {
        const auto name=projectNameLabel.getText().trim();
        if(name.isEmpty() || name.length()>120) {
            projectNameLabel.setText(show.name,juce::dontSendNotification);
            return;
        }
        if(name!=show.name) {
            show.name=name; dirty=true; status="Project renamed (save to persist)."; repaint();
        }
    };
    helpButton.setName("showHelpMenu");
    helpButton.setTooltip("User Guide and About The Cuelist.");
    helpButton.onClick=[this] {
        juce::PopupMenu menu;
        menu.setLookAndFeel(&theme);
        menu.addItem(1,"Guide");
        menu.addItem(2,"About The Cuelist");
        auto safe=juce::Component::SafePointer<MainComponent>(this);
        menu.showMenuAsync(
            juce::PopupMenu::Options().withTargetComponent(&helpButton),
            [safe](int choice) {
                if (!safe) return;
                if (choice==1) safe->openUserGuide();
                else if (choice==2) safe->showAbout();
            });
    };
    preferencesButton.setName("projectPreferences");
    preferencesButton.setTooltip("Settings: appearance, MIDI/OSC, audio and clock.");
    // Application setting, never serialized into or applied destructively to a Show.
    juce::PropertiesFile::Options prefOptions;
    prefOptions.applicationName="The Cuelist";
    prefOptions.filenameSuffix="settings";
    prefOptions.folderName="The Cuelist";
    appPreferences=std::make_unique<juce::PropertiesFile>(prefOptions);
    // Load the app-level appearance before initializing editor controls;
    // nothing is stored inside a .cuelist Show.
    palette::activate(appPreferences->getValue("appearanceTheme","industrialOrange")
        =="legacy" ? palette::Appearance::legacy :
                      palette::Appearance::industrialOrange);
    theme.applyColourTokens();
    // Component-specific colour IDs must be initialized AFTER restoring the
    // stored appearance. Otherwise a Legacy palette from a previous session
    // can leak into Import while the LookAndFeel uses Industrial Orange.
    importButton.setColour(juce::TextButton::buttonColourId,palette::raised);
    importButton.setColour(juce::TextButton::textColourOffId,palette::text);
    faderCeilingDb=appPreferences->getIntValue("audioFaderCeilingDb",6)==0 ? 0 : 6;
    showClock=appPreferences->getBoolValue("viewShowClock",true);
    showCpuMeter=appPreferences->getBoolValue("viewShowCpu",true);
    showRamMeter=appPreferences->getBoolValue("viewShowRam",true);
    timelineVisible=appPreferences->getBoolValue("viewTimelineVisible",true);
    recentOpenButton.setName("recentProjectOpen");
    recentMoreButton.setName("recentProjectsAll");
    recentOpenButton.setTooltip("Choose a saved Cuelist project");
    recentMoreButton.setTooltip("Show all recently opened or saved projects");
    recentOpenButton.onClick=[this] {
        withDiscardConfirmation([this] {openProject();});
    };
    recentMoreButton.onClick=[this] {showAllRecentProjects();};
    addAndMakeVisible(recentOpenButton);
    addAndMakeVisible(recentMoreButton);
    for(int i=0;i<visibleRecentProjects;++i) {
        auto& button=recentProjectButtons[(size_t)i];
        auto& detail=recentProjectDetails[(size_t)i];
        button=std::make_unique<juce::TextButton>();
        detail=std::make_unique<juce::Label>();
        button->setName("recentProject"+juce::String(i));
        button->onClick=[this,i] {openRecentProject(i);};
        detail->setName("recentProjectDetail"+juce::String(i));
        detail->setFont(font(10.0f));
        detail->setColour(juce::Label::textColourId,palette::muted);
        detail->setJustificationType(juce::Justification::centredLeft);
        detail->setInterceptsMouseClicks(false,false);
        addAndMakeVisible(*button);
        addAndMakeVisible(*detail);
    }
    restoreRecentProjects();
    masterSlider.setMaximumDb(faderCeilingDb);
    restoreAdditionalMidiInputs();
    for (const auto& midi:juce::MidiOutput::getAvailableDevices())
        engine.setMidiOutputEnabled(midi.identifier,
            appPreferences->getBoolValue("midiOutputEnabled_"+midi.identifier,true));
    // Legacy hidden button remains a compatibility entrypoint. All settings
    // and actions now use the same native operator menus and dispatcher.
    preferencesButton.onClick=[this] { showOperatorMenu(legacyPreferencesMenu); };
    newButton.onClick=[this] { withDiscardConfirmation([this] { if(projectLoader) projectLoader->cancel(); closeMidiClip(); closeOscClip(); editHistory.clearUndoHistory(); stopAll(); show=makeShow(); clips.clear(); projectFile={}; selectedRow=selectedColumn=nextCue=0;
        liveCuelist=show.activeCuelist; liveNextCue=0;
        resetActiveMapping(); refreshControlSettings();
        dirty=false; if (recoveryEnabled) {
            if (recoveryWriter) recoveryWriter->flush();
            ProjectRecovery::discard(ProjectRecovery::defaultDirectory());
            preserveDeferredRecovery=false;
        }
        engine.publish(makeLiveRenderState()); markHistoryBaseline();
        refresh(); status="New show. Import files into Audio cells."; }); };
    openButton.onClick=[this] { withDiscardConfirmation([this] { openProject(); }); };
    preflightButton.setName("showPreflight");
    diagnosticsButton.setName("showDiagnostics");
    preflightButton.setTooltip("Check show media, output devices and controller availability before a performance.");
    diagnosticsButton.setTooltip("Live engine status and counters for dropped MIDI/OSC/audio commands.");
    preflightButton.onClick=[this] { showPreflight(); };
    diagnosticsButton.onClick=[this] { showDiagnostics(); };
    saveButton.onClick=[this] { saveProject(); };
    addRowButton.onClick=[this] { addCue(); };
    addColumnButton.onClick=[this] { showColumnMenu(addColumnButton.getScreenBounds().getCentre()); };
    goButton.onClick=[this] { goNext(); }; panicButton.onClick=[this] { stopAll(); };
    deviceButton.setButtonText("Settings");
    deviceButton.onClick=[this] {preferencesButton.triggerClick();};
    // Each tab is a genuine Cuelist selector. The third tab must never
    // double as an overflow popup: the overflow appears only when needed.
    for (int i=0;i<(int)cuelistTabs.size();++i) {
        cuelistTabs[(size_t)i]=std::make_unique<CuelistTabButton>();
        cuelistCloseButtons[(size_t)i]=std::make_unique<juce::TextButton>("x");
        auto& tab=*cuelistTabs[(size_t)i];
        auto& close=*cuelistCloseButtons[(size_t)i];
        tab.setName("cuelistTab"+juce::String(i));
        close.setName("deleteCuelist"+juce::String(i));
        tab.setWantsKeyboardFocus(false);
        close.setWantsKeyboardFocus(false);
        tab.onClick=[this,i] {activateCuelist(i);};
        tab.onRename=[this,i] {renameCuelist(i);};
        tab.onDragMove=[this,i](int x) {
            cuelistDragSource=i;
            updateCuelistDropTarget(x);
        };
        tab.onDropped=[this,i](int x) {
            updateCuelistDropTarget(x);
            const int target=cuelistDragTarget;
            cuelistDragSource=cuelistDragTarget=-1;
            if (target>=0) moveCuelist(i,target);
            refreshCuelistTabs();
        };
        close.onClick=[this,i] {requestDeleteCuelist(i);};
        tab.setVisible(false);
        close.setVisible(false);
        addAndMakeVisible(tab);
        addAndMakeVisible(close);
    }
    cuelistOverflowButton.setName("cuelistOverflow");
    cuelistOverflowButton.setTooltip("Other Cuelists that do not fit in the tab bar.");
    cuelistOverflowButton.onClick=[this] {showCuelistOverflowMenu();};
    addCuelistButton.setName("newCuelist");
    addCuelistButton.setTooltip("Create another Cuelist in this show.");
    addCuelistButton.onClick=[this] {createCuelist();};
    cuelistMoveLeft.setName("cuelistMoveLeft");
    cuelistMoveRight.setName("cuelistMoveRight");
    cuelistMoveLeft.setTooltip("Move active Cuelist left (Ctrl+Shift+Left)");
    cuelistMoveRight.setTooltip("Move active Cuelist right (Ctrl+Shift+Right)");
    cuelistMoveLeft.onClick=[this] {
        moveCuelist(show.activeCuelist,show.activeCuelist-1);
    };
    cuelistMoveRight.onClick=[this] {
        moveCuelist(show.activeCuelist,show.activeCuelist+1);
    };
    undoButton.setName("showUndo");
    redoButton.setName("showRedo");
    undoButton.setTooltip("Undo the previous Show or clip edit (Ctrl+Z).");
    redoButton.setTooltip("Redo the previous Show or clip edit (Ctrl+Y / Ctrl+Shift+Z).");
    undoButton.setWantsKeyboardFocus(false);
    redoButton.setWantsKeyboardFocus(false);
    undoButton.onClick=[this] {undoEdit();};
    redoButton.onClick=[this] {redoEdit();};
    undoButton.setEnabled(false);
    redoButton.setEnabled(false);
    quickControlTargets.setVisible(false);
    quickLearnMapSelector.setVisible(false);
    quickLearnMapSelector.setName("quickLearnMapSelector");
    quickLearnMapSelector.setTooltip("Choose Default Map or a cue Control Column override");
    quickLearnMapSelector.onChange=[this] {
        const int index=quickLearnMapSelector.getSelectedId()-1;
        if (index>=0 && index<(int)quickLearnMapLocations.size()) {
            const auto location=quickLearnMapLocations[(size_t)index];
            selectQuickLearnMap(location.first,location.second);
        }
    };
    quickControlTargets.onRename=[this](int id){renameQuickLearnFunction(id);};
    quickControlTargets.onEscapePanic=[this]{stopAll();};
    cancelQuickLearn.setVisible(false);
    quickControlTargets.onPick=[this](int functionId) {chooseQuickLearnTarget(functionId);};
    cancelQuickLearn.onClick=[this] {finishQuickLearn();status="Quick Learn ended.";repaint();};
    cancelQuickLearn.setName("cancelQuickLearn");
    goLearnHotspot.onPick=[this] {chooseQuickLearnGuiTarget(ControlTarget::go);};
    panicLearnHotspot.onPick=[this] {chooseQuickLearnGuiTarget(ControlTarget::panic);};
    masterGainLearnHotspot.onPick=[this] {chooseQuickLearnGuiTarget(ControlTarget::masterGain);};
    masterPanLearnHotspot.onPick=[this] {chooseQuickLearnGuiTarget(ControlTarget::masterPan);};
    masterLimiterLearnHotspot.onPick=[this] {chooseQuickLearnGuiTarget(ControlTarget::limiterToggle);};
    utilities.addAndMakeVisible(masterGainLearnHotspot);
    utilities.addAndMakeVisible(masterPanLearnHotspot);
    utilities.addAndMakeVisible(masterLimiterLearnHotspot);
    masterGainLearnHotspot.setVisible(false);
    masterPanLearnHotspot.setVisible(false);
    masterLimiterLearnHotspot.setName("quickLearnMasterLimiter");
    masterLimiterLearnHotspot.setVisible(false);
    goLearnHotspot.setVisible(false);
    panicLearnHotspot.setVisible(false);
    globalMidiLearn.setName("globalMidiLearn");
    globalOscLearn.setName("globalOscLearn");
    globalMidiLearn.setTooltip("MIDI Learn — select a parameter, then move the MIDI controller. One-click global command.");
    globalOscLearn.setTooltip("OSC Learn — select a parameter, then send an OSC message. One-click global command.");
    globalMidiLearn.setColour(juce::TextButton::textColourOffId,palette::amber);
    globalOscLearn.setColour(juce::TextButton::textColourOffId,palette::purple);
    globalMidiLearn.onClick=[this] {beginQuickLearn(false);};
    globalOscLearn.onClick=[this] {beginQuickLearn(true);};
    controlButton.setName("controlMapping");
    controlButton.setTooltip("MIDI input, MIDI Learn, and global/per-cue mappings.");
    controlButton.onClick=[this] {openControlPanel(false);};
    importButton.onClick=[this] { importDialog(); };
    playCellButton.onClick=[this] { triggerCell(); }; stopCellButton.onClick=[this] { send(show.rows[selectedRow].cells[selectedColumn].id,false,true); };
    clearButton.onClick=[this] {
        auto& c=show.rows[selectedRow].cells[selectedColumn];
        clips.erase(c.id); c.file.clear(); c.name.clear(); c.midi.reset(); c.osc.reset(); c.click.reset();
        if (show.columns[(size_t)selectedColumn].type==ColumnType::control)
            c.controlSet.reset();
        closeOscClip();
        refreshControlSettings(); changed("Cell content removed. Original files retained.",true);
    };
    midiEditButton.onClick=[this] { editMidiClip(); };
    oscEditButton.onClick=[this] {
        if (show.columns[(size_t)selectedColumn].type==ColumnType::control)
            editControlSet(selectedRow,selectedColumn);
        else editOscClip();
    };
    applyButton.onClick=[this] { applyInspector(); };
    importButton.setTooltip(audioImportTooltip);
    applyButton.setTooltip("Apply parameters without stopping playback. Enter confirms names.");
    addColumnButton.setTooltip("Add a column.");
    addRowButton.setTooltip("Add a cue.");
    if (startAudio) { auto error=engine.initialise(); if (error.isNotEmpty()) status="Audio unavailable: "+error; }
    else status="Offline mode: audio device not opened.";
    refreshControlSettings();
    engine.publish(makeLiveRenderState()); refresh(); startTimerHz(25);
    recoveryEnabled=juce::JUCEApplication::getInstance()!=nullptr;
    if (recoveryEnabled) {
        recoveryWriter=std::make_unique<RecoveryCheckpointWriter>(ProjectRecovery::defaultDirectory());
    }
    if (recoveryEnabled && ProjectRecovery::exists(ProjectRecovery::defaultDirectory())) {
        auto safe=juce::Component::SafePointer<MainComponent>(this);
        juce::MessageManager::callAsync([safe] {if (safe) safe->offerCrashRecovery();});
    }
}
MainComponent::~MainComponent() {
    stopTimer(); engine.panic(); chooser.reset();
    // Cancel in-flight background decoding before destroying UI/engine state.
    projectLoader.reset();
    // Join the writer before touching its destination on clean shutdown.
    recoveryWriter.reset();
    if (recoveryEnabled && !preserveDeferredRecovery)
        ProjectRecovery::discard(ProjectRecovery::defaultDirectory());
    // The device selector holds a reference to engine.devices: destroy it first.
    closeMidiClip();
    closeOscClip();
    delete deviceDialog.getComponent();
    delete controlDialog.getComponent();
    viewport.setViewedComponent(nullptr,false); setLookAndFeel(nullptr);
}

void MainComponent::setAppearanceTheme(palette::Appearance id) {
    // Snapshot the old role colours before replacing a single centralized
    // token table. Explicit component-level colour overrides made by native
    // editors are remapped too, otherwise the LookAndFeel alone would leave
    // old TextEditors and ComboBoxes behind until next restart.
    const auto captures=[] {
        return std::vector<juce::Colour>{
            palette::background,palette::input,palette::panel,palette::toolbar,
            palette::raised,palette::line,palette::hover,
            palette::accent,palette::accentHover,palette::accentPressed,
            palette::text,palette::muted,palette::dimmed,palette::disabled,
            palette::teal,palette::red,palette::go,palette::panic,
            palette::warning,palette::selection,palette::selectedTab,
            palette::type(0),palette::type(1),palette::type(2),palette::type(3),
            palette::technicalInfo,palette::playing,palette::standby
        };
    };
    const auto oldColours=captures();
    palette::activate(id); // UI/message thread only; does not touch an engine
    theme.applyColourTokens();
    const auto newColours=captures();

    const auto rebind=[&](juce::Component& component,int colourId,
                          int preferredRole=-1) {
        if (!component.isColourSpecified(colourId))return;
        const auto stored=component.findColour(colourId,false);
        // Some old semantic roles share exact RGB values (e.g. Industrial
        // input/panel and Legacy copper/Audio). Without a role preference,
        // switching themes can turn an Audio label into orange accent text,
        // or a neutral TextEditor background into a panel background.
        if(preferredRole>=0 &&
           preferredRole<(int)oldColours.size() &&
           stored==oldColours[(size_t)preferredRole]) {
            if(stored!=newColours[(size_t)preferredRole])
                component.setColour(colourId,newColours[(size_t)preferredRole]);
            return;
        }
        for(size_t i=0;i<oldColours.size();++i)
            if(stored==oldColours[i]) {
                if(stored!=newColours[i])
                    component.setColour(colourId,newColours[i]);
                return;
            }
    };
    std::function<void(juce::Component&)> refreshChildren;
    refreshChildren=[&](juce::Component& component) {
        if(dynamic_cast<juce::TextButton*>(&component)) {
            for(int id:{juce::TextButton::buttonColourId,
                        juce::TextButton::buttonOnColourId})
                rebind(component,id,2); // prefer panel to coincident input
            for(int id:{juce::TextButton::textColourOffId,
                        juce::TextButton::textColourOnId})
                rebind(component,id,14); // prefer foreground accent over Audio
        } else if(dynamic_cast<juce::TextEditor*>(&component)) {
            rebind(component,juce::TextEditor::backgroundColourId,1);
            for(int id:{juce::TextEditor::textColourId,
                        juce::TextEditor::outlineColourId,
                        juce::TextEditor::focusedOutlineColourId,
                        juce::TextEditor::highlightColourId})
                rebind(component,id,14);
        } else if(dynamic_cast<juce::ComboBox*>(&component)) {
            rebind(component,juce::ComboBox::backgroundColourId,1);
            for(int id:{juce::ComboBox::textColourId,
                        juce::ComboBox::outlineColourId,
                        juce::ComboBox::arrowColourId})
                rebind(component,id,14);
        } else if(dynamic_cast<juce::Label*>(&component)) {
            rebind(component,juce::Label::textColourId,14);
            rebind(component,juce::Label::backgroundColourId,2);
        } else if(dynamic_cast<juce::Slider*>(&component)) {
            for(int id:{juce::Slider::thumbColourId,
                        juce::Slider::trackColourId,
                        juce::Slider::textBoxOutlineColourId})
                rebind(component,id,14);
            rebind(component,juce::Slider::backgroundColourId,5);
            rebind(component,juce::Slider::textBoxTextColourId,10);
        } else if(dynamic_cast<juce::ToggleButton*>(&component)) {
            rebind(component,juce::ToggleButton::textColourId,10);
            for(int id:{juce::ToggleButton::tickColourId,
                        juce::ToggleButton::tickDisabledColourId})
                rebind(component,id,14);
        } else if(dynamic_cast<juce::ScrollBar*>(&component)) {
            rebind(component,juce::ScrollBar::backgroundColourId,4);
            rebind(component,juce::ScrollBar::thumbColourId,11);
        }
        for(int i=0;i<component.getNumChildComponents();++i)
            if(auto* child=component.getChildComponent(i))
                refreshChildren(*child);
        component.repaint();
    };
    refreshChildren(*this);
    // Import has an intentional neutral editor role in both themes. Re-apply
    // it after generic token remapping to avoid collisions between semantic
    // roles that happened to use the same ARGB value in the previous theme.
    importButton.setColour(juce::TextButton::buttonColourId,palette::raised);
    importButton.setColour(juce::TextButton::textColourOffId,palette::text);
    // The explicit active-tab JUCE button colours must reflect selectedTab
    // (Legacy has a copper-dark tab; Industrial uses neutral graphite).
    refreshCuelistTabs();
    sendLookAndFeelChange();
    // Preserve the safety warning after JUCE rebuilds native button state
    // during appearance changes. Avoid relying on an earlier constructor
    // assignment that may be cleared by a LookAndFeel refresh.
    importButton.setTooltip(audioImportTooltip);
    // PianoRollEditor and OscClipEditor own independent Theme instances.
    // They do not inherit the MainComponent LookAndFeel when docked or
    // reparented into floating DocumentWindows.
    if(clipEditor)clipEditor->refreshAppearance();
    if(oscEditor)oscEditor->refreshAppearance();
    if(clipWindow) {
        clipWindow->setBackgroundColour(palette::panel);
        clipWindow->sendLookAndFeelChange();
        clipWindow->repaint();
    }
    if(oscWindow) {
        oscWindow->setBackgroundColour(palette::panel);
        oscWindow->sendLookAndFeelChange();
        oscWindow->repaint();
    }
    if(appPreferences) {
        appPreferences->setValue("appearanceTheme",
            id==palette::Appearance::legacy ? "legacy" : "industrialOrange");
        appPreferences->saveIfNeeded();
    }
    status="Appearance: "+juce::String(palette::displayName(id))+".";
    repaint();
}

void MainComponent::setAudioFaderCeilingDb(int db) {
    const int chosen=db==0 ? 0 : 6;
    if (faderCeilingDb==chosen) return;
    faderCeilingDb=chosen;
    if (appPreferences) {
        appPreferences->setValue("audioFaderCeilingDb",chosen);
        appPreferences->saveIfNeeded();
    }
    // A range change must never alter Show gain, queued transport or MIDI/OSC.
    masterSlider.setMaximumDb(chosen);
    utilities.refresh();
    status=chosen==0 ? "Audio faders: -inf / 0 dB (unity at maximum)." :
        "Audio faders: -inf / +6 dB.";
    repaint();
}

WorkspaceLayout MainComponent::workspaceLayout() const {
    const bool docked=(clipEditor && !clipWindow) || (oscEditor && !oscWindow);
    const int activeDockHeight=docked ? (clipCollapsed ? 36 : dockHeight) : 0;
    return calculateWorkspaceLayout({getWidth(),getHeight(),activeDockHeight,
        grid.columnX((int)show.columns.size()),(int)show.rows.size(),
        CueGrid::headerHeight,CueGrid::rowHeight,CueMetricsOverlay::width,
        CueGrid::nameWidth,timelineHeightAdjustment,inspectorVisible,
        timelineVisible});
}

juce::String MainComponent::liveCuelistNameForUi() const {
    if (liveCuelist<0 || liveCuelist>=(int)show.cuelists.size())
        return "LIVE unavailable";
    return show.cuelists[(size_t)liveCuelist].name;
}
juce::String MainComponent::nextLiveGoCueForUi() const {
    if (liveCuelist<0 || liveCuelist>=(int)show.cuelists.size())
        return "END";
    const auto& rows=liveCuelist==show.activeCuelist
        ? show.rows : show.cuelists[(size_t)liveCuelist].rows;
    return liveNextCue>=0 && liveNextCue<(int)rows.size()
        ? juce::String(liveNextCue+1).paddedLeft('0',2) : "END";
}
void MainComponent::paint(juce::Graphics& g) {
    const int w=getWidth(),h=getHeight();
    const auto geometry=workspaceLayout();
    g.fillAll(palette::background);
    // Two quiet header tiers: 38px operator navigation and 34px Cuelists.
    // No change to the 72px workspace boundary or any child hitbox.
    g.setColour(palette::toolbar); g.fillRect(0,0,w,38);
    g.setColour(palette::toolbar.interpolatedWith(palette::panel,.43f));
    g.fillRect(0,38,w,35);
    g.setColour(palette::panel); g.fillRect(0,73,180,h-73);
    // A collapsed MIDI/OSC editor retains a 36px docking region while
    // its editor component is hidden. Paint the reserved panel background
    // so the Timeline splitter borders a real panel, not a black stripe.
    if (geometry.dockEditor.getHeight()>0) {
        g.setColour(palette::panel);
        g.fillRect(geometry.dockEditor);
    }
    g.setColour(palette::line.withAlpha(.38f));
    g.drawHorizontalLine(37,0,(float)std::min(w,HeaderTelemetryLayout::atWidth(w).clock.getX()-10));
    g.setColour(palette::line.withAlpha(.62f));
    g.drawHorizontalLine(72,0,(float)w);
    g.drawVerticalLine(180,73,(float)h);
    // Replace the otherwise empty sidebar void with a responsive, usable
    // recent-project launcher underneath Workspace.
    const int recentTop=218,recentBottom=h-349;
    if(recentBottom>recentTop+62) {
        const auto panel=juce::Rectangle<float>(
            10.f,(float)recentTop,160.f,(float)(recentBottom-recentTop));
        g.setColour(palette::background.withAlpha(.40f));
        g.fillRoundedRectangle(panel,8.f);
        g.setColour(palette::line.withAlpha(.42f));
        g.drawRoundedRectangle(panel,8.f,.7f);
        label(g,"RECENT PROJECTS",{19,recentTop+7,142,20},
              11,palette::muted,true);
        if(recentProjects.isEmpty())
            label(g,"No saved projects yet.",{19,recentTop+82,141,34},
                  11,palette::muted);
    }
    static const auto appIcon=juce::ImageFileFormat::loadFrom(
        BinaryData::thecuelist_icon_png,BinaryData::thecuelist_icon_pngSize);
    if (appIcon.isValid())
        g.drawImageWithin(appIcon,15,7,31,31,juce::RectanglePlacement::centred);
    else for (int i=0;i<3;++i) {
        g.setColour(i==1 ? palette::teal : palette::text);
        g.fillRoundedRectangle(17.0f,12.0f+i*7,23.0f-i*4,3,1.5f);
    }
    // Product wordmark: neutral "The" plus the restrained copper CUElist
    // accent from the approved reference (no font or icon replacement).
    label(g,"The",{53,7,38,32},19,palette::text,true);
    label(g,"CUElist",{91,7,105,32},19,palette::teal,true);
    // Global Learn now belongs to the live I/O Activity surface, while
    // the unchanged top bar and telemetry remain within the 72px header.
    // A single enlarged monitoring console, responsive in X only.
    // All positions (including real Learn buttons) originate from one
    // geometry contract; the 72px global header remains unchanged.
    const auto telemetry=HeaderTelemetryLayout::atWidth(w);
    const auto monitorStrip=juce::Rectangle<float>(
        (float)telemetry.clock.getX(),5.f,
        (float)(w-telemetry.clock.getX()-10),61.f);
    g.setColour(palette::panel.withAlpha(.94f));
    g.fillRoundedRectangle(monitorStrip,6.f);
    g.setColour(palette::line.withAlpha(.49f));
    g.drawRoundedRectangle(monitorStrip,6.f,.8f);
    g.setColour(palette::line.withAlpha(.50f));
    for (const int x:{telemetry.usage.getX(),telemetry.activity.getX(),
                     telemetry.audio.getX()})
        g.drawVerticalLine(x-5,13.f,59.f);

    // Original 12h/24h system-clock preference and one-second refresh.
    const auto clockTime=juce::Time::getCurrentTime();
    const bool clock12=appPreferences &&
        appPreferences->getBoolValue("clock12Hour",false);
    const float timeSize=(float)(telemetry.clock.getWidth()>=160?35:
                                 telemetry.clock.getWidth()>=150?33:30);
    if (showClock) {
        label(g,clockTime.formatted(clock12?"%I:%M:%S":"%H:%M:%S"),
              clock12 ? telemetry.timeText : telemetry.clock.reduced(4,9),
              timeSize,palette::text,true,juce::Justification::centred);
        if (clock12)
            label(g,clockTime.formatted("%p"),telemetry.meridiemText,12,
                  palette::teal,true,juce::Justification::centredRight);
    }

    // Real measured CPU/RAM values and meters are unchanged; only their
    // typography and available horizontal room have increased.
    if (showCpuMeter) {
        label(g,"CPU",telemetry.cpuLabel,14,palette::text,true);
        label(g,HeaderTelemetryLayout::percentage(systemCpuPercent,true),
              telemetry.cpuValue,14,palette::text,true,
              juce::Justification::centredRight);
    }
    if (showRamMeter) {
        label(g,"RAM",telemetry.ramLabel,14,palette::text,true);
        label(g,HeaderTelemetryLayout::percentage(systemRamPercent,false),
              telemetry.ramValue,14,palette::text,true,
              juce::Justification::centredRight);
    }
    const auto meter=[&](juce::Rectangle<float> bounds,float percentage,
                         juce::Colour colour) {
        g.setColour(palette::line);
        g.fillRoundedRectangle(bounds,3.f);
        if (std::isfinite(percentage) && percentage>=0) {
            g.setColour(colour);
            g.fillRoundedRectangle(bounds.withWidth(
                bounds.getWidth()*juce::jlimit(0.0f,1.0f,
                                              percentage/100.0f)),3.f);
        }
    };
    if (showCpuMeter) meter(telemetry.cpuMeter,systemCpuPercent,palette::technicalInfo);
    if (showRamMeter) meter(telemetry.ramMeter,systemRamPercent,palette::technicalInfo);

    label(g,"I/O ACTIVITY",telemetry.activityHeading,11,palette::muted,true);
    const double now=juce::Time::getMillisecondCounterHiRes();
    const std::array<juce::String,4> activityNames{{
        "MIDI RX","MIDI TX","OSC RX","OSC TX"}};
    const std::array<bool,4> activityStates{{
        now<midiRxUntil,now<midiTxUntil,now<oscRxUntil,now<oscTxUntil}};
    for (int i=0;i<4;++i) {
        g.setColour(activityStates[(size_t)i] ?
                    (i<2 ? palette::type(1) : palette::type(2)) :
                    palette::line.brighter(.22f));
        g.fillEllipse(telemetry.activityDots[(size_t)i].toFloat());
        label(g,activityNames[(size_t)i],
              telemetry.activityLabels[(size_t)i],12,
              activityStates[(size_t)i] ? palette::text : palette::muted,true);
    }
    // Independent Audio engine availability, no inferred state.
    const bool audioConnected=engine.connected.load();
    label(g,"AUDIO",telemetry.audioHeading,12,palette::muted,true);
    g.setColour(audioConnected?palette::playing:palette::red);
    g.fillEllipse((float)telemetry.audioStatus.getX()-17.f,39.f,11.f,11.f);
    label(g,audioConnected?"AVAILABLE":"UNAVAILABLE",
          telemetry.audioStatus,12.5f,audioConnected?palette::playing:palette::red,true);
    // N2: one editable Show identity instead of two redundant Show/Project
    // cards. File, Settings and Control Mapping are in genuine JUCE menus.
    label(g,"WORKSPACE",{20,91,140,17},11,palette::muted,true);
    g.setColour(palette::teal.withAlpha(0.19f));
    g.fillRoundedRectangle(12,112,156,54,8);
    g.setColour(palette::teal);
    g.fillRoundedRectangle(12,119,4,40,2);
    // The project title remains the original editable JUCE Label.
    // Do not repeat cue/column counts from the Cuelist header.
    if (hasUnsavedChanges()) {
        g.setColour(palette::amber);
        g.fillEllipse(20.0f,181.0f,7.0f,7.0f);
        label(g,"UNSAVED CHANGES",{34,174,130,21},10,palette::amber,true);
    }
    g.setColour(palette::amber.withAlpha(.11f));
    g.fillRoundedRectangle(12.0f,(float)h-339,156.0f,51.0f,8.0f);
    // Keep the established 51px ON DECK panel and its adjacent GO/PANIC
    // hitboxes. The actual LIVE name remains on-screen even when its tab
    // is hidden behind the Cuelist overflow menu.
    // The numeric locator is the LIVE Cuelist's CURRENT tab position,
    // not a persistent project ID. It disambiguates long names that render
    // identically after ellipsis in the fixed 94px name field.
    label(g,"ON DECK / LIVE",{20,h-333,106,16},11,palette::amber,true);
    label(g,"L"+juce::String(liveCuelist+1).paddedLeft('0',2),
          {129,h-333,31,16},10,palette::amber,true,
          juce::Justification::centredRight);
    label(g,liveCuelistNameForUi(),{20,h-314,94,24},15,palette::text,true);
    const auto nextLiveGo=nextLiveGoCueForUi();
    label(g,nextLiveGo=="END" ? "END" : "GO "+nextLiveGo,
          {115,h-314,45,24},12,palette::text,true,
          juce::Justification::centredRight);
    // Output meter removed from the sidebar; audio peak telemetry remains
    // available in the Master and channel meters.
    // Continue the non-scrollable metrics surface beneath the mixer. This
    // rightmost band is never a hidden extension of audio/MIDI/OSC strips.
    g.setColour(palette::panel);
    g.fillRect(geometry.metrics.getX(),geometry.utilities.getY(),
               geometry.metrics.getWidth(),geometry.utilities.getHeight());
    // Continue the exact 3px accent edge from the pinned metrics down
    // through the utility area, without a doubled boundary or dark gap.
    g.setColour(palette::teal);
    g.fillRect(geometry.metrics.getX(),geometry.utilities.getY(),
               3,geometry.utilities.getHeight());
    g.setColour(palette::line);
    g.drawVerticalLine(geometry.metrics.getX()+3,(float)geometry.utilities.getY(),
                       (float)geometry.utilities.getBottom());
    g.setColour(palette::line.withAlpha(.27f));
    g.drawVerticalLine(geometry.metrics.getX()+94,
                       (float)geometry.utilities.getY(),
                       (float)geometry.utilities.getBottom());
    if (!inspectorVisible) {
        // A slim, permanently visible rail provides a full-height spatial
        // cue for reopening without stealing any BPM/Auto Stop hit targets.
        g.setColour(palette::panel);
        g.fillRect(geometry.editorX,76,w-geometry.editorX,h-123);
        g.setColour(palette::line);
        g.drawVerticalLine(geometry.editorX,76.0f,(float)h-47.0f);
    }
    if (inspectorVisible) {
    if (quickLearnMode!=QuickLearnMode::off) {
        const int x=geometry.inspectorLeft;
        // Learn uses the same flat inspector chassis. The protocol colour
        // remains meaningful state, not an extra decorative card.
        g.setColour(palette::panel);
        g.fillRect(geometry.inspectorPanel);
        g.setColour(palette::line.withAlpha(.52f));
        g.drawVerticalLine(geometry.inspectorPanel.getX(),
                           (float)geometry.inspectorPanel.getY(),
                           (float)geometry.inspectorPanel.getBottom());
        const auto learnColour=quickLearnAccent();
        label(g,quickLearnMode==QuickLearnMode::midi ? "MIDI LEARN" : "OSC LEARN",
              {x+5,98,geometry.inspectorInnerWidth-10,25},19,learnColour,true);
        const bool midiInputMissing=quickLearnMode==QuickLearnMode::midi &&
                                    !midiInput.connected();
        label(g,"Map to edit",{x+5,126,geometry.inspectorInnerWidth-10,16},11,palette::muted,true);
        label(g,quickLearnFunctionId>0 ?
              (midiInputMissing ? "MIDI input missing - select device" :
               "Selected target blinking - move your controller") :
              "Select a parameter or a named function",
              {x+5,155,geometry.inspectorInnerWidth-10,38},12,
              quickLearnFunctionId>0 ? learnColour : palette::text);
        if (quickLearnFeedback.isNotEmpty())
            label(g,quickLearnFeedback,
                  {x+5,184,geometry.inspectorInnerWidth-10,20},
                  10,palette::teal,true);
        g.setColour(palette::line.withAlpha(.55f));
        g.drawHorizontalLine(201,(float)x-6,
                              (float)x+geometry.inspectorInnerWidth+6);
        label(g,"AVAILABLE TARGETS",
              {x+4,205,geometry.inspectorInnerWidth-8,24},
              12,palette::muted,true);
    } else {
    // N4.2 — one native Inspector layout for both rendered labels and
    // actual editor bounds. No affine-shrunken labels or nested cards.
    // Group identity remains stable while selecting Audio/MIDI/OSC/Control.
    const auto& i=geometry.inspector;
    const int x=geometry.inspectorLeft;
    const int inspectorInner=geometry.inspectorInnerWidth;
    g.setColour(palette::panel);
    g.fillRect(geometry.inspectorPanel);
    g.setColour(palette::line.withAlpha(.52f));
    g.drawVerticalLine(geometry.inspectorPanel.getX(),
                       (float)geometry.inspectorPanel.getY(),
                       (float)geometry.inspectorPanel.getBottom());
    label(g,"INSPECTOR",i.inspectorTitle,12,palette::muted,true);
    const auto& col=show.columns[selectedColumn];
    label(g,"CUE "+juce::String(selectedRow+1).paddedLeft('0',2),
          i.cueIdentity,22,palette::text,true);
    label(g,typeName(col.type),i.cellType,11,
          palette::type((int)col.type),true,
          juce::Justification::centredRight);
    const auto rule=[&](int y) {
        g.setColour(palette::line.withAlpha(.55f));
        g.drawHorizontalLine(y,(float)x-6,(float)x+inspectorInner+6);
    };
    rule(i.cueDividerY);
    label(g,"CUE PARAMETERS",i.cueSection,13,palette::muted,true);
    label(g,"Cue name",i.cueNameCaption,13,palette::text,true);
    label(g,"Initial BPM",i.bpmCaption,13,palette::text,true);
    rule(i.cellDividerY);
    label(g,"CELL PARAMETERS",i.cellSection,13,palette::muted,true);
    label(g,col.name,i.selectedColumn,12,palette::text,true,
          juce::Justification::centredRight);
    label(g,"Cell name",i.cellNameCaption,13,palette::text,true);
    if (col.type==ColumnType::audio) {
        label(g,"Gain",i.gainCaption,12,palette::muted,true);
        label(g,"dB",i.dbSuffix,12,palette::muted,true);
    }
    if (col.type==ColumnType::osc)
        label(g,"UDP destination in column Utility",
              i.oscDestinationHint,10,palette::muted);
    rule(i.notesDividerY);
    label(g,"CUE NOTES",i.notesTitle,13,palette::muted,true);
    }
    }
    // Drag/drop only changes tab ordering. A narrow insertion marker between
    // tab slots mirrors the cue-row drop feedback without covering their names.
    if (cuelistDragSource>=0 && cuelistDragTarget>=0 &&
        cuelistDragSource!=cuelistDragTarget &&
        cuelistDragTarget<(int)cuelistTabs.size() &&
        cuelistTabs[(size_t)cuelistDragTarget] &&
        cuelistTabs[(size_t)cuelistDragTarget]->isVisible()) {
        const auto slot=cuelistTabs[(size_t)cuelistDragTarget]->getBounds();
        const bool after=cuelistDragSource<cuelistDragTarget;
        const float edge=(float)(after ? slot.getX()+82 : slot.getX()-2);
        g.setColour(palette::teal);
        g.fillRoundedRectangle(edge,42.f,3.f,30.f,1.5f);
    }
    label(g,status,{geometry.viewport.getX(),h-35,std::max(1,geometry.editorX-200),25},12,palette::muted);
}
void MainComponent::showCuelistOverflowMenu() {
    juce::PopupMenu menu;
    menu.setLookAndFeel(&theme);
    for (int i=0;i<(int)show.cuelists.size();++i)
        if (!cuelistTabs[(size_t)i]->isVisible())
            menu.addItem(100+i,
                         (i==liveCuelist ? juce::String("LIVE  /  ") :
                                            juce::String())+
                             show.cuelists[(size_t)i].name,true,
                         i==show.activeCuelist);

    // Off-screen tabs must remain manageable, not just selectable.
    juce::PopupMenu renameMenu;
    renameMenu.setLookAndFeel(&theme);
    for (int i=0;i<(int)show.cuelists.size();++i)
        if (!cuelistTabs[(size_t)i]->isVisible())
            renameMenu.addItem(300+i,"Rename "+show.cuelists[(size_t)i].name);
    if (renameMenu.getNumItems()>0) {
        menu.addSeparator();
        menu.addSubMenu("Rename hidden Cuelist...",renameMenu);
    }
    juce::PopupMenu deleteMenu;
    deleteMenu.setLookAndFeel(&theme);
    if (show.cuelists.size()>1)
        for (int i=0;i<(int)show.cuelists.size();++i)
            if (!cuelistTabs[(size_t)i]->isVisible())
                deleteMenu.addItem(200+i,"Delete "+show.cuelists[(size_t)i].name);
    if (deleteMenu.getNumItems()>0) {
        menu.addSeparator();
        menu.addSubMenu("Delete hidden Cuelist...",deleteMenu);
    }
    juce::PopupMenu moveLeft,moveRight;
    moveLeft.setLookAndFeel(&theme);
    moveRight.setLookAndFeel(&theme);
    for (int i=0;i<(int)show.cuelists.size();++i)
        if (!cuelistTabs[(size_t)i]->isVisible()) {
            moveLeft.addItem(400+i,show.cuelists[(size_t)i].name,i>0);
            moveRight.addItem(500+i,show.cuelists[(size_t)i].name,
                              i+1<(int)show.cuelists.size());
        }
    if (moveLeft.getNumItems()>0) {
        menu.addSeparator();
        menu.addSubMenu("Move hidden Cuelist left...",moveLeft);
        menu.addSubMenu("Move hidden Cuelist right...",moveRight);
    }
    if (menu.getNumItems()==0) return;
    auto safe=juce::Component::SafePointer<MainComponent>(this);
    menu.showMenuAsync(
        juce::PopupMenu::Options().withTargetComponent(&cuelistOverflowButton),
        [safe](int choice) {
            if (!safe) return;
            if (choice>=100 && choice<112) safe->activateCuelist(choice-100);
            if (choice>=200 && choice<212) safe->requestDeleteCuelist(choice-200);
            if (choice>=300 && choice<312) safe->renameCuelist(choice-300);
            if (choice>=400 && choice<412) safe->moveCuelist(choice-400,choice-401);
            if (choice>=500 && choice<512) safe->moveCuelist(choice-500,choice-499);
        });
}

void MainComponent::updateCuelistDropTarget(int rootX) {
    int candidate=-1,nearest=std::numeric_limits<int>::max();
    for (int i=0;i<(int)show.cuelists.size();++i) {
        auto* tab=cuelistTabs[(size_t)i].get();
        if (!tab || !tab->isVisible()) continue;
        const int distance=std::abs(rootX-tab->getBounds().getCentreX());
        if (distance<nearest) {nearest=distance;candidate=i;}
    }
    if (cuelistDragTarget!=candidate) {
        cuelistDragTarget=candidate;
        refreshCuelistTabs(); // update insertion edge, no change to sorting
        repaint();
    }
}
void MainComponent::refreshCuelistTabs() {
    // setSize() can call resized() before dynamic tab allocation in the ctor.
    if (!cuelistTabs[0]) return;

    constexpr int left=190,slot=84,buttonHeight=27;
    const int count=(int)show.cuelists.size();
    // Stop tabs before the fixed clock/CPU panels. This gives nine real tabs
    // at 1647px, three at 1180px, and overflows only when space runs out.
    // Tab overflow follows the same monitor boundary as the top menus.
    // Enlarging the telemetry must never overlay a Cuelist tab.
    const int available=std::max(150,
        HeaderTelemetryLayout::atWidth(getWidth()).clock.getX()-left-16);
    const int plusWidth=count<12 ? 32 : 0;
    const bool overflow=count*slot+plusWidth>available;
    const int overflowWidth=overflow ? 42 : 0;
    const int displayed=overflow
        ? juce::jlimit(1,count,(available-plusWidth-overflowWidth)/slot)
        : count;
    // Keep displayed indices contiguous, including the active Cuelist.
    const int first=overflow
        ? juce::jlimit(0,count-displayed,show.activeCuelist-displayed+1)
        : 0;
    for (int i=0;i<(int)cuelistTabs.size();++i) {
        auto& tab=*cuelistTabs[(size_t)i];
        auto& close=*cuelistCloseButtons[(size_t)i];
        const bool visible=i>=first && i<first+displayed && i<count;
        tab.setVisible(visible);
        close.setVisible(visible && count>1);
        if (!visible) continue;

        const int x=left+(i-first)*slot;
        const bool active=i==show.activeCuelist;
        tab.setActiveVisual(active);
        tab.setLiveVisual(i==liveCuelist);
        const auto colour=active ? palette::selectedTab : palette::panel;
        auto name=show.cuelists[(size_t)i].name;
        tab.setButtonText(name.length()>13 ? name.substring(0,11)+"..." : name);
        tab.setTooltip(name+(i==liveCuelist ? " [LIVE]" : " [VIEW]")+
                       " - click: view; GO continues on LIVE Cuelist; double-click: rename; drag: move.");
        tab.setColour(juce::TextButton::buttonColourId,colour);
        tab.setBounds(x,43,count>1 ? 64 : 81,buttonHeight);
        close.setColour(juce::TextButton::buttonColourId,colour);
        close.setColour(juce::TextButton::textColourOffId,palette::muted);
        close.setBounds(x+64,43,18,buttonHeight);
        close.setTooltip("Delete "+name+" (confirmation required).");
        close.toFront(false);
    }
    const int trailingX=left+displayed*slot;
    cuelistOverflowButton.setVisible(overflow);
    if (overflow) {
        cuelistOverflowButton.setTooltip("More Cuelists. Current LIVE: "+
                                        liveCuelistNameForUi());
        cuelistOverflowButton.setBounds(trailingX,43,38,buttonHeight);
        cuelistOverflowButton.setColour(juce::TextButton::buttonColourId,palette::raised);
    }
    cuelistMoveLeft.setEnabled(count>1 && show.activeCuelist>0);
    cuelistMoveRight.setEnabled(count>1 && show.activeCuelist<count-1);
    addCuelistButton.setVisible(count<12);
    if (count<12) addCuelistButton.setBounds(trailingX+overflowWidth,43,28,buttonHeight);
}

// N3: A single function location per operator action. The menus are
// functional, not a reduced mirror of the former Preferences popup. All
// existing MIDI, OSC, clock and fader-range options remain accessible.
void MainComponent::showOperatorMenu(int index) {
    const bool legacyPreferences=index==legacyPreferencesMenu;
    if (!legacyPreferences &&
        (index<0 || index>OperatorNavigationLayout::overflowIndex)) return;
    // A hidden compatibility button must anchor its popup to a visible control.
    // The same rule works when Show is moved into the narrow-layout overflow.
    auto* target=legacyPreferences
        ? (operatorMenus[2] && operatorMenus[2]->isVisible()
            ? operatorMenus[2].get()
            : operatorMenus[OperatorNavigationLayout::overflowIndex].get())
        : operatorMenus[(size_t)index].get();
    if (!target || !target->isVisible()) return;

    // Capture device identities *when the menu opens*, not when it closes.
    // Enumeration order may change when a USB MIDI device is reconnected.
    const auto inputs=juce::MidiInput::getAvailableDevices();
    const auto outputs=juce::MidiOutput::getAvailableDevices();
    const int faderRange=faderCeilingDb;
    const bool twelve=appPreferences && appPreferences->getBoolValue("clock12Hour",false);

    const auto populate=[this,&inputs,&outputs,faderRange,twelve]
                        (juce::PopupMenu& m,int group) {
        const int base=(group+1)*100;
        switch (group) {
        case 0: // File: no duplicate Pre-flight or Diagnostics.
            m.addItem(base+1,"New show");
            m.addItem(base+2,"Open show...");
            m.addItem(base+3,"Save show");
            break;
        case 1: // Edit actions are only here (plus direct cue affordances).
            m.addItem(base+1,"Undo",undoButton.isEnabled());
            m.addItem(base+2,"Redo",redoButton.isEnabled());
            m.addSeparator();
            m.addItem(base+3,"Add cue");
            m.addItem(base+4,"Add column...");
            break;
        case 2: // Show: all operational checks in one place.
            m.addItem(base+1,"Rename show...");
            m.addSeparator();
            m.addItem(base+2,"Pre-flight...");
            m.addItem(base+3,"Diagnostics...");
            break;
        case 3: // Do not clutter a one-Cuelist project with inert actions.
            m.addItem(base+1,"New Cuelist",show.cuelists.size()<12);
            m.addItem(base+2,"Rename active Cuelist");
            if (show.cuelists.size()>1) {
                m.addSeparator();
                if (show.activeCuelist>0)
                    m.addItem(base+3,"Move active Cuelist left");
                if (show.activeCuelist+1<(int)show.cuelists.size())
                    m.addItem(base+4,"Move active Cuelist right");
                m.addItem(base+5,"Delete active Cuelist...");
            }
            break;
        case 4: { // Devices: full legacy MIDI/OSC controls, not "Settings".
            m.addItem(base+1,"Audio Devices...");
            juce::PopupMenu midiIn;
            midiIn.addSectionHeader("MIDI INPUT — ENABLE");
            midiIn.addItem(10000,"Disable main MIDI input",true,
                           show.controls.midiInputId.isEmpty());
            for (int i=0;i<inputs.size() && i<100;++i) {
                const auto& device=inputs.getReference(i);
                midiIn.addItem(10001+i,device.name,true,
                    show.controls.midiInputId==device.identifier ||
                    isAdditionalMidiInputEnabled(device.identifier));
            }
            m.addSubMenu("MIDI Inputs",midiIn);

            juce::PopupMenu midiOut;
            midiOut.addSectionHeader("MIDI OUTPUT — ENABLE");
            for (int i=0;i<outputs.size() && i<100;++i) {
                const auto& device=outputs.getReference(i);
                const bool enabled=!appPreferences ||
                    appPreferences->getBoolValue(
                        "midiOutputEnabled_"+device.identifier,true);
                midiOut.addItem(11001+i,device.name,true,enabled);
            }
            m.addSubMenu("MIDI Outputs",midiOut,!outputs.isEmpty());

            juce::PopupMenu osc;
            osc.addSectionHeader("Local IP: "+
                                 juce::IPAddress::getLocalAddress().toString());
            const auto ports=configuredOscPorts();
            osc.addItem(base+2,"Receive UDP ports: "+
                         (ports.isEmpty()?"not configured":ports));
            osc.addItem(base+3,"Disable OSC input",ports.isNotEmpty());
            m.addSubMenu("OSC Input",osc);
            m.addSeparator();
            // MIDI/OSC Learn already have dedicated always-visible status
            // buttons in the live header. Do not duplicate them here.
            m.addItem(base+4,"Control Mapping...");
            break;
        }
        case 5: { // View: old Fader Range and Clock were not removed.
            m.addItem(base+1,inspectorVisible ? "Collapse Inspector" :
                                              "Expand Inspector");
            m.addItem(base+2,timelineVisible ? "Collapse Timeline" :
                                               "Expand Timeline");
            m.addSeparator();
            m.addItem(base+14,"Show Clock",true,showClock);
            m.addItem(base+15,"Show CPU Meter",true,showCpuMeter);
            m.addItem(base+16,"Show RAM Meter",true,showRamMeter);
            // Collapse all remains directly on the cue header; a second
            // identical menu command only adds operator noise.
            m.addSeparator();
            juce::PopupMenu appearanceMenu;
            appearanceMenu.addItem(base+20,"Legacy — Graphite / Copper",true,
                appearanceTheme()==palette::Appearance::legacy);
            appearanceMenu.addItem(base+21,"Industrial Orange",true,
                appearanceTheme()==palette::Appearance::industrialOrange);
            m.addSubMenu("Appearance",appearanceMenu);
            juce::PopupMenu faderMenu;
            faderMenu.addItem(base+10,faderRangeMenuTitle(6),true,faderRange==6);
            faderMenu.addItem(base+11,faderRangeMenuTitle(0),true,faderRange==0);
            m.addSubMenu("Fader Range",faderMenu);
            juce::PopupMenu clockMenu;
            clockMenu.addItem(base+12,"24-hour clock",true,!twelve);
            clockMenu.addItem(base+13,"12-hour clock",true,twelve);
            m.addSubMenu("Clock",clockMenu);
            break;
        }
        case 6:
            m.addItem(base+1,"User Guide");
            m.addItem(base+2,"About The CUElist");
            break;
        default: break;
        }
    };

    juce::PopupMenu menu;
    menu.setLookAndFeel(&theme);
    if (legacyPreferences) {
        // Preserve every legacy Settings action without a second menu tree:
        // Show = Pre-flight/Diagnostics, Cuelists = rename, Devices = I/O,
        // View = appearance, fader range and clock.
        for (int group : {2,3,4,5}) {
            juce::PopupMenu sub;
            sub.setLookAndFeel(&theme);
            populate(sub,group);
            menu.addSubMenu(OperatorNavigationLayout::title(group),sub);
        }
    } else if (index==OperatorNavigationLayout::overflowIndex) {
        const int first=OperatorNavigationLayout::atWidth(getWidth())
                            .visibleMenuCount;
        for (int i=first;i<OperatorNavigationLayout::menuCount;++i) {
            juce::PopupMenu sub;
            sub.setLookAndFeel(&theme);
            populate(sub,i);
            menu.addSubMenu(OperatorNavigationLayout::title(i),sub);
        }
    } else populate(menu,index);

    auto safe=juce::Component::SafePointer<MainComponent>(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(target),
        [safe,inputs,outputs](int chosen) {
            if (!safe || chosen<=0) return;
            if (chosen>=10000) {
                safe->dispatchOperatorDeviceChoice(chosen,inputs,outputs);
            } else {
                safe->dispatchOperatorMenu(chosen/100-1,chosen%100);
            }
        });
}

void MainComponent::dispatchOperatorDeviceChoice(int choice,
    const juce::Array<juce::MidiDeviceInfo>& inputs,
    const juce::Array<juce::MidiDeviceInfo>& outputs) {
    if (choice==10000) {
        selectControlInput({});
    } else if (choice>=10001 && choice<10101) {
        const int index=choice-10001;
        if (index>=inputs.size()) return;
        const auto device=inputs.getReference(index).identifier;
        if (device==show.controls.midiInputId)
            selectControlInput({});
        else if (show.controls.midiInputId.isEmpty() &&
                 !isAdditionalMidiInputEnabled(device))
            selectControlInput(device);
        else toggleAdditionalMidiInput(device);
    } else if (choice>=11001 && choice<11101) {
        const int index=choice-11001;
        if (index>=outputs.size()) return;
        const auto& device=outputs.getReference(index);
        const bool enabled=appPreferences ?
            !appPreferences->getBoolValue(
                "midiOutputEnabled_"+device.identifier,true) : false;
        if (appPreferences)
            appPreferences->setValue("midiOutputEnabled_"+device.identifier,
                                     enabled);
        engine.setMidiOutputEnabled(device.identifier,enabled);
        status="MIDI output "+device.name+
               (enabled?" enabled.":" disabled.");
        repaint();
    }
}

void MainComponent::dispatchOperatorMenu(int group,int command) {
    switch (group) {
    case 0: // File: preserve original discard confirmation and save.
        if (command==1 && newButton.onClick) newButton.onClick();
        else if (command==2 && openButton.onClick) openButton.onClick();
        else if (command==3 && saveButton.onClick) saveButton.onClick();
        break;
    case 1:
        if (command==1 && undoButton.isEnabled()) undoEdit();
        else if (command==2 && redoButton.isEnabled()) redoEdit();
        else if (command==3) addCue();
        else if (command==4)
            showColumnMenu(viewport.getScreenBounds().getCentre());
        break;
    case 2: // Show: checks appear only here.
        if (command==1) projectNameLabel.showEditor();
        else if (command==2) showPreflight();
        else if (command==3) showDiagnostics();
        break;
    case 3:
        if (command==1 && show.cuelists.size()<12) createCuelist();
        else if (command==2) renameActiveCuelist();
        else if (command==3 && show.activeCuelist>0)
            moveCuelist(show.activeCuelist,show.activeCuelist-1);
        else if (command==4 &&
                 show.activeCuelist+1<(int)show.cuelists.size())
            moveCuelist(show.activeCuelist,show.activeCuelist+1);
        else if (command==5 && show.cuelists.size()>1)
            requestDeleteCuelist(show.activeCuelist);
        break;
    case 4: // Devices: same native handlers used in the original Settings.
        if (command==1) showDevices();
        else if (command==2) {
            auto safe=juce::Component::SafePointer<MainComponent>(this);
            showOperatorTextPrompt(*this,theme,"OSC Input Ports",
                "Comma-separated UDP ports, up to 16; empty disables listening.",
                "Receive UDP ports",configuredOscPorts(),false,"Apply",
                [safe](juce::String value) {
                    if (safe) safe->configureOscInputPorts(value);
                });
        }
        else if (command==3) configureOscInputPorts("");
        else if (command==4) openControlPanel(false);
        break;
    case 5:
        if(command==20 || command==21) {
            setAppearanceTheme(command==20 ?
                palette::Appearance::legacy : palette::Appearance::industrialOrange);
        }
        else if (command==1) inspectorSideTab.triggerClick();
        else if (command==2) setTimelineVisible(!timelineVisible);
        else if (command>=14 && command<=16) {
            bool* target=command==14 ? &showClock :
                         command==15 ? &showCpuMeter : &showRamMeter;
            *target=!*target;
            if (appPreferences) appPreferences->setValue(
                command==14 ? "viewShowClock" :
                command==15 ? "viewShowCpu" : "viewShowRam",*target);
            repaint();
        }
        else if (command==10 || command==11)
            setAudioFaderCeilingDb(command==10 ? 6 : 0);
        else if (command==12 || command==13) {
            if (appPreferences)
                appPreferences->setValue("clock12Hour",command==13);
            repaint();
        }
        break;
    case 6:
        if (command==1) openUserGuide();
        else if (command==2) showAbout();
        break;
    default: break;
    }
}

void MainComponent::setTimelineVisible(bool visible) {
    if(timelineVisible==visible)return;
    timelineVisible=visible;
    if(appPreferences)appPreferences->setValue("viewTimelineVisible",visible);
    resized();
    repaint();
}
void MainComponent::resized() {
    const int h=getHeight();
    const auto geometry=workspaceLayout();
    editorX=geometry.editorX;
    // Reuse the existing command callbacks from the menus, but hide their
    // duplicated sidebar hitboxes. No action or shortcut is deleted.
    for (auto* control:{&newButton,&openButton,&saveButton,
                        &deviceButton,&preferencesButton,&controlButton})
        control->setVisible(false);
    projectNameLabel.setBounds(28,122,132,34);
    // setSize() in the MainComponent constructor runs resized() before
    // optional recent-project child controls have been constructed.
    if(recentProjectButtons[0]) {
    const int recentTop=218,recentBottom=h-349;
    const bool recentVisible=recentBottom>recentTop+62;
    recentOpenButton.setVisible(recentVisible);
    if(recentVisible) {
        recentOpenButton.setBounds(19,recentTop+32,102,26);
        recentMoreButton.setBounds(126,recentTop+32,35,26);
    }
    const int listTop=recentTop+68;
    const int capacity=recentVisible ?
        juce::jlimit(0,visibleRecentProjects,
                     (recentBottom-listTop-5)/43) : 0;
    // At short window heights fewer than five rows fit. The All action
    // must expose *every* hidden entry, not only history positions >5.
    recentMoreButton.setVisible(recentVisible &&
                                recentProjects.size()>capacity);
    for(int i=0;i<visibleRecentProjects;++i) {
        const bool showRow=i<capacity && i<recentProjects.size();
        auto& button=*recentProjectButtons[(size_t)i];
        auto& detail=*recentProjectDetails[(size_t)i];
        button.setVisible(showRow);
        detail.setVisible(showRow);
        if(showRow) {
            const int y=listTop+i*43;
            button.setBounds(19,y,142,24);
            detail.setBounds(21,y+24,138,16);
        }
    }
    }
    refreshCuelistTabs();
    const auto nav=OperatorNavigationLayout::atWidth(getWidth());
    for (int i=0;i<=OperatorNavigationLayout::overflowIndex;++i)
        if (auto* button=operatorMenus[(size_t)i].get()) {
            const bool visible=i<nav.visibleMenuCount ||
                (i==OperatorNavigationLayout::overflowIndex &&
                 nav.visibleMenuCount<OperatorNavigationLayout::menuCount);
            button->setVisible(visible);
            if (visible) button->setBounds(nav.bounds[(size_t)i]);
        }
    // The seven native menu categories are unchanged. On wide headers, two
    // compact curved-arrow controls live immediately after Guide; on narrow
    // widths they remain accessible from Edit/More and via keyboard.
    helpButton.setVisible(false);
    const bool guideVisible=nav.visibleMenuCount>
                            OperatorNavigationLayout::menuCount-1;
    const int historyLeft=guideVisible ?
        nav.bounds[OperatorNavigationLayout::menuCount-1].getRight()+6 : 0;
    constexpr int historyButtonSize=24, historyGap=3;
    const int historyRight=HeaderTelemetryLayout::atWidth(getWidth()).clock.getX()-8;
    const bool showHistory=guideVisible &&
        historyLeft+2*historyButtonSize+historyGap<=historyRight;
    undoButton.setVisible(showHistory);
    redoButton.setVisible(showHistory);
    if(showHistory) {
        const int historyY=OperatorNavigationLayout::menuY+
            (OperatorNavigationLayout::menuHeight-historyButtonSize)/2;
        undoButton.setBounds(historyLeft,historyY,
                             historyButtonSize,historyButtonSize);
        redoButton.setBounds(historyLeft+historyButtonSize+historyGap,
                             historyY,historyButtonSize,historyButtonSize);
    }
    // Retain both original one-click actions and their Learn callbacks.
    const auto ioActivity=HeaderTelemetryLayout::atWidth(getWidth());
    globalMidiLearn.setBounds(ioActivity.midiLearn);
    globalOscLearn.setBounds(ioActivity.oscLearn);
    // Collapse-all follows the pinned header after JUCE has computed the
    // actual left scrollbar inset (see placement below).
    const bool allCollapsed=std::all_of(show.columns.begin(),show.columns.end(),
        [](const Column& col){return col.collapsed;});
    collapseAllButton.setButtonText(allCollapsed ? "Expand all" : "Collapse all");
    collapseAllButton.setAllCollapsed(allCollapsed);
    // A minimal edge grip: 20 x 46px instead of the oversized 26 x 86px
    // handle. The 26px collapsed rail remains unchanged to preserve all
    // pinned metrics, mixer and Timeline geometry.
    constexpr int gripWidth=20,gripHeight=46;
    const int tabX=inspectorVisible ? geometry.editorX-gripWidth/2
                                   : geometry.editorX+3;
    const int tabY=juce::jlimit(150,std::max(150,h-135),h/2-gripHeight/2);
    inspectorSideTab.setBounds(tabX,tabY,gripWidth,gripHeight);
    inspectorSideTab.setInspectorOpen(inspectorVisible);
    inspectorSideTab.setTooltip(inspectorVisible
        ? "Collapse Inspector to the right"
        : "Expand Inspector to the left");

    preflightButton.setVisible(false); diagnosticsButton.setVisible(false);
    addRowButton.setVisible(false); addColumnButton.setVisible(false);
    const bool docked=(clipEditor && !clipWindow) || (oscEditor && !oscWindow);
    // All workspace geometry comes from a single model-independent module.
    // The component continues to own editor lifetime and visibility.
    viewport.setBounds(geometry.viewport);
    utilities.setBounds(geometry.utilities);
    utilities.syncScroll(viewport.getViewPositionX());
    clipCollapseButton.setVisible(docked);
    clipCollapseButton.setCollapsed(clipCollapsed);
    clipCloseButton.setVisible(docked);
    editorDivider.setVisible(docked);
    if (docked) {
        editorDivider.setBounds(geometry.dockEditor.getX(),geometry.dockY-2,
                                geometry.dockEditor.getWidth(),2);
        editorDivider.toFront(false);
    }
    dockResize.setVisible(docked && !clipCollapsed);
    if (docked && !clipCollapsed) {
        dockResize.setBounds(geometry.dockResize);
        dockResize.toFront(false);
    }
    if (docked) {
        if (oscEditor && !oscWindow) {
            // The OSC editor uses the same top-right native window controls
            // as MIDI (and remains reachable when its body is collapsed).
            const auto layout=OscEditorToolbarLayout::atWidth(
                geometry.dockEditor.getWidth());
            const auto origin=geometry.dockEditor.getPosition();
            clipCollapseButton.setBounds(
                layout.collapse.translated(origin.x,origin.y));
            clipCloseButton.setBounds(
                layout.close.translated(origin.x,origin.y));
        } else if (clipEditor && !clipWindow) {
            // All MIDI dock widths use the SAME editor-local geometry.
            // Detach is owned by PianoRoll; Collapse and Close are owned
            // by MainComponent, translated exactly once into root space.
            const auto layout=MidiEditorToolbarLayout::atWidth(
                geometry.dockEditor.getWidth());
            const auto origin=geometry.dockEditor.getPosition();
            clipCollapseButton.setBounds(
                layout.collapse.translated(origin.x,origin.y));
            clipCloseButton.setBounds(
                layout.close.translated(origin.x,origin.y));
        } else {
            clipCollapseButton.setBounds(geometry.dockCollapse.withWidth(28));
            clipCloseButton.setBounds(geometry.dockClose);
        }
        clipCollapseButton.toFront(false); clipCloseButton.toFront(false);
        if (clipEditor && !clipWindow) {
            clipEditor->setVisible(!clipCollapsed);
            if (!clipCollapsed) clipEditor->setBounds(geometry.dockEditor);
        }
        if (oscEditor && !oscWindow) {
            oscEditor->setVisible(!clipCollapsed);
            if (!clipCollapsed) oscEditor->setBounds(geometry.dockEditor);
        }
    }
    timeline.setBounds(geometry.timeline);
    timeline.setVisible(timelineVisible);
    // The arrow is rooted in MainComponent, NOT the Timeline child:
    // once hidden, its expand target must remain reachable. When shown it
    // sits after the TIMELINE title and before the GRID section.
    timelineCollapseButton.setCollapsed(!timelineVisible);
    timelineCollapseButton.setTooltip(timelineVisible ?
        "Collapse Timeline (also available in View)." :
        "Expand Timeline (also available in View).");
    // The fold control precedes the title on the left in both states,
    // so it never intersects GRID, SNAP or the right-aligned zoom tools.
    const int timelineGripX=geometry.timeline.getX()+12;
    const int timelineGripY=timelineVisible ? geometry.timeline.getY()+10 :
        std::max(72,std::min(getHeight()-52,geometry.timeline.getY()-31));
    timelineCollapseButton.setBounds(timelineGripX,timelineGripY,28,26);
    timelineCollapseButton.setVisible(true);
    timelineCollapseButton.toFront(false);
    timelineResize.setBounds(geometry.timelineResize);
    timelineResize.setVisible(timelineVisible);
    if (timelineVisible) timelineResize.toFront(false);
    const int x=geometry.inspectorLeft, inner=geometry.inspectorInnerWidth;
    cellTab.setVisible(false);rowTab.setVisible(false);
    rowBehaviour.setVisible(false);rowMute.setVisible(false);rowGain.setVisible(false);
    behaviour.setVisible(false);cellAutoStop.setVisible(false);
    clearButton.setVisible(false);applyButton.setVisible(false);
    playCellButton.setVisible(false);stopCellButton.setVisible(false);
    rowName.setBounds(x,225,inner,33);
    bpm.setBounds(x,293,inner,32);
    rowLoop.setBounds(x-3,374,inner,25);
    cellName.setBounds(x,475,inner,31);
    gainDb.setBounds(x+57,528,99,27);
    follow.setBounds(x-3,564,inner,25);
    loop.setBounds(x-3,597,inner,25);
    midiEditButton.setBounds(x,648,inner,33);
    oscEditButton.setBounds(x,648,inner,33);
    importButton.setBounds(x,691,inner,33);
    // Place notes below the existing Inspector actions at every supported
    // height, without changing the safe minimum hitboxes of those actions.
    const int notesAfterActions=geometry.scaleInspectorControl({0,728,1,25}).getBottom()+8;
    const int notesTop=std::max(notesAfterActions,geometry.inspectorPanel.getBottom()-167);
    cueNotes.setBounds(x,notesTop+27,inner,
        std::max(38,geometry.inspectorPanel.getBottom()-notesTop-38));
    // Compact inspector on smaller displays, keeping all actions reachable.
    {
        for (juce::Component* c : std::initializer_list<juce::Component*>{&rowLoop,&rowName,&bpm,&cellName,&follow,&loop,&gainDb,&importButton,&midiEditButton,&oscEditButton})
            c->setBounds(geometry.scaleInspectorControl(c->getBounds()));
    }
    goButton.setBounds(14,h-282,152,48);
    panicButton.setBounds(14,h-228,152,48);
    const bool learning=quickLearnMode!=QuickLearnMode::off;
    goLearnHotspot.setBounds(goButton.getBounds());
    panicLearnHotspot.setBounds(panicButton.getBounds());
    goLearnHotspot.setLearnActive(learning);
    panicLearnHotspot.setLearnActive(learning);
    grid.setSize(geometry.gridSize.x,geometry.gridSize.y);
    // With the scrollbar on the left, JUCE also moves the content origin.
    // Shift our separately pinned header by the identical visible inset so
    // its text and hitboxes stay pixel-aligned with their cue rows.
    const int leftScrollInset=viewport.getVerticalScrollBar().isVisible()
        ? viewport.getScrollBarThickness() : 0;
    cueHeaders.setBounds(geometry.cueHeaders.translated(leftScrollInset,0));
    cueHeaders.toFront(false);
    cuelistMoveLeft.setBounds(
        cueHeaders.getX()+CueGrid::nameWidth-146,cueHeaders.getY()+10,24,22);
    cuelistMoveRight.setBounds(
        cueHeaders.getX()+CueGrid::nameWidth-116,cueHeaders.getY()+10,24,22);
    cuelistMoveLeft.toFront(false);
    cuelistMoveRight.toFront(false);
    // Place an independent 72px-high heading over every visible column.
    // Its x is the exact right edge of the pinned cue-name lane. Column
    // content scrolls horizontally beneath it, but cannot rise through it.
    const int columnHeaderX=cueHeaders.getRight();
    const int columnHeaderWidth=std::max(0,viewport.getRight()-columnHeaderX);
    columnHeaders.setBounds(columnHeaderX,viewport.getY(),
                            columnHeaderWidth,CueGrid::headerHeight);
    columnHeaders.toFront(false);
    collapseAllButton.setBounds(cueHeaders.getX()+CueGrid::nameWidth-38,
                                cueHeaders.getY()+9,30,24);
    cueMetrics.setBounds(geometry.metrics);
    cueMetrics.syncBpmEditor();
    cueMetrics.toFront(false);
    collapseAllButton.toFront(false);
    cuelistMoveLeft.toFront(false);
    cuelistMoveRight.toFront(false);
    inspectorSideTab.toFront(false);
    if (learning) {
        goLearnHotspot.toFront(false);
        panicLearnHotspot.toFront(false);
    }
    const bool learnVisible=inspectorVisible && quickLearnMode!=QuickLearnMode::off;
    quickControlTargets.setVisible(learnVisible);
    quickLearnMapSelector.setVisible(learnVisible);
    cancelQuickLearn.setVisible(learnVisible);
    if (learnVisible) {
        // Learning temporarily replaces Inspector content; no extra window.
        for (juce::Component* c:std::initializer_list<juce::Component*>{
                 &rowName,&bpm,&rowLoop,&cellName,&gainDb,&follow,
                 &loop,&midiEditButton,&oscEditButton,&importButton,&cueNotes,
                 &cellAutoStop,&rowMute,&rowGain,&rowBehaviour,&behaviour})
            c->setVisible(false);
        quickLearnMapSelector.setBounds(x+4,145,inner-8,27);
        cancelQuickLearn.setBounds(x+4,179,inner-8,29);
        const int top=235;
        quickControlTargets.setBounds(x+4,top,inner-8,
            std::max(60,geometry.inspectorPanel.getBottom()-top-20));
        quickControlTargets.toFront(false);
        quickLearnMapSelector.toFront(false);
        cancelQuickLearn.toFront(false);
    }
    // Overlays must stay above the grid, pinned headings and Cuelist tabs.
    positionInlineNameEditor();
}
void MainComponent::refresh() {
    checkpointHistory("Edit Show");
    selectedRow=juce::jlimit(0,(int)show.rows.size()-1,selectedRow);
    selectedColumn=juce::jlimit(0,(int)show.columns.size()-1,selectedColumn);
    visibleClipCount=0;
    for (const auto& row:show.rows)
        for (size_t c=0;c<row.cells.size() && c<show.columns.size();++c)
            if (show.columns[c].type!=ColumnType::control) {
                const auto& cell=row.cells[c];
                if (cell.midi || cell.osc || cell.file.isNotEmpty() ||
                    clips.find(cell.id)!=clips.end()) ++visibleClipCount;
            }
    syncClipEditor(); syncOscEditor(); refreshInspector(); utilities.refresh(); resized();
    cueHeaders.repaint();
    columnHeaders.repaint();
    if (!projectNameLabel.isBeingEdited() && projectNameLabel.getText()!=show.name)
        projectNameLabel.setText(show.name,juce::dontSendNotification);
    repaint(); grid.repaint(); timeline.repaint(); cueMetrics.repaint();
}
bool MainComponent::select(int row,int column) {
    if (row<0 || row>=(int)show.rows.size() || column<0 || column>=(int)show.columns.size() || !applyInspector()) return false;
    // Collapsed cells remain selectable as cues, but not editable.
    nextCue=row;
    if (liveCuelist==show.activeCuelist) liveNextCue=row;
    selectedRow=row; selectedColumn=column; syncClipEditor(); syncOscEditor(); refreshInspector();
    const int top=CueGrid::headerHeight+row*CueGrid::rowHeight, bottom=top+CueGrid::rowHeight;
    int viewY=viewport.getViewPositionY();
    // The top 72px are occupied by a pinned heading, not usable row space.
    // Otherwise selecting the first cue while scrolled can hide it under BPM.
    if (top<viewY+CueGrid::headerHeight)
        viewY=std::max(0,top-CueGrid::headerHeight);
    else if (bottom>viewY+viewport.getViewHeight())
        viewY=bottom-viewport.getViewHeight();
    viewport.setViewPosition(viewport.getViewPositionX(),std::max(0,viewY));
    repaint(); grid.repaint(); timeline.repaint(); return true;
}
Show MainComponent::liveShowSnapshot() const {
    Show live=show;
    if(liveCuelist>=0 && liveCuelist<(int)show.cuelists.size() &&
       liveCuelist!=show.activeCuelist) {
        live.rows=show.cuelists[(size_t)liveCuelist].rows;
        live.activeCuelist=liveCuelist;
    }
    return live;
}
std::unique_ptr<RenderState> MainComponent::makeLiveRenderState() const {
    return makeRenderState(liveShowSnapshot(),clips);
}
void MainComponent::changed(const juce::String& text,bool stopPlayback,
                            bool retimeSchedule) {
    if (projectLoader) projectLoader->cancel(); // never overwrite newer edits
    checkpointHistory(text);
    const bool stopLive=stopPlayback && liveCuelist==show.activeCuelist;
    if (stopLive) { engine.panic(); resetActiveMapping(); }
    engine.publish(makeLiveRenderState(),!stopLive,retimeSchedule);
    dirty=true; status=text; refresh();
}
void MainComponent::openUserGuide() {
    // Keep the Markdown file as the source of truth, but display it in a
    // native non-modal JUCE window so the operator is never sent to a
    // browser or an external text editor.
    const auto directory=juce::File::getSpecialLocation(
        juce::File::currentExecutableFile).getParentDirectory();
    const auto guide=directory.getChildFile("docs")
                              .getChildFile("complete-user-guide.md");
    if (!guide.existsAsFile()) {
        message("User Guide not found next to the executable. "
                "Rebuild the application or verify that "
                "docs/complete-user-guide.md is included with the package.");
        return;
    }
    const auto contents=guide.loadFileAsString();
    if (contents.trim().isEmpty()) {
        message("The bundled User Guide is empty or could not be read.");
        return;
    }
    if (!userGuideWindow)
        userGuideWindow=std::make_unique<GuideWindow>(contents);
    else
        userGuideWindow->setMarkdown(contents);

    userGuideWindow->setVisible(true);
    userGuideWindow->toFront(true);
}

void MainComponent::showAbout() {
    using Status=OperatorReportRow::Status;
    showOperatorReport(*this,theme,"About The Cuelist",
        "Version "+juce::String(THE_CUELIST_VERSION),{
            {"AUTHORS",{
                {"Davide Salvatore","Co-author",Status::neutral},
                {"Riccardo D'Ambrosio","Co-author",Status::neutral}
            }},
            {"APPLICATION",{
                {"Copyright","2026",Status::neutral},
                {"Rights","All rights reserved",Status::neutral}
            }},
            {"LICENCE",{
                {"Software","Proprietary",Status::neutral},
                {"Public source licence","Not granted",Status::caution}
            }}
        },
        "The Cuelist's copyright notice is in LICENSE. Third-party "
        "notices (including JUCE and embedded fonts) are in NOTICE.md.");
}

void MainComponent::message(const juce::String& text) { status=text; repaint(); juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"The Cuelist",text,"OK",this); }
bool MainComponent::memoryFits(const ClipMap& collection) const {
    return withinPcmBudget(collection);
}
bool MainComponent::canGenerateClickTrack(int row,int column) const {
    if (row<0 || row>=(int)show.rows.size() ||
        column<0 || column>=(int)show.columns.size()) return false;
    const auto& col=show.columns[(size_t)column];
    const auto& cell=show.rows[(size_t)row].cells[(size_t)column];
    return col.type==ColumnType::audio && !col.collapsed &&
           cell.file.isEmpty() && !cell.click && !cell.midi && !cell.osc &&
           !hasClip(row,column);
}
bool MainComponent::commitClickTrack(int row,int column,
                                     const ClickTrackConfig& config) {
    if (!applyInspector() || row<0 || row>=(int)show.rows.size() ||
        column<0 || column>=(int)show.columns.size())
        return false;
    const auto& existing=show.rows[(size_t)row].cells[(size_t)column];
    const bool editing=existing.click.has_value();
    if (show.columns[(size_t)column].type!=ColumnType::audio ||
        show.columns[(size_t)column].collapsed ||
        (!editing && !canGenerateClickTrack(row,column)) ||
        (editing && (existing.file.isNotEmpty() || existing.midi || existing.osc)))
        return false;
    // Build the validated candidate AFTER applying unsaved Inspector edits.
    // Never call select(): selecting a cue changes NEXT in the LIVE tab.
    auto candidate=show;
    auto& target=candidate.rows[(size_t)row].cells[(size_t)column];
    target.click=config;
    if (target.name.trim().isEmpty()) target.name="Click Track";
    const auto valid=validateShow(candidate);
    if (valid.failed()) {
        status=valid.getErrorMessage();
        repaint();
        return false;
    }
    show=std::move(candidate);
    changed(editing ? "Click Track updated. LIVE continues." :
        "Click Track generated. LIVE continues.",false,true);
    return true;
}
void MainComponent::generateClickTrack(int row,int column) {
    if (row<0 || row>=(int)show.rows.size() ||
        column<0 || column>=(int)show.columns.size()) return;
    const auto& cell=show.rows[(size_t)row].cells[(size_t)column];
    if (!canGenerateClickTrack(row,column) &&
        !(show.columns[(size_t)column].type==ColumnType::audio &&
          !show.columns[(size_t)column].collapsed && cell.click)) return;
    // Resolve stable identity after the non-modal dialog: rows, columns and VIEW
    // may have moved while the operator was entering the configuration.
    const int rowId=show.rows[(size_t)row].id;
    const int columnId=show.columns[(size_t)column].id;
    const int cellId=cell.id;
    const int viewIndex=show.activeCuelist;
    const bool editing=cell.click.has_value();
    const auto current=cell.click.value_or(ClickTrackConfig{});
    auto* dialog=new juce::AlertWindow(editing ? "Configure Click Track" :
        "Generate Click Track",
        "Procedural Audio source. Tempo Markers retime pulses without stopping LIVE. "
        "Use cell gain and track fader for level.",
        juce::MessageBoxIconType::NoIcon,this);
    dialog->setLookAndFeel(&theme);
    dialog->addComboBox("sound",{"Classic Click","Woodblock","Electronic Beep",
                                  "Rimshot"},"Sound");
    dialog->addComboBox("accent",{"On (first movement of each bar)","Off"},"Accent");
    dialog->addComboBox("subdivision",{"1 - Quarter notes","2 - Eighth notes",
                                       "3 - Triplets","4 - Sixteenth notes"},"Subdivision");
    dialog->addTextEditor("duration",juce::String(current.durationSeconds,2),
                          "Duration in seconds (0.1-600)");
    dialog->getComboBoxComponent("sound")->setSelectedId((int)current.sound+1,
                                                       juce::dontSendNotification);
    dialog->getComboBoxComponent("accent")->setSelectedId(current.accent ? 1 : 2,
                                                        juce::dontSendNotification);
    dialog->getComboBoxComponent("subdivision")->setSelectedId(current.subdivision,
                                                             juce::dontSendNotification);
    dialog->addButton(editing ? "Update Click Track" : "Generate Click Track",1);
    dialog->addButton("Cancel",0);
    auto safe=juce::Component::SafePointer<MainComponent>(this);
    dialog->enterModalState(true,juce::ModalCallbackFunction::create(
        [safe,dialog,rowId,columnId,cellId,viewIndex](int result) {
            std::unique_ptr<juce::AlertWindow> owned(dialog);
            if (!safe || result!=1 || safe->show.activeCuelist!=viewIndex) return;
            int r=-1,c=-1;
            for (int i=0;i<(int)safe->show.rows.size();++i)
                if (safe->show.rows[(size_t)i].id==rowId) {r=i;break;}
            for (int i=0;i<(int)safe->show.columns.size();++i)
                if (safe->show.columns[(size_t)i].id==columnId) {c=i;break;}
            if (r<0 || c<0 ||
                safe->show.rows[(size_t)r].cells[(size_t)c].id!=cellId) return;
            const auto& source=safe->show.rows[(size_t)r].cells[(size_t)c];
            if (!safe->canGenerateClickTrack(r,c) &&
                !(safe->show.columns[(size_t)c].type==ColumnType::audio &&
                  !safe->show.columns[(size_t)c].collapsed && source.click)) {
                safe->message("Click Track cancelled: the target Audio cell has changed.");
                return;
            }
            const auto duration=dialog->getTextEditorContents("duration").trim();
            const char* raw=duration.toRawUTF8();
            char* end=nullptr;
            const double seconds=std::strtod(raw,&end);
            if(duration.isEmpty() || end==raw || *end!='\0' ||
               !std::isfinite(seconds) || seconds<0.1 || seconds>600) {
                safe->message("Click duration must be a number from 0.1 to 600 seconds.");
                return;
            }
            const auto* sound=dialog->getComboBoxComponent("sound");
            const auto* accent=dialog->getComboBoxComponent("accent");
            const auto* subdivision=dialog->getComboBoxComponent("subdivision");
            if(!sound || !accent || !subdivision || sound->getSelectedId()<1 ||
               sound->getSelectedId()>4 || accent->getSelectedId()<1 ||
               accent->getSelectedId()>2 || subdivision->getSelectedId()<1 ||
               subdivision->getSelectedId()>4) {
                safe->message("Select sound, accent and subdivision.");
                return;
            }
            ClickTrackConfig config{(ClickSound)(sound->getSelectedId()-1),
                accent->getSelectedId()==1,subdivision->getSelectedId(),seconds};
            if (!safe->commitClickTrack(r,c,config))
                safe->message("Click Track was not updated. Check the current cell and settings.");
        }),false); // async; no nested modal loop or audio callback dependency
}
bool MainComponent::canGenerateLtcTrack(int row,int column) const {
    if (row<0 || row>=(int)show.rows.size() ||
        column<0 || column>=(int)show.columns.size()) return false;
    const auto& col=show.columns[(size_t)column];
    const auto& cell=show.rows[(size_t)row].cells[(size_t)column];
    return col.type==ColumnType::audio && !col.collapsed &&
           cell.file.isEmpty() && !cell.midi && !cell.osc &&
           !hasClip(row,column);
}
void MainComponent::generateLtcTrack(int row,int column) {
    if (!canGenerateLtcTrack(row,column)) return;

    // Stable IDs make the async dialog resilient to row/column reorder.
    const int rowId=show.rows[(size_t)row].id;
    const int columnId=show.columns[(size_t)column].id;
    auto* dialog=new juce::AlertWindow("Generate LTC Track",
        "Create a mono 48 kHz SMPTE LTC WAV in Documents/The Cuelist/"
        "Generated LTC. It will be loaded as ordinary Audio.",
        juce::MessageBoxIconType::NoIcon,this);
    dialog->setLookAndFeel(&theme);
    dialog->addTextEditor("duration","10.0","Duration in seconds (0.1–600)");
    dialog->addTextEditor("timecode","01:00:00:00","Start timecode HH:MM:SS:FF");
    dialog->addComboBox("rate",{"24 fps","25 fps","29.97 fps (drop-frame)",
                                 "30 fps","29.97 fps (non-drop)"},
                        "Frame rate");
    if (auto* rate=dialog->getComboBoxComponent("rate"))
        rate->setSelectedId(2,juce::dontSendNotification);
    dialog->addButton("Generate LTC",1);
    dialog->addButton("Cancel",0);
    auto safe=juce::Component::SafePointer<MainComponent>(this);
    dialog->enterModalState(true,juce::ModalCallbackFunction::create(
        [safe,dialog,rowId,columnId](int result) {
            // Keep the AlertWindow alive throughout the async callback:
            // JUCE must not auto-delete it before the fields are read.
            std::unique_ptr<juce::AlertWindow> owned(dialog);
            if (!safe || result!=1) return;
            int r=-1,c=-1;
            for (int i=0;i<(int)safe->show.rows.size();++i)
                if (safe->show.rows[(size_t)i].id==rowId) {r=i;break;}
            for (int i=0;i<(int)safe->show.columns.size();++i)
                if (safe->show.columns[(size_t)i].id==columnId) {c=i;break;}
            if (!safe->canGenerateLtcTrack(r,c)) {
                safe->message("LTC cancelled: the target Audio cell is no longer empty.");
                return;
            }
            const auto duration=dialog->getTextEditorContents("duration").trim();
            const auto start=dialog->getTextEditorContents("timecode").trim();
            const char* raw=duration.toRawUTF8();
            char* end=nullptr;
            const double seconds=std::strtod(raw,&end);
            if (duration.isEmpty() || end==raw || *end!='\0' ||
                !std::isfinite(seconds)) {
                safe->message("Enter a valid LTC duration in seconds (e.g. 30 or 12.5).");
                return;
            }
            ltc::Config spec;
            spec.durationSeconds=seconds;
            const auto* rateBox=dialog->getComboBoxComponent("rate");
            switch (rateBox ? rateBox->getSelectedId() : 0) {
                case 1:spec.rate=ltc::Rate::fps24;break;
                case 2:spec.rate=ltc::Rate::fps25;break;
                case 3:spec.rate=ltc::Rate::drop2997;break;
                case 4:spec.rate=ltc::Rate::fps30;break;
                case 5:spec.rate=ltc::Rate::nonDrop2997;break;
                default:safe->message("Select an LTC frame rate.");return;
            }
            if (const auto parsed=ltc::parseTimecode(start,spec.rate,spec.start);
                parsed.failed()) {safe->message(parsed.getErrorMessage());return;}
            if (const auto validated=ltc::validate(spec);validated.failed()) {
                safe->message(validated.getErrorMessage());return;
            }
            const auto directory=juce::File::getSpecialLocation(
                juce::File::userDocumentsDirectory)
                .getChildFile("The Cuelist").getChildFile("Generated LTC");
            const auto file=directory.getChildFile("LTC-"+start
                .replaceCharacter(':','-').replaceCharacter(';','-')+
                "-"+juce::Uuid().toString()+".wav");
            if (const auto generated=ltc::writeWav(file,spec);generated.failed()) {
                safe->message("Could not generate LTC: "+generated.getErrorMessage());
                return;
            }
            if (!safe->select(r,c) || !safe->canGenerateLtcTrack(r,c)) {
                file.deleteFile();
                return;
            }
            safe->importFile(file); // regular Audio loading, routing and budget
            // A rejected preload must not leave an unused generated WAV.
            if (safe->show.rows[(size_t)r].cells[(size_t)c].file!=
                file.getFullPathName()) file.deleteFile();
        }),false); // callback owns the dialog and releases it after reading fields
}
void MainComponent::importDialog() {
    if (show.columns[selectedColumn].type==ColumnType::osc ||
        show.columns[selectedColumn].collapsed) return;
    if (!applyInspector()) return;
    const int cellId=show.rows[selectedRow].cells[selectedColumn].id;
    const bool midi=show.columns[selectedColumn].type==ColumnType::midi;
    chooser=std::make_unique<juce::FileChooser>(midi ? "Import MIDI clip (notes)" : "Import mono or stereo WAV or MP3",juce::File{},midi ? "*.mid;*.midi" : "*.wav;*.mp3");
    chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
        [safe=juce::Component::SafePointer<MainComponent>(this),cellId](const juce::FileChooser& fc) {
            if (!safe || fc.getResult()==juce::File{}) return;
            for (size_t r=0;r<safe->show.rows.size();++r) for (size_t c=0;c<safe->show.columns.size();++c)
                if (safe->show.rows[r].cells[c].id==cellId) { if (safe->select((int)r,(int)c)) safe->importFile(fc.getResult()); return; }
        });
}
void MainComponent::importFile(const juce::File& file) {
    if (show.columns[(size_t)selectedColumn].collapsed) return;
    if (show.columns[selectedColumn].type==ColumnType::midi) {
        if (!applyInspector()) return;
        MidiClip imported; juce::String warning;
        auto result=loadMidiClip(file,imported,warning);
        if (result.failed()) { message(result.getErrorMessage()); return; }
        auto& cell=show.rows[selectedRow].cells[selectedColumn]; cell.midi=std::move(imported); cell.click.reset();
        cell.file.clear(); cell.name=file.getFileNameWithoutExtension();
        changed("MIDI clip imported. Double-click to open the piano roll.",true);
        if (warning.isNotEmpty()) message(warning);
        return;
    }
    if (show.columns[selectedColumn].type!=ColumnType::audio) { message("Import audio into an Audio column."); return; }
    std::shared_ptr<const AudioClip> clip;
    const auto result=loadClip(file,clip);
    if (result.failed()) { message(result.getErrorMessage()); return; }
    auto updated=clips; auto& c=show.rows[selectedRow].cells[selectedColumn]; updated[c.id]=clip;
    if (!memoryFits(updated)) { message("Preload limit reached: 512 MB PCM per show. The file was not added."); return; }
    // Invalid imports no longer stop an ongoing performance; a successful
    // import still uses changed(...), preserving its original transport policy.
    clips=std::move(updated); c.click.reset(); c.file=file.getFullPathName(); c.name=file.getFileNameWithoutExtension();
    changed("Imported "+file.getFileName()+". Double-click the cell to play.");
}
bool MainComponent::send(int id,bool row,bool stop) {
    if (!engine.connected.load()) { message("No audio device available. Open Audio devices."); return false; }
    if (!engine.send(id,row,stop)) { engine.panic(); message("Command queue full: playback stopped. Try again."); return false; }
    status=stop ? "Cell stopped." : "Command sent to audio / MIDI transport."; return true;
}
void MainComponent::triggerRow(int row) {
    if (!select(row,selectedColumn)) return;
    // An explicit cue GO on a browsed Cuelist intentionally takes over the
    // transport. Browsing a tab alone never changes the live source.
    if (liveCuelist!=show.activeCuelist) {
        stopAll();
        liveCuelist=show.activeCuelist;
        liveNextCue=row;
        refreshCuelistTabs();
    }
    bool rowPausing=false;
    for (const auto& cell:show.rows[row].cells) {
        const auto action=show.rows[row].overrideBehaviour ? show.rows[row].behaviour : cell.behaviour;
        if (cell.followsRow && (action==Behaviour::pauseResume || action==Behaviour::pauseRestart)
            && engine.getCellPlayback(cell.id).playing) rowPausing=true;
    }
    for (size_t c=0;c<show.columns.size();++c) if (show.columns[c].type==ColumnType::midi
        && show.rows[row].cells[c].midi && show.rows[row].cells[c].followsRow && !show.rows[row].cells[c].midi->notes.empty()) {
        const auto& cell=show.rows[row].cells[c];
        const auto action=show.rows[row].overrideBehaviour ? show.rows[row].behaviour : cell.behaviour;
        if (action==Behaviour::stop || ((action==Behaviour::pauseResume || action==Behaviour::pauseRestart) && rowPausing)
            || (action==Behaviour::continuePlaying && engine.getCellPlayback(cell.id).playing)) continue;
        const auto& identifier=show.columns[c].midiOutputId;
        // A missing or disconnected MIDI output must never abort an Audio/OSC
        // cue. The router drops packets for invalid routes and refreshes the
        // configured device on reconnect; neither case blocks GO.
        if (identifier.isNotEmpty())
            (void)engine.setMidiOutput(identifier);
    }
    engine.publish(makeLiveRenderState());
    if (send(show.rows[row].id,true)) {
        nextCue=row+1; liveNextCue=nextCue;
        // The mapping changes when the audio callback acknowledges execution,
        // not when this command is merely accepted into its lock-free queue.
        // Physical-button edges remain tracked across the pending transition.
    }
    repaint(); grid.repaint();
}
void MainComponent::toggleRowPlayPause(int row) {
    if (liveCuelist!=show.activeCuelist) { triggerRow(row); return; }
    if(row<0 || row>=(int)show.rows.size() || !applyInspector()) return;
    // A pause/resume must not change the armed Next Cue or Inspector selection.
    bool active=false,paused=false;
    for(const auto& cell:show.rows[(size_t)row].cells) {
        if(!cell.followsRow) continue;
        const auto p=engine.getCellPlayback(cell.id);
        active=active||p.playing;
        paused=paused||p.paused;
    }
    if(!active && !paused) {triggerRow(row);return;}
    if(!engine.connected.load()) {message("No audio device available.");return;}
    if(!engine.sendRowPauseToggle(show.rows[(size_t)row].id)) {
        engine.panic();message("Transport command queue full.");return;
    }
    status=active ? "Cue paused." : "Cue resumed.";
    grid.repaint();repaint();
}
void MainComponent::goNext() {
    cueMetrics.cancelAutoStopCellPicking();
    if(liveCuelist==show.activeCuelist) {
        if(nextCue<(int)show.rows.size()) triggerRow(nextCue);
        else { status="End of live sequence. Select a cue to restart."; repaint(); }
        return;
    }
    if(liveCuelist<0 || liveCuelist>=(int)show.cuelists.size()) return;
    const auto& liveRows=show.cuelists[(size_t)liveCuelist].rows;
    if(liveNextCue<0 || liveNextCue>=(int)liveRows.size()) {
        status="End of live Cuelist. Return to its tab to rearm.";repaint();return;
    }
    const auto& cue=liveRows[(size_t)liveNextCue];
    for(size_t index=0;index<show.columns.size() && index<cue.cells.size();++index)
        if(show.columns[index].type==ColumnType::midi &&
           cue.cells[index].midi && cue.cells[index].followsRow &&
           show.columns[index].midiOutputId.isNotEmpty())
            (void)engine.setMidiOutput(show.columns[index].midiOutputId);
    engine.publish(makeLiveRenderState());
    if(send(cue.id,true)) {
        ++liveNextCue;
        status="GO sent to live Cuelist: "+show.cuelists[(size_t)liveCuelist].name+
               ". Browsed tab unchanged.";
    }
    repaint();grid.repaint();refreshCuelistTabs();
}
void MainComponent::triggerCell() {
    if (!applyInspector()) return;
    auto& c=show.rows[selectedRow].cells[selectedColumn];
    // Editing or importing a dormant VIEW cell is not a playback action.
    // Only an actual clip trigger may transfer ownership of the LIVE transport.
    if (show.columns[selectedColumn].type==ColumnType::control) {
        editControlSet(selectedRow,selectedColumn); return;
    }
    if (show.columns[selectedColumn].type==ColumnType::osc && !c.osc) { editOscClip(); return; }
    if (!hasClip(selectedRow,selectedColumn)) { if (show.columns[selectedColumn].type==ColumnType::midi) editMidiClip(); else if (show.columns[selectedColumn].type==ColumnType::osc) editOscClip(); else importDialog(); return; }
    if(liveCuelist!=show.activeCuelist) {
        stopAll();
        liveCuelist=show.activeCuelist;
        liveNextCue=nextCue;
        refreshCuelistTabs();
        // The audio callback still owns the old LIVE RenderState until a
        // snapshot of this Cuelist is published. A Cell Play from VIEW must
        // enqueue its command against the newly selected LIVE voices.
        engine.publish(makeLiveRenderState());
    }
    const bool playing=engine.getCellPlayback(c.id).playing;
    const bool needsOutput=c.behaviour!=Behaviour::stop && !(playing && (c.behaviour==Behaviour::pauseResume
        || c.behaviour==Behaviour::pauseRestart || c.behaviour==Behaviour::continuePlaying));
    if (show.columns[selectedColumn].type==ColumnType::midi && needsOutput) {
        const auto& identifier=show.columns[selectedColumn].midiOutputId;
        // Attempt to open only a configured route. Missing routes are a
        // global warning in the mixer, not a per-cell playback error.
        if (identifier.isNotEmpty()) (void)engine.setMidiOutput(identifier);
        engine.publish(makeLiveRenderState());
    }
    send(c.id,false);
}
void MainComponent::triggerCellAt(int row,int column) {
    const int armed=nextCue, armedLive=liveNextCue;
    const bool wasLiveView=liveCuelist==show.activeCuelist;
    if (!select(row,column)) return;
    // select() intentionally arms row GO during normal selection, but
    // the direct Cell Play hitbox must preserve both independent GO cursors.
    nextCue=armed;
    if (wasLiveView) liveNextCue=armedLive;
    triggerCell(); grid.repaint();
}
void MainComponent::stopCellAt(int row,int column) {
    if (row<0 || row>=(int)show.rows.size() || column<0 || column>=(int)show.columns.size()) return;
    send(show.rows[row].cells[column].id,false,true); repaint();
}
void MainComponent::stopRow(int row) {
    if (row<0 || row>=(int)show.rows.size()) return;
    if (send(show.rows[row].id,true,true)) status="All clips in cue "+juce::String(row+1)+" stopped.";
    repaint();
}
void MainComponent::setMasterAudio(float level) {
    if (!std::isfinite(level)) return;
    const float value=juce::jlimit(0.0f,4.0f,level);
    if (std::abs(show.masterGain-value)<1.0e-6f) return;
    show.masterGain=value;
    dirty=true;
    engine.publish(makeLiveRenderState());
    // The engine already receives the mapped value immediately. Keep the
    // actual JUCE fader in lockstep without invoking its onGainChanged hook.
    masterSlider.setGainDb(GainMeterFader::fromLinear(show.masterGain));
    checkpointHistory("setMasterAudio");
    status="MASTER audio gain updated."; repaint();
}
void MainComponent::setMasterPan(float pan) {
    if (!std::isfinite(pan)) return;
    const float value=juce::jlimit(-1.0f,1.0f,pan);
    if (std::abs(show.masterPan-value)<1.0e-6f) return;
    show.masterPan=value; dirty=true;
    engine.publish(makeLiveRenderState());
    masterPan.setValue(show.masterPan,juce::dontSendNotification);
    checkpointHistory("setMasterPan");
    status="MASTER pan updated."; utilities.repaint(); repaint();
}
void MainComponent::setMasterMute(bool muted) {
    if (show.masterMuted==muted) return;
    show.masterMuted=muted; dirty=true;
    engine.publish(makeLiveRenderState());
    checkpointHistory("setMasterMute");
    status=muted ? "PCM MASTER muted." : "PCM MASTER unmuted.";
    utilities.repaint(); repaint();
}
void MainComponent::setMasterOutput(int channel) {
    if(channel<0 || channel>1022 || channel%2!=0 || show.masterOutput==channel) return;
    show.masterOutput=channel; dirty=true;
    engine.publish(makeLiveRenderState());
    checkpointHistory("setMasterOutput");
    status="Master Bus Main Out set to "+juce::String(channel+1)+"/"+juce::String(channel+2)+".";
    utilities.refreshRouting(); repaint();
}
void MainComponent::setSafetyLimiter(bool enabled) {
    if(show.safetyLimiterEnabled==enabled) return;
    show.safetyLimiterEnabled=enabled; dirty=true;
    engine.publish(makeLiveRenderState());
    checkpointHistory("setSafetyLimiter");
    status=enabled ? "Master Safety Limiter enabled." : "Master Safety Limiter bypassed.";
    safetyLimiter.setToggleState(enabled,juce::dontSendNotification);
    repaint();
}
void MainComponent::setColumnMasterSend(int column,bool enabled) {
    if(column<0 || column>=(int)show.columns.size() ||
       show.columns[(size_t)column].type!=ColumnType::audio ||
       show.columns[(size_t)column].masterSend==enabled) return;
    show.columns[(size_t)column].masterSend=enabled;
    dirty=true; engine.publish(makeLiveRenderState());
    checkpointHistory("setColumnMasterSend");
    status=enabled ? "L/R: column sent to stereo Master Bus." : "L/R: column removed from Master Bus.";
    utilities.syncAudioControls(column); repaint();
}
void MainComponent::setColumnDirectPostFader(int column,bool post) {
    if(column<0 || column>=(int)show.columns.size() ||
       show.columns[(size_t)column].type!=ColumnType::audio ||
       show.columns[(size_t)column].directPostFader==post) return;
    show.columns[(size_t)column].directPostFader=post;
    dirty=true; engine.publish(makeLiveRenderState());
    checkpointHistory("setColumnDirectPostFader");
    status=post ? "Direct Out: POST column fader." : "Direct Out: PRE column fader.";
    utilities.refresh(); repaint();
}
void MainComponent::setColumnDirectEnabled(int column,bool enabled) {
    if(column<0 || column>=(int)show.columns.size() ||
       show.columns[(size_t)column].type!=ColumnType::audio ||
       show.columns[(size_t)column].directOutputEnabled==enabled) return;
    show.columns[(size_t)column].directOutputEnabled=enabled;
    dirty=true; engine.publish(makeLiveRenderState());
    checkpointHistory("setColumnDirectEnabled");
    status=enabled ? "Direct Output enabled." : "Direct Output off (L/R unchanged).";
    utilities.refresh(); repaint();
}
void MainComponent::setDataOutputMuted(int column,bool muted) {
    if(column<0 || column>=(int)show.columns.size()) return;
    auto& col=show.columns[(size_t)column];
    if((col.type!=ColumnType::midi && col.type!=ColumnType::osc) || col.muted==muted) return;
    col.muted=muted;
    dirty=true;
    engine.publish(makeLiveRenderState());
    checkpointHistory("setDataOutputMuted");
    status=muted ? "Data output muted (cue timeline remains active)."
                 : "Data output unmuted.";
    utilities.refresh();
    repaint();
}
void MainComponent::setColumnUtilities(int column,float level,float pan,bool muted,bool inverted) {
    if (column<0 || column>=(int)show.columns.size() || !std::isfinite(level) || !std::isfinite(pan)) return;
    auto& col=show.columns[column];
    col.gain=juce::jlimit(0.0f,4.0f,level); col.pan=juce::jlimit(-1.0f,1.0f,pan);
    col.muted=muted; col.phaseInverted=inverted;
    dirty=true; engine.publish(makeLiveRenderState());
    // Mapped changes and direct GUI edits share the same one-way UI sync.
    // A plain repaint cannot move a GainMeterFader's stored thumb position.
    utilities.syncAudioControls(column);
    checkpointHistory("setColumnUtilities");
    status="Column utilities updated."; repaint();
}
void MainComponent::setColumnOutput(int column,int outputIndex) {
    if (column<0 || column>=(int)show.columns.size() || show.columns[column].type!=ColumnType::audio
        || outputIndex<0 || outputIndex>1022) return;
    show.columns[column].output=outputIndex;
    show.columns[column].directOutputEnabled=true; dirty=true;
    engine.publish(makeLiveRenderState());
    checkpointHistory("setColumnOutput");
    status="Column audio output updated."; utilities.refresh(); repaint();
}
void MainComponent::stopAll() {
    cueMetrics.cancelAutoStopCellPicking(); // Esc remains PANIC, never just picker cancel.
    engine.panic(); controlOutput.clear(); resetActiveMapping();
    status="PANIC / audio, MIDI and OSC playback stopped."; repaint();
}
bool MainComponent::keyPressed(const juce::KeyPress& key) {
    // Do not use a juce::PopupMenu here. JUCE's modal MenuWindow handles
    // Escape by dismissing itself, preventing the root PANIC handler from
    // receiving that key during a show.
    if (key.getKeyCode()==juce::KeyPress::F10Key &&
        key.getModifiers().isShiftDown() &&
        !key.getModifiers().isCommandDown() &&
        !key.getModifiers().isAltDown() &&
        !dynamic_cast<juce::TextEditor*>(
            juce::Component::getCurrentlyFocusedComponent()) &&
        !show.columns.empty()) {
        toggleSelectedColumnKeyboardActions();
        return true;
    }
    if (columnKeyboardActionsOpen) {
        if (key==juce::KeyPress::escapeKey ||
            key==juce::KeyPress::spaceKey) {
            // Dismiss the visual hint, then fall through to PANIC or GO.
            dismissSelectedColumnKeyboardActions();
        } else {
            const bool modified=key.getModifiers().isCommandDown() ||
                                key.getModifiers().isShiftDown() ||
                                key.getModifiers().isAltDown();
            const int code=key.getKeyCode();
            if (!modified && ((code>='1' && code<='5') ||
                              code=='D' || code=='d'))
                return performSelectedColumnKeyboardAction(code);
            dismissSelectedColumnKeyboardActions();
            // Unrelated plain keys cancel the mode without executing a
            // structural command or accidentally deleting a cell.
            if (!modified && code!=juce::KeyPress::F10Key) return true;
        }
    }
    // Explicit modifier combination so normal Left/Right arrows and all
    // text editing remain untouched.
    if (!dynamic_cast<juce::TextEditor*>(
            juce::Component::getCurrentlyFocusedComponent()) &&
        key.getModifiers().isCommandDown() &&
        key.getModifiers().isShiftDown()) {
        if (key.getKeyCode()==juce::KeyPress::leftKey)
            return moveCuelist(show.activeCuelist,show.activeCuelist-1);
        if (key.getKeyCode()==juce::KeyPress::rightKey)
            return moveCuelist(show.activeCuelist,show.activeCuelist+1);
        if (key.getKeyCode()==juce::KeyPress::upKey)
            return moveCueRow(selectedRow,selectedRow-1);
        if (key.getKeyCode()==juce::KeyPress::downKey)
            return moveCueRow(selectedRow,selectedRow+1);
    }
    // Ctrl+R renames only the selected cell. Do not steal text-editor shortcuts.
    if (key.getModifiers().isCommandDown() &&
        !key.getModifiers().isShiftDown() &&
        !key.getModifiers().isAltDown() &&
        !dynamic_cast<juce::TextEditor*>(
            juce::Component::getCurrentlyFocusedComponent()) &&
        (key.getKeyCode()=='R' || key.getKeyCode()=='r'))
        return renameCell(selectedRow,selectedColumn);
    if (key.getModifiers().isCommandDown() && !dynamic_cast<juce::TextEditor*>(juce::Component::getCurrentlyFocusedComponent())) {
        if (key.getKeyCode()=='C' || key.getKeyCode()=='c') return copySelectedCell();
        if (key.getKeyCode()=='V' || key.getKeyCode()=='v') return pasteSelectedCell();
        if (key.getKeyCode()=='X' || key.getKeyCode()=='x') return cutSelectedCell();
    }
    if (key.getModifiers().isCommandDown() && (key.getKeyCode()=='Z' || key.getKeyCode()=='z'))
        return key.getModifiers().isShiftDown() ? redoEdit() : undoEdit();
    if (key.getModifiers().isCommandDown() && (key.getKeyCode()=='Y' || key.getKeyCode()=='y')) return redoEdit();
    if (key.getKeyCode()==juce::KeyPress::backspaceKey &&
        !dynamic_cast<juce::TextEditor*>(juce::Component::getCurrentlyFocusedComponent()))
        return deleteSelectedCell();
    if (key==juce::KeyPress::escapeKey) { stopAll(); return true; }
    if (key.getModifiers().isCommandDown() && key.getKeyCode()=='S') { saveProject(); return true; }
    if (key==juce::KeyPress::spaceKey && !dynamic_cast<juce::TextEditor*>(juce::Component::getCurrentlyFocusedComponent())) { goNext(); return true; }
    if (key==juce::KeyPress::downKey) { select(std::min(selectedRow+1,(int)show.rows.size()-1),selectedColumn); return true; }
    if (key==juce::KeyPress::upKey) { select(std::max(0,selectedRow-1),selectedColumn); return true; } return false;
}
void MainComponent::refreshSystemTelemetry() {
#if JUCE_WINDOWS
    // Query OS counters on the UI thread at 1 Hz only; never in the audio
    // callback or MIDI dispatch path. Kernel time includes idle time.
    FILETIME idle{},kernel{},user{};
    const auto ticks=[](const FILETIME& t)->uint64_t {
        return (uint64_t(t.dwHighDateTime)<<32) | uint64_t(t.dwLowDateTime);
    };
    if (::GetSystemTimes(&idle,&kernel,&user)) {
        const auto i=ticks(idle),k=ticks(kernel),u=ticks(user);
        if (previousKernelTime>0 && k>=previousKernelTime &&
            u>=previousUserTime && i>=previousIdleTime) {
            const uint64_t deltaTotal=k-previousKernelTime+u-previousUserTime;
            const uint64_t deltaIdle=i-previousIdleTime;
            if (deltaTotal>0)
                systemCpuPercent=(float)juce::jlimit(0.0,100.0,
                    100.0*(1.0-double(deltaIdle)/double(deltaTotal)));
        }
        previousIdleTime=i;previousKernelTime=k;previousUserTime=u;
    }
    MEMORYSTATUSEX memory{};
    memory.dwLength=sizeof(memory);
    if (::GlobalMemoryStatusEx(&memory))
        systemRamPercent=(float)memory.dwMemoryLoad;
#else
    // Unsupported hosts display N/A, never invented telemetry.
    systemCpuPercent=-1.f;systemRamPercent=-1.f;
#endif
}

void MainComponent::timerCallback() {
    const bool canUndo=editHistory.canUndo(),canRedo=editHistory.canRedo();
    if (undoButton.isEnabled()!=canUndo) undoButton.setEnabled(canUndo);
    if (redoButton.isEnabled()!=canRedo) redoButton.setEnabled(canRedo);
    // Keep system telemetry sampled at approximately one second intervals,
    // rather than polling Windows counters on each 25 Hz GUI refresh.
    if (++systemTelemetryTicks>=25) {
        systemTelemetryTicks=0;
        refreshSystemTelemetry();
    }
    engine.collectRetired();
    if (projectLoader) {
        PreparedProject prepared;
        if (projectLoader->takeCompleted(prepared)) {
            if (prepared.status.failed())
                message(prepared.status.getErrorMessage()+
                    " The current show has not been replaced.");
            else
                commitPreparedProject(std::move(prepared.show),prepared.source,false,
                                      std::move(prepared.media));
        }
    }
    // Checkpoint only the validated in-memory model; never copy media or
    // touch a running audio callback. The update is atomic and recoverable.
    if (recoveryEnabled && ++recoveryTicks>=25*20) {
        recoveryTicks=0;
        // "Later" must preserve the previous crash snapshot. A new
        // session must not silently overwrite it before the user decides.
        if (dirty && !preserveDeferredRecovery) {
            const auto result=recoveryWriter ?
                recoveryWriter->enqueue(show) :
                juce::Result::fail("Recovery writer unavailable.");
            if (result.failed()) {
                juce::Logger::writeToLog("Recovery checkpoint: "+result.getErrorMessage());
                status="Recovery checkpoint unavailable. Save your project.";
            }
        }
    }
    // Consume transport confirmation before hardware events in this UI tick.
    // Empty cue GO is acknowledged too; merely selecting a row is not.
    const int confirmed=engine.acknowledgedCueId.load(std::memory_order_acquire);
    if (confirmed!=activeCueId) {
        activeCueId=0; int preset=0;
        if (confirmed>0) for (const auto& row:liveShowSnapshot().rows)
            if (row.id==confirmed) {
                activeCueId=confirmed; preset=row.mappingPresetId; break;
            }
        controlMapping.setActiveCue(activeCueId,preset);
    }
    ControlEvent control; int processed=0;
    const bool consumeMidiTick=quickLearnMode==QuickLearnMode::midi;
    const auto receiveMidi=[this,consumeMidiTick](const ControlEvent& control) {
        if (consumeMidiTick) {
            if (quickLearnFunctionId>0 && controlLearnArmed &&
                !(control.kind==ControlKind::note && control.isNoteOff)) {
                controlLearnArmed=false;completeQuickLearn(control);
            } else physicalEdges.recordAndWasDown(control);
        } else if (!consumeMidiLearnEvent(controlLearnArmed,learnedInput,control))
            dispatchControl(control);
    };
    while (processed++<256 && midiInput.poll(control)) receiveMidi(control);
    for (auto& extra:additionalMidiInputs) {
        int received=0;
        while (received++<256 && extra.second->poll(control)) receiveMidi(control);
    }

    int oscProcessed=0;
    const bool consumeOscTick=quickLearnMode==QuickLearnMode::osc;
    while (oscProcessed++<256 && oscInput.poll(control)) {
        if (consumeOscTick) {
            if (quickLearnFunctionId>0 && oscLearnArmed) {
                oscLearnArmed=false;
                completeQuickLearn(control);
            }
        } else if (oscLearnArmed) {
            learnedInput=control;oscLearnArmed=false;
        } else dispatchControl(control);
    }
    if (++midiRefreshTicks>=25) {
        midiRefreshTicks=0;
        // Reapply persisted output switches for devices connected after startup.
        if (appPreferences) for (const auto& dev:juce::MidiOutput::getAvailableDevices())
            engine.setMidiOutputEnabled(dev.identifier,
                appPreferences->getBoolValue("midiOutputEnabled_"+dev.identifier,true));
        engine.refreshMidiOutputs();
        // A MIDI Output chosen while disconnected had no registered route.
        // Register it only if that EXACT port reappears; republish the
        // prepared snapshot so its MIDI voice receives the new route without
        // interrupting Audio/OSC/row time. Existing routes are reconnected
        // internally by refreshMidiOutputs() and need no audio reset.
        const auto availableOutputs=juce::MidiOutput::getAvailableDevices();
        bool restoredRoute=false;
        for (const auto& col:show.columns) {
            if (col.type!=ColumnType::midi || col.midiOutputId.isEmpty() ||
                engine.midiOutputRoute(col.midiOutputId)>=0 ||
                !hasExactMidiIdentifier(availableOutputs,col.midiOutputId))continue;
            if (engine.setMidiOutput(col.midiOutputId).wasOk())
                restoredRoute=true;
        }
        if(restoredRoute) engine.publish(makeLiveRenderState());
        // Take one inventory snapshot per refresh. The policy checks exact
        // device identifiers and never substitutes a similarly named port.
        const auto devices=juce::MidiInput::getAvailableDevices();
        for (auto& extra:additionalMidiInputs) {
            if (!hasExactMidiIdentifier(devices,extra.first)) {
                if (extra.second->connected()) {
                    extra.second->disconnect();
                    physicalEdges.clear();controlMapping.clearPickup();
                }
            } else if (!extra.second->connected()) {
                if (extra.second->connect(extra.first).wasOk()) {
                    physicalEdges.clear();controlMapping.clearPickup();
                }
            }
        }
        const auto firstAction=planMidiSession(show.controls.midiInputId,
            midiInput.identifier(),midiInput.connected(),devices);
        if (firstAction==MidiSessionAction::disconnectMissing) {
            midiInput.disconnect();
            physicalEdges.clear();controlMapping.clearPickup(); // stale MIDI state cannot survive unplug
            status="MIDI input disconnected. Mappings suspended.";
        }
        // Preserve the original same-tick disconnect/reconnect flow, if a
        // different explicitly selected device is already present.
        if (planMidiSession(show.controls.midiInputId,midiInput.identifier(),
                midiInput.connected(),devices)==MidiSessionAction::reconnectSelected) {
            ++midiInputReconnectAttempts;
            const auto restored=midiInput.connect(show.controls.midiInputId);
            if (restored.wasOk()) {
                physicalEdges.clear();controlMapping.clearPickup();
                status="MIDI controller reconnected (same device).";
            }
        }
    }
    // Connection status can change while a function remains selected.
    // Keep the yellow blinking target armed, but show a truthful live
    // warning until its explicitly selected MIDI input becomes available.
    if (quickLearnMode==QuickLearnMode::midi && quickLearnFunctionId>0) {
        const juce::String warning=midiInput.connected() ? juce::String() :
                                   juce::String("MIDI input not connected");
        if (quickLearnFeedback!=warning) {
            quickLearnFeedback=warning;
            repaint();
        }
    }
    // Compare monotonic counters; LEDs reflect actual RX/TX, not queued timeline events.
    const auto diag=engine.diagnostics();
    const uint64_t rx=midiInput.receivedCount();
    const uint64_t tx=diag.midiOutputSent;
    const uint64_t oscTx=static_cast<uint64_t>(diag.oscSent)+controlOutput.sentCount();
    const uint64_t oscRx=oscInput.receivedCount();
    const double now=juce::Time::getMillisecondCounterHiRes();
    if(activityInitialized) {
        if(rx>lastMidiRx) midiRxUntil=now+170.0;
        if(tx>lastMidiTx) midiTxUntil=now+170.0;
        if(oscTx>lastOscTx) oscTxUntil=now+170.0;
        if(oscRx>lastOscRx) oscRxUntil=now+170.0;
    }
    lastMidiRx=rx; lastMidiTx=tx; lastOscTx=oscTx; lastOscRx=oscRx;
    activityInitialized=true;
    // The protocol stays visibly ON between assignments, even when
    // there is no currently selected target waiting for its next event.
    globalMidiLearn.setColour(juce::TextButton::buttonColourId,
        (quickLearnMode==QuickLearnMode::midi || controlLearnArmed) ?
        palette::amber.withAlpha(.55f) : palette::amber.withAlpha(.13f));
    globalOscLearn.setColour(juce::TextButton::buttonColourId,
        (quickLearnMode==QuickLearnMode::osc || oscLearnArmed) ?
        palette::purple.withAlpha(.55f) : palette::purple.withAlpha(.13f));
    // 25 Hz UI refresh: only the armed target pulses (~1.8 Hz).
    // Other available controls retain their steady protocol-coloured tint.
    if (quickLearnFunctionId>0 && quickLearnMode!=QuickLearnMode::off &&
        ++quickLearnPulseTicks>=7) {
        quickLearnPulseTicks=0;
        quickLearnPulseBright=!quickLearnPulseBright;
        updateQuickLearnVisuals();
    }
    // Keep the full untruncated LIVE target discoverable on the existing
    // native GO button. A 25Hz UI tick handles cue advances without touching
    // the audio callback or queue.
    const auto nextLiveGo=nextLiveGoCueForUi();
    const auto goTooltip="Global GO / Space targets LIVE: "+liveCuelistNameForUi()+
                         (nextLiveGo=="END" ? juce::String(" / END OF LIVE") :
                                              juce::String(" / cue ")+nextLiveGo);
    if (goButton.getTooltip()!=goTooltip) goButton.setTooltip(goTooltip);
    utilities.syncScroll(viewport.getViewPositionX()); utilities.refreshRouting();
    grid.repaint(); cueHeaders.repaint(); columnHeaders.repaint();
    displayPeak=std::max(engine.peak.load(),displayPeak*.88f);
    utilities.refreshMeters(); repaint(); timeline.repaint(); cueMetrics.repaint();
}
void MainComponent::showDevices() {
    if (deviceDialog) { deviceDialog->toFront(true); return; }
    stopAll(); auto* selector=new juce::AudioDeviceSelectorComponent(engine.devices,0,0,0,64,false,false,true,false);
    selector->setSize(600,450);
    juce::DialogWindow::LaunchOptions options; options.content.setOwned(selector); options.dialogTitle="Audio devices / playback stopped";
    options.dialogBackgroundColour=palette::panel; options.escapeKeyTriggersCloseButton=true; options.useNativeTitleBar=true;
    options.componentToCentreAround=this; options.resizable=false; deviceDialog=options.launchAsync(); deviceDialog->setLookAndFeel(&theme);
}
}
