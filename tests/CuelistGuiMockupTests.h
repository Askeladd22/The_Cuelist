#pragma once
#include "ProjectMediaLoader.h"

void cuelistGuiMockupTests() {
    test("LIVE audio and armed GO survive cue add, row move and column reorder",[] {
        MainComponent ui(false);
        // 12 seconds at 100 Hz, long enough to exercise several edits.
        auto audio=std::make_shared<AudioClip>();
        audio->sampleRate=100;audio->samples.setSize(1,1200);
        for(int i=0;i<1200;++i)audio->samples.setSample(0,i,.3f);
        const int audioCell=ui.show.rows[0].cells[0].id;
        ui.clips[audioCell]=audio;
        addRow(ui.show,"Armed second cue");
        ui.engine.connected.store(true);
        ui.triggerRow(0);
        float buffer[20]{};
        float* outputs[]{buffer};
        // Mimic MainComponent::timerCallback: reclaim retired snapshots before
        // the audio thread is asked to adopt another live render publication.
        ui.engine.collectRetired();
        ui.engine.processBlock(outputs,1,20,100);
        const double initial=ui.engine.getCellPlayback(audioCell).positionSeconds;
        const int armedId=ui.show.rows[(size_t)ui.liveNextCueIndex()].id;
        check(initial>0 && ui.engine.getCellPlayback(audioCell).playing,
              "Baseline audio cue did not start");
        ui.addCueForTest();
        // Mimic MainComponent::timerCallback: reclaim retired snapshots before
        // the audio thread is asked to adopt another live render publication.
        ui.engine.collectRetired();
        ui.engine.processBlock(outputs,1,20,100);
        check(ui.engine.getCellPlayback(audioCell).playing &&
              ui.engine.getCellPlayback(audioCell).positionSeconds>initial,
              "Adding a cue interrupted playback or reset its position");
        check(ui.show.rows[(size_t)ui.liveNextCueIndex()].id==armedId,
              "Adding a cue changed the stable armed GO identity");
        check(ui.moveCueRow(1,0),"Cue drag reorder did not succeed");
        // Mimic MainComponent::timerCallback: reclaim retired snapshots before
        // the audio thread is asked to adopt another live render publication.
        ui.engine.collectRetired();
        ui.engine.processBlock(outputs,1,20,100);
        check(ui.engine.getCellPlayback(audioCell).playing &&
              ui.show.rows[(size_t)ui.liveNextCueIndex()].id==armedId,
              "Cue reorder stopped audio or changed the next armed cue");
        addColumn(ui.show,ColumnType::midi,{});
        ui.commitModelEditForTest("MIDI column added");
        // Mimic MainComponent::timerCallback: reclaim retired snapshots before
        // the audio thread is asked to adopt another live render publication.
        ui.engine.collectRetired();
        ui.engine.processBlock(outputs,1,20,100);
        check(ui.engine.getCellPlayback(audioCell).playing,
              "Adding a column interrupted active audio");
        ui.reorderColumn(0,1);
        // Mimic MainComponent::timerCallback: reclaim retired snapshots before
        // the audio thread is asked to adopt another live render publication.
        ui.engine.collectRetired();
        ui.engine.processBlock(outputs,1,20,100);
        check(ui.engine.getCellPlayback(audioCell).playing &&
              ui.engine.getCellPlayback(audioCell).positionSeconds>initial+.6,
              "Column reorder lost the active audio voice and position");
        check(ui.engine.acknowledgedCueId.load()==ui.show.rows[1].id,
              "Structural edits unexpectedly reset the acknowledged LIVE cue");
    });
    test("Selecting unavailable MIDI output preserves running audio and cue clock",[] {
        MainComponent ui(false);
        auto audio=std::make_shared<AudioClip>();
        audio->sampleRate=100;audio->samples.setSize(1,1200);
        for(int i=0;i<1200;++i)audio->samples.setSample(0,i,.2f);
        const int id=ui.show.rows[0].cells[0].id;
        ui.clips[id]=audio;
        addColumn(ui.show,ColumnType::midi,{});
        ui.engine.connected.store(true);
        ui.triggerRow(0);
        float buffer[30]{};
        float* out[]{buffer};
        ui.engine.collectRetired();
        ui.engine.processBlock(out,1,30,100);
        const double before=ui.engine.getCellPlayback(id).positionSeconds;
        const int runningId=ui.engine.acknowledgedCueId.load();
        const int midiIndex=(int)ui.show.columns.size()-1;
        ui.setMidiOutput(midiIndex,"intentionally-missing-device-0924");
        ui.engine.collectRetired();
        ui.engine.processBlock(out,1,30,100);
        check(ui.show.columns[(size_t)midiIndex].midiOutputId==
                "intentionally-missing-device-0924" &&
              ui.engine.getCellPlayback(id).playing &&
              ui.engine.getCellPlayback(id).positionSeconds>before &&
              ui.engine.acknowledgedCueId.load()==runningId,
              "Unplugged MIDI selection stopped or restarted the live audio cue");
        ui.setMidiOutput(midiIndex,{});
        ui.engine.collectRetired();
        ui.engine.processBlock(out,1,30,100);
        check(ui.engine.getCellPlayback(id).playing &&
              ui.engine.getCellPlayback(id).positionSeconds>before+.3 &&
              ui.engine.acknowledgedCueId.load()==runningId,
              "Clearing MIDI output during GO interrupted audio transport");
    });
    test("VIEW Cuelist switches without stopping the live transport", [] {
        MainComponent ui(false);
        addRow(ui.show,"Following live cue");
        auto clip=std::make_shared<AudioClip>();
        clip->sampleRate=100;clip->samples.setSize(1,100);
        for(int i=0;i<100;++i)clip->samples.setSample(0,i,.2f);
        const int liveFirst=ui.show.rows[0].id;
        const int liveSecond=ui.show.rows[1].id;
        const int audioCell=ui.show.rows[0].cells[0].id;
        ui.clips[audioCell]=clip;
        ui.engine.connected.store(true);
        ui.triggerRow(0); // arms liveNextCue=1, like the real operator GO
        check(ui.liveNextCueIndex()==1,"Initial live transport was not armed");
        ui.engine.processBlock(nullptr,0,1,100);
        check(ui.engine.getCellPlayback(audioCell).playing,
              "Baseline live audio did not start");
        ui.createCuelist(); // select a different VIEW, do not touch live playback
        check(ui.show.activeCuelist==1 && ui.liveCuelistIndex()==0,
              "VIEW switch unexpectedly changed live Cuelist");
        check(ui.engine.getCellPlayback(audioCell).playing,
              "VIEW switch silenced active audio");
        check(ui.show.rows.front().id!=liveFirst,
              "Cuelist VIEW did not actually change");
        ui.goNext(); // Transport GO still targets Cuelist 1, not VIEW Cuelist 2
        ui.engine.processBlock(nullptr,0,1,100);
        check(ui.engine.acknowledgedCueId.load()==liveSecond,
              "Global GO targeted the VIEW tab instead of the LIVE tab");
        check(ui.liveNextCueIndex()==2,
              "Independent live cursor did not advance");
        ui.activateCuelist(0);
        check(ui.liveCuelistIndex()==ui.show.activeCuelist &&
              ui.engine.acknowledgedCueId.load()==liveSecond,
              "Returning to LIVE view lost the transport or its cue");
    });

    // RQ-001 regression: independent VIEW and LIVE transport cursors.
    // The native file-picker scenario runs LAST in Tests.cpp to prevent an
    // asynchronous modal dialog from influencing subsequent UI click tests.
    test("RQ001 T01: Cell Play preserves armed LIVE cue across VIEW change", [] {
        MainComponent ui(false);
        for (int i=1;i<4;++i) addRow(ui.show,"Cue "+juce::String(i+1));
        auto audio=std::make_shared<AudioClip>();
        audio->sampleRate=100;
        audio->samples.setSize(1,1200);
        for (int i=0;i<1200;++i) audio->samples.setSample(0,i,.2f);
        const int playedCell=ui.show.rows[3].cells[0].id;
        ui.clips[playedCell]=audio;
        ui.engine.connected.store(true);
        ui.engine.publish(ui.makeLiveRenderState());
        check(ui.select(1,0),"T01 setup: cannot arm cue 2");
        const int armedCueId=ui.show.rows[1].id;
        check(ui.liveNextCueIndex()==1,"T01 setup: wrong armed cue");
        // Direct Cell Play must not change the next GO for the same LIVE list.
        ui.triggerCellAt(3,0);
        check(ui.liveNextCueIndex()==1,
              "T01: direct Cell Play unexpectedly changed the LIVE GO cursor");
        ui.createCuelist(); // Switch VIEW, leave LIVE on the original list.
        check(ui.liveCuelistIndex()==0 && ui.show.activeCuelist==1,
              "T01 setup: VIEW and LIVE did not separate");
        ui.goNext();
        ui.engine.collectRetired();
        ui.engine.processBlock(nullptr,0,1,100);
        check(ui.engine.acknowledgedCueId.load()==armedCueId,
              "T01: Space/GO from VIEW launched the wrong LIVE cue");
    });

    test("RQ001: loaded Cell Play in VIEW transfers LIVE and starts Audio", [] {
        MainComponent ui(false);
        auto makeAudio=[] {
            auto audio=std::make_shared<AudioClip>();
            audio->sampleRate=100;
            audio->samples.setSize(1,1200);
            for (int i=0;i<1200;++i) audio->samples.setSample(0,i,.2f);
            return audio;
        };
        const int firstCell=ui.show.rows[0].cells[0].id;
        ui.clips[firstCell]=makeAudio();
        ui.engine.connected.store(true);
        ui.engine.publish(ui.makeLiveRenderState());
        ui.triggerRow(0);
        ui.engine.processBlock(nullptr,0,1,100);
        check(ui.engine.getCellPlayback(firstCell).playing,
              "RQ001 transfer setup: LIVE A audio did not start");
        ui.createCuelist();
        const int secondCell=ui.show.rows[0].cells[0].id;
        ui.clips[secondCell]=makeAudio();
        check(ui.liveCuelistIndex()!=ui.show.activeCuelist,
              "RQ001 transfer setup: VIEW did not separate from LIVE");
        ui.triggerCell();
        ui.engine.collectRetired();
        ui.engine.processBlock(nullptr,0,1,100);
        check(ui.liveCuelistIndex()==ui.show.activeCuelist,
              "RQ001: explicit Cell Play failed to transfer LIVE");
        check(ui.engine.getCellPlayback(secondCell).playing,
              "RQ001: transferred LIVE did not publish the selected Audio clip");
        check(!ui.engine.getCellPlayback(firstCell).playing,
              "RQ001: old LIVE Audio continued after explicit transfer");
    });

    using namespace cueViewFixtures;
    test("Cue notepad survives switching Cuelists and v10 persistence", [] {
        MainComponent ui(false);
        auto* cue=dynamic_cast<juce::TextEditor*>(controlsV3::named(ui,"cueName"));
        auto* notes=dynamic_cast<juce::TextEditor*>(controlsV3::named(ui,"cueNotes"));
        auto* plus=dynamic_cast<juce::TextButton*>(controlsV3::named(ui,"newCuelist"));
        check(cue && notes && plus,"Cue editor, notes or Cuelist plus missing");
        cue->setText("Preshow cue",false);
        notes->setText("Stand by the conductor\nOpen the main curtain",false);
        ui.createCuelist();
        check(ui.show.activeCuelist==1 && ui.show.cuelists.size()==2,
              "Creating Cuelist did not select a second cue list");
        check(ui.show.rows.size()==1 && ui.show.rows[0].name.isEmpty(),
              "New Cuelist did not begin with an independent blank cue");
        notes->setText("Musical notepad",false);
        ui.activateCuelist(0);
        check(ui.show.rows[0].name=="Preshow cue" &&
              ui.show.rows[0].notes.contains("conductor"),
              "Changing Cuelist lost the original cue and notes");
        check(notes->getText().contains("conductor"),
              "Inspector notepad failed to follow the active Cuelist");
        ui.activateCuelist(1);
        check(ui.show.rows[0].notes=="Musical notepad",
              "Second Cuelist note content was lost on return");
        const auto snapshot=encodeShow(ui.show);
        Show restored;
        check(decodeShow(snapshot,restored).wasOk(),"Multi-Cuelist v10 file rejected");
        check(restored.activeCuelist==1 && restored.cuelists.size()==2 &&
              restored.cuelists[0].rows[0].notes.contains("conductor") &&
              restored.rows[0].notes=="Musical notepad",
              "Cuelist names, selection or cue notes failed round-trip");
    });

    test("Drag reorders Cuelist tabs without activating or stopping the active list",[] {
        MainComponent ui(false);
        ui.createCuelist();
        ui.createCuelist();
        ui.setSize(1440,900);
        const int activeCue=ui.show.rows[0].id;
        const auto originalOrder=juce::StringArray{
            ui.show.cuelists[0].name,ui.show.cuelists[1].name,
            ui.show.cuelists[2].name};
        auto* first=dynamic_cast<CuelistTabButton*>(
            controlsV3::named(ui,"cuelistTab0"));
        auto* third=dynamic_cast<CuelistTabButton*>(
            controlsV3::named(ui,"cuelistTab2"));
        check(first && third && first->isVisible() && third->isVisible(),
              "Reorder test requires three visible draggable tabs.");
        const juce::Point<float> start{12.0f,13.0f};
        const juce::Point<float> drop{
            (float)(third->getBounds().getCentreX()-first->getX()),13.0f};
        first->mouseDown(event(*first,start,start));
        first->mouseDrag(event(*first,drop,start,1,true));
        first->mouseUp(event(*first,drop,start,1,true));
        check(ui.show.cuelists[0].name==originalOrder[1] &&
              ui.show.cuelists[1].name==originalOrder[2] &&
              ui.show.cuelists[2].name==originalOrder[0] &&
              ui.show.activeCuelist==1 &&
              ui.show.rows[0].id==activeCue,
              "Tab drag failed to reorder or accidentally changed active Cuelist.");
        check(validateShow(ui.show).wasOk(),
              "Reordering tabs must retain valid independent cue lists.");
        Show decoded;
        check(decodeShow(encodeShow(ui.show),decoded).wasOk() &&
              decoded.cuelists[2].name==originalOrder[0] &&
              decoded.activeCuelist==1 && decoded.rows[0].id==activeCue,
              "Dragged Cuelist order did not persist in project data.");
    });
    test("Cuelists move by dedicated arrows and keyboard shortcut",[] {
        MainComponent ui(false);
        ui.createCuelist();
        ui.createCuelist();
        auto* left=dynamic_cast<juce::TextButton*>(
            controlsV3::named(ui,"cuelistMoveLeft"));
        auto* right=dynamic_cast<juce::TextButton*>(
            controlsV3::named(ui,"cuelistMoveRight"));
        check(left && right && left->isEnabled() && !right->isEnabled(),
              "Cuelist arrows should reflect active tab boundaries.");
        const int cueId=ui.show.rows[0].id;
        click(*left,10.f,10.f);
        check(ui.show.activeCuelist==1 && ui.show.rows[0].id==cueId &&
              ui.show.cuelists[1].name=="Cuelist 3" &&
              left->isEnabled() && right->isEnabled(),
              "Left arrow failed to move the active tab one place.");
        const juce::ModifierKeys shortcut(
            juce::ModifierKeys::ctrlModifier |
            juce::ModifierKeys::shiftModifier);
        check(ui.keyPressed(juce::KeyPress(juce::KeyPress::leftKey,shortcut,0)) &&
              ui.show.activeCuelist==0 && ui.show.rows[0].id==cueId,
              "Ctrl+Shift+Left must move the active Cuelist without changing cues.");
        check(ui.keyPressed(juce::KeyPress(juce::KeyPress::rightKey,shortcut,0)) &&
              ui.show.activeCuelist==1 && ui.show.rows[0].id==cueId,
              "Ctrl+Shift+Right must restore the active Cuelist ordering.");
        click(*right,10.f,10.f);
        check(ui.show.activeCuelist==2 && ui.show.rows[0].id==cueId &&
              !right->isEnabled(),"Right arrow should stop at final Cuelist.");
        check(!ui.moveCuelist(2,3) && !ui.moveCuelist(-1,0),
              "Invalid move indices must not change the show.");
    });

    test("Drag cues within the Cuelist and keyboard arrows preserve their cell identities",[] {
        MainComponent ui(false);
        addRow(ui.show,"Second");
        addRow(ui.show,"Third");
        const int firstId=ui.show.rows[0].id;
        const int secondId=ui.show.rows[1].id;
        const int thirdId=ui.show.rows[2].id;
        const int firstCell=ui.show.rows[0].cells[0].id;
        auto* pinned=cueViewFixtures::find<PinnedCueHeaders>(ui);
        check(pinned!=nullptr,"Pinned cue header missing.");
        const juce::Point<float> from{156.0f,
            (float)CueGrid::headerHeight+CueGrid::rowContentInset+20};
        const juce::Point<float> to{156.0f,
            from.y+2*CueGrid::rowHeight};
        pinned->mouseDown(event(*pinned,from,from));
        pinned->mouseDrag(event(*pinned,to,from,1,true));
        pinned->mouseUp(event(*pinned,to,from,1,true));
        check(ui.show.rows[0].id==secondId &&
              ui.show.rows[1].id==thirdId &&
              ui.show.rows[2].id==firstId &&
              ui.show.rows[2].cells[0].id==firstCell &&
              ui.selectedRow==2,
              "Cue drag failed to move the complete row with its cells.");
        const juce::ModifierKeys mods(
            juce::ModifierKeys::ctrlModifier |
            juce::ModifierKeys::shiftModifier);
        check(ui.keyPressed(juce::KeyPress(juce::KeyPress::upKey,mods,0)) &&
              ui.selectedRow==1 && ui.show.rows[1].id==firstId,
              "Ctrl+Shift+Up did not move the selected cue.");
        check(ui.keyPressed(juce::KeyPress(juce::KeyPress::downKey,mods,0)) &&
              ui.selectedRow==2 && ui.show.rows[2].id==firstId,
              "Ctrl+Shift+Down did not move the selected cue.");
        check(validateShow(ui.show).wasOk(),
              "Cue row reordering invalidated project model.");
        Show restored;
        check(decodeShow(encodeShow(ui.show),restored).wasOk() &&
              restored.rows[2].id==firstId,
              "Cue drag order was not persisted.");
    });

    test("Cuelist names update active tab and Cue List header bidirectionally",[] {
        MainComponent ui(false);
        ui.createCuelist();
        check(ui.setCuelistName(1,"Act I - Musical"),
              "Active Cuelist rename was rejected");
        auto* second=dynamic_cast<juce::TextButton*>(
            controlsV3::named(ui,"cuelistTab1"));
        check(second && second->getButtonText().startsWith("Act I") &&
              ui.show.cuelists[1].name=="Act I - Musical",
              "Renaming the active heading did not update the selected tab");
        check(ui.setCuelistName(0,"Pre-show"),
              "An inactive Cuelist could not be renamed from its tab");
        auto* first=dynamic_cast<juce::TextButton*>(
            controlsV3::named(ui,"cuelistTab0"));
        check(first && first->getButtonText()=="Pre-show",
              "Renaming an inactive tab did not update its label");
        ui.activateCuelist(0);
        check(ui.show.cuelists[ui.show.activeCuelist].name=="Pre-show",
              "Switching to the renamed tab did not update the Cue List heading");
        check(!ui.setCuelistName(0,"Act I - Musical") &&
              !ui.setCuelistName(0,"  ") &&
              !ui.setCuelistName(0,juce::String::repeatedString("X",65)),
              "Duplicate, blank or oversized Cuelist names should be rejected");
        check(ui.show.cuelists[0].name=="Pre-show",
              "Invalid rename overwrote the original name");
        Show restored;
        check(decodeShow(encodeShow(ui.show),restored).wasOk() &&
              restored.cuelists[0].name=="Pre-show" &&
              restored.cuelists[1].name=="Act I - Musical",
              "Bidirectional Cuelist names did not persist to the project");
    });

    test("Cuelist tabs stay consecutive and inline until the real width limit",[] {
        MainComponent ui(false);
        check(ui.show.cuelists.size()==1 &&
              ui.show.cuelists[0].name=="Cuelist 1","Default tab name incorrect");
        for (int i=2;i<=9;++i) {
            ui.createCuelist();
            check(ui.show.cuelists.back().name=="Cuelist "+juce::String(i),
                  "Cuelist numbering skipped a value");
        }
        // Nine tabs remain inline when there is genuine space to the
        // left of the enlarged monitoring console.
        ui.setSize(1920,1080);
        ui.resized();
        int previousX=-1;
        for (int i=0;i<9;++i) {
            auto* tab=controlsV3::named(ui,"cuelistTab"+juce::String(i));
            auto* close=controlsV3::named(ui,"deleteCuelist"+juce::String(i));
            check(tab && close && tab->isVisible() && close->isVisible(),
                  "Every Cuelist must have an inline tab when space allows");
            check(tab->getX()>previousX,"Cuelist tabs are not consecutive");
            previousX=tab->getX();
        }
        auto* overflow=controlsV3::named(ui,"cuelistOverflow");
        check(overflow && !overflow->isVisible(),
              "Overflow appeared despite room for nine tabs at 1920px");
        // At 1647px telemetry has priority over the tab row. Hidden
        // Cuelists stay reachable using the original overflow action.
        ui.setSize(1647,895);
        ui.resized();
        check(overflow->isVisible() &&
              controlsV3::named(ui,"cuelistTab8")->isVisible(),
              "Wider telemetry obscured the active tab instead of overflowing");
        const auto edge=HeaderTelemetryLayout::atWidth(1647).clock.getX();
        for (int i=0;i<9;++i) {
            auto* tab=controlsV3::named(ui,"cuelistTab"+juce::String(i));
            if (tab->isVisible())
                check(tab->getRight()<edge,
                      "An inline Cuelist tab overlaps operator telemetry");
        }
        ui.setSize(1180,820);
        ui.resized();
        check(overflow->isVisible(),"Narrow windows must retain access to hidden Cuelists");
        check(controlsV3::named(ui,"cuelistTab8")->isVisible(),
              "Currently active Cuelist must remain visible when tabs overflow");
        ui.activateCuelist(0);
        check(controlsV3::named(ui,"cuelistTab0")->isVisible() &&
              controlsV3::named(ui,"cuelistTab1")->isVisible(),
              "Overflow scrolling skipped sequential Cuelists");
    });

    test("RQ001 T03: hidden LIVE still receives GO while another tab is viewed",[] {
        MainComponent ui(false);
        addRow(ui.show,"Next LIVE cue");
        const int nextLiveCueId=ui.show.rows[1].id;
        ui.engine.connected.store(true);
        ui.triggerRow(0);
        ui.engine.processBlock(nullptr,0,1,100);
        check(ui.liveNextCueIndex()==1,
              "T03 setup: second cue in LIVE was not armed");
        for(int i=1;i<9;++i) ui.createCuelist();
        ui.setSize(1180,820);
        ui.resized();
        auto* liveTab=controlsV3::named(ui,"cuelistTab0");
        auto* viewTab=controlsV3::named(ui,"cuelistTab8");
        auto* overflow=controlsV3::named(ui,"cuelistOverflow");
        check(liveTab && viewTab && overflow,
              "T03: native tab or overflow control is missing");
        check(!liveTab->isVisible() && viewTab->isVisible() &&
              overflow->isVisible(),
              "T03: compact VIEW did not hide the LIVE tab in overflow");
        check(ui.liveCuelistIndex()==0 && ui.show.activeCuelist==8,
              "T03: browsing changed LIVE ownership");
        ui.goNext();
        ui.engine.collectRetired();
        ui.engine.processBlock(nullptr,0,1,100);
        check(ui.engine.acknowledgedCueId.load()==nextLiveCueId,
              "T03: global GO targeted the browsed VIEW, not hidden LIVE");
        check(ui.liveCuelistIndex()==0 && ui.show.activeCuelist==8,
              "T03: GO unexpectedly switched the visible tab");
        ui.setSize(1920,1080);
        ui.resized();
        check(liveTab->isVisible() && viewTab->isVisible() &&
              !overflow->isVisible(),
              "T03: wide comparison must expose LIVE and VIEW together");
        // controlsV3::named returns a juce::Component*. TooltipClient
        // is implemented by the actual native TextButton, not Component.
        auto* liveButton=dynamic_cast<juce::TextButton*>(liveTab);
        check(liveButton && liveButton->getTooltip().contains("[LIVE]"),
              "T03: visible LIVE tab has lost its textual identity");
    });

    test("RQ001 T03B: persistent LIVE identity and hidden-tab discovery survive GO",[] {
        MainComponent ui(false);
        check(ui.setCuelistName(0,"Pre-show"),
              "T03B setup: cannot name LIVE Cuelist");
        addRow(ui.show,"Next LIVE cue");
        const int liveSecond=ui.show.rows[1].id;
        ui.engine.connected.store(true);
        ui.triggerRow(0);
        ui.engine.processBlock(nullptr,0,1,100);
        check(ui.liveCuelistNameForUi()=="Pre-show" &&
              ui.nextLiveGoCueForUi()=="02",
              "T03B: LIVE label does not represent the armed cue");
        for(int i=1;i<9;++i) ui.createCuelist();
        check(ui.setCuelistName(8,"Emergency"),
              "T03B setup: cannot name VIEW Cuelist");
        ui.setSize(1180,820);
        ui.resized();
        auto* overflow=dynamic_cast<juce::TextButton*>(
            controlsV3::named(ui,"cuelistOverflow"));
        check(overflow && overflow->isVisible() &&
              overflow->getTooltip().contains("Pre-show"),
              "T03B: hidden LIVE has no textual discoverability in overflow");
        check(ui.show.activeCuelist==8 && ui.liveCuelistIndex()==0 &&
              ui.liveCuelistNameForUi()=="Pre-show" &&
              ui.nextLiveGoCueForUi()=="02",
              "T03B: VIEW switched the LIVE label or the armed GO target");
        const auto snapshot=ui.createComponentSnapshot(
            {12,ui.getHeight()-339,156,51});
        check(snapshot.isValid() && snapshot.getWidth()==156 &&
              snapshot.getHeight()==51,
              "T03B: native ON DECK surface cannot be rendered");
        ui.goNext();
        ui.engine.collectRetired();
        ui.engine.processBlock(nullptr,0,1,100);
        check(ui.engine.acknowledgedCueId.load()==liveSecond &&
              ui.show.activeCuelist==8 &&
              ui.liveCuelistNameForUi()=="Pre-show" &&
              ui.nextLiveGoCueForUi()=="END",
              "T03B: GO from VIEW failed to refresh LIVE target/end-of-sequence");
        check(ui.setCuelistName(0,"Pre-show renamed") &&
              ui.liveCuelistNameForUi()=="Pre-show renamed" &&
              overflow->getTooltip().contains("Pre-show renamed"),
              "T03B: a LIVE rename did not refresh hidden-tab identity");
        ui.activateCuelist(0);
        check(ui.show.activeCuelist==ui.liveCuelistIndex() &&
              ui.nextLiveGoCueForUi()=="END",
              "T03B: returning to LIVE changed its GO cursor");
        ui.select(0,0);
        check(ui.nextLiveGoCueForUi()=="01",
              "T03B: explicit LIVE selection did not rearm the displayed GO");
    });

    test("RQ001 T04: global GO preserves LIVE while native Row GO takes over VIEW",[] {
        MainComponent ui(false);
        check(ui.setCuelistName(0,"Act I"),
              "T04 setup: cannot name original LIVE");
        addRow(ui.show,"Second LIVE cue");
        const int liveNextId=ui.show.rows[1].id;
        ui.engine.connected.store(true);
        ui.triggerRow(0);
        ui.engine.collectRetired();
        ui.engine.processBlock(nullptr,0,1,100);
        check(ui.liveCuelistIndex()==0 && ui.liveNextCueIndex()==1,
              "T04 setup: wrong initial LIVE cursor");
        ui.createCuelist();
        check(ui.setCuelistName(1,"Emergency"),
              "T04 setup: cannot name browsed VIEW");
        const int viewedCueId=ui.show.rows[0].id;
        ui.setSize(1180,820);
        ui.resized();
        auto* pinned=cueViewFixtures::find<PinnedCueHeaders>(ui);
        check(pinned!=nullptr,"T04: missing native pinned row GO surface");
        const auto go=CueGrid::rowGoBounds(CueGrid::headerHeight);
        const auto x=(float)go.getCentreX();
        const auto y=(float)(go.getCentreY()+CueGrid::rowContentInset);
        pinned->mouseMove(cueViewFixtures::event(*pinned,{x,y},{x,y}));
        check(pinned->getTooltip().contains("stop LIVE Act I") &&
              pinned->getTooltip().contains("Emergency"),
              "T04: native Row GO does not warn about LIVE takeover");

        // Global Space/GO remains on Act I even though Emergency is VIEW.
        ui.goNext();
        ui.engine.collectRetired();
        ui.engine.processBlock(nullptr,0,1,100);
        check(ui.engine.acknowledgedCueId.load()==liveNextId &&
              ui.liveCuelistIndex()==0 && ui.show.activeCuelist==1,
              "T04: global GO incorrectly took ownership of browsed VIEW");
        check(ui.nextLiveGoCueForUi()=="END",
              "T04: global GO did not advance the independent LIVE cursor");

        // Click the genuinely painted Row GO target in the pinned JUCE
        // overlay; do not call triggerRow() as a substitute for native input.
        cueViewFixtures::click(*pinned,x,y);
        ui.engine.collectRetired();
        ui.engine.processBlock(nullptr,0,1,100);
        check(ui.engine.acknowledgedCueId.load()==viewedCueId &&
              ui.liveCuelistIndex()==1 && ui.show.activeCuelist==1,
              "T04: native Row GO did not intentionally transfer LIVE");
        pinned->mouseMove(cueViewFixtures::event(*pinned,{x,y},{x,y}));
        check(pinned->getTooltip().contains("on LIVE Emergency"),
              "T04: Row GO tooltip did not update after LIVE takeover");
    });

    test("RQ001 T04: similar 64-character LIVE names have distinct visible locators",[] {
        MainComponent ui(false);
        const juce::String prefix="Emergency Lighting Subsystem - Cue Group ";
        const auto left=prefix+juce::String::repeatedString("A",64-prefix.length());
        const auto right=prefix+juce::String::repeatedString("B",64-prefix.length());
        check(left.length()==64 && right.length()==64 && left!=right,
              "T04 setup: two maximal similar names are required");
        check(ui.setCuelistName(0,left),
              "T04 setup: cannot name first long LIVE");
        ui.engine.connected.store(true);
        ui.triggerRow(0);
        ui.engine.processBlock(nullptr,0,1,100);
        ui.createCuelist();
        check(ui.setCuelistName(1,right),
              "T04 setup: cannot name second long LIVE");
        for(int i=2;i<9;++i) ui.createCuelist();
        ui.setSize(1180,820);
        ui.resized();
        auto* overflow=dynamic_cast<juce::TextButton*>(
            controlsV3::named(ui,"cuelistOverflow"));
        check(overflow && overflow->isVisible() && ui.show.activeCuelist==8,
              "T04 setup: LIVE must be hidden in compact overflow");
        check(ui.liveCuelistNameForUi()==left &&
              overflow->getTooltip().contains(left),
              "T04: long LIVE name A lost its full operator identity");
        const auto bounds=juce::Rectangle<int>(129,ui.getHeight()-333,31,16);
        const auto imageA=ui.createComponentSnapshot(bounds);
        check(imageA.isValid(),"T04: first native locator cannot be rendered");

        // Explicit Row GO in the second Cuelist transfers LIVE. Return to
        // the same VIEW to isolate only the live identity in the snapshot.
        ui.activateCuelist(1);
        ui.triggerRow(0);
        ui.engine.collectRetired();
        ui.engine.processBlock(nullptr,0,1,100);
        ui.activateCuelist(8);
        check(ui.liveCuelistIndex()==1 && ui.liveCuelistNameForUi()==right &&
              overflow->getTooltip().contains(right),
              "T04: switching to similarly named LIVE lost exact identity");
        const auto imageB=ui.createComponentSnapshot(bounds);
        check(imageB.isValid() && imageB.getSize()==imageA.getSize(),
              "T04: native locator snapshots have inconsistent dimensions");
        bool distinct=false;
        for(int y=0;y<imageA.getHeight() && !distinct;++y)
            for(int x=0;x<imageA.getWidth();++x)
                if(imageA.getPixelAt(x,y)!=imageB.getPixelAt(x,y)) {
                    distinct=true; break;
                }
        check(distinct,
              "T04: visually identical LIVE locators for two long similar names");
    });

    test("Deleting Cuelists protects data and reuses the first free number",[] {
        MainComponent ui(false);
        ui.createCuelist();
        ui.createCuelist();
        check(ui.show.cuelists.size()==3 && ui.show.activeCuelist==2,
              "Test could not create three Cuelists");
        const auto originalId=ui.show.cuelists[0].rows[0].id;
        const auto deletedId=ui.show.cuelists[1].rows[0].id;
        const auto deletedCellId=ui.show.cuelists[1].rows[0].cells[0].id;
        ui.deleteCuelist(1);
        check(ui.show.cuelists.size()==2 &&
              ui.show.cuelists[0].name=="Cuelist 1" &&
              ui.show.cuelists[1].name=="Cuelist 3" &&
              ui.show.activeCuelist==1 &&
              ui.show.rows[0].id==ui.show.cuelists[1].rows[0].id,
              "Delete did not update active index and preserve other Cuelists");
        check(ui.show.cuelists[0].rows[0].id==originalId &&
              ui.show.cuelists[0].rows[0].id!=deletedId &&
              ui.show.cuelists[0].rows[0].cells[0].id!=deletedCellId,
              "Deleting a Cuelist damaged surviving cue identities");
        ui.createCuelist();
        check(ui.show.cuelists.back().name=="Cuelist 2",
              "A deleted numbered slot was not reused");
        ui.deleteCuelist(ui.show.activeCuelist);
        check(ui.show.cuelists.size()==2 &&
              ui.show.activeCuelist>=0 &&
              ui.show.activeCuelist<(int)ui.show.cuelists.size() &&
              validateShow(ui.show).wasOk(),
              "Deleting the active Cuelist left the show invalid");
        ui.deleteCuelist(1);
        const auto copy=encodeShow(ui.show);
        ui.deleteCuelist(0);
        check(ui.show.cuelists.size()==1 && encodeShow(ui.show)==copy,
              "The final remaining Cuelist must not be deletable");
        Show restored;
        check(decodeShow(copy,restored).wasOk(),
              "Cuelist deletion left an invalid persisted project");
    });
    test("Historical projects without notes or Cuelists remain readable", [] {
        auto root=juce::JSON::parse(encodeShow(makeShow()));
        root.getDynamicObject()->removeProperty("cuelists");
        root.getDynamicObject()->removeProperty("activeCuelist");
        root["rows"].getArray()->getReference(0).getDynamicObject()->removeProperty("notes");
        Show restored;
        check(decodeShow(juce::JSON::toString(root),restored).wasOk(),
              "Original v10 project could no longer load");
        check(restored.cuelists.size()==1 && restored.activeCuelist==0 &&
              restored.rows[0].notes.isEmpty(),
              "Missing historical notes were not initialized empty");
    });

    test("Collapse-all and toolbar are layout-only across supported window sizes", [] {
        MainComponent ui(false);
        addColumn(ui.show,ColumnType::midi,"MIDI");
        addColumn(ui.show,ColumnType::osc,"OSC");
        ui.setAllColumnsCollapsed(true);
        for (const auto& c:ui.show.columns)
            check(c.collapsed,"Collapse-all missed a column");
        ui.setAllColumnsCollapsed(false);
        for (const auto& c:ui.show.columns)
            check(!c.collapsed,"Expand-all missed a column");
        auto* toggle=controlsV3::named(ui,"toggleAllColumns");
        auto* headers=cueViewFixtures::find<PinnedCueHeaders>(ui);
        auto* notes=controlsV3::named(ui,"cueNotes");
        auto* mapping=controlsV3::named(ui,"controlMapping");
        auto* plus=controlsV3::named(ui,"newCuelist");
        check(toggle && headers && notes && mapping && plus,
              "Mockup controls are missing");
        for (const auto window:std::initializer_list<juce::Point<int>>{
                 {1180,820},{1440,900},{1647,895},{1920,1080}}) {
            ui.setSize(window.x,window.y);
            ui.resized();
            // Keep the button inside the actual pinned header. Its width
            // changed when the cue-name lane grew to fit Play/Stop safely.
            check(toggle->isVisible() &&
                  toggle->getX()>=headers->getX() &&
                  toggle->getRight()<=headers->getRight() &&
                  toggle->getY()>=headers->getY() &&
                  toggle->getBottom()<=headers->getY()+CueGrid::headerHeight,
                  "Collapse-all escaped the Cue List header");
            auto* devices=controlsV3::named(ui,"operatorMenuDevices");
            auto* more=controlsV3::named(ui,"operatorMenuMore");
            check(!mapping->isVisible() && devices && more &&
                  (devices->isVisible() || more->isVisible()),
                  "Control Mapping must be in Devices/More, not a duplicate sidebar button");
            check(notes->getHeight()>=38 && notes->getRight()<=window.x &&
                  notes->getBottom()<window.y-30,
                  "Cue notes escaped the Inspector on a compact window");
            check(plus->getRight()<window.x-650,
                  "Cuelist tabs overlapped the clock");
        }
        auto* plusButton=dynamic_cast<juce::TextButton*>(plus);
        check(plusButton!=nullptr,"Cuelist plus must be a real button");
        click(*plusButton,12.f,12.f);
        check(ui.show.cuelists.size()==2 && ui.show.activeCuelist==1,
              "Top-bar plus did not create and select a Cuelist");
    });

    test("Multiple Cuelists preserve inactive audio on actual disk save and reload",[] {
        auto directory=juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getChildFile("cuelist-multi-test-"+juce::Uuid().toString());
        check(directory.createDirectory().wasOk(),"Cannot create temporary test folder");
        const auto first=directory.getChildFile("pres.mp3");
        const auto second=directory.getChildFile("mus.mp3");
        check(first.replaceWithText("audio fixture 1") &&
              second.replaceWithText("audio fixture 2"),
              "Cannot create temporary media files");
        auto show=makeShow();
        show.cuelists[0].name="Preshow";
        show.rows[0].cells[0].file=first.getFullPathName();
        show.rows[0].notes="First notes";
        Cuelist secondList;
        secondList.name="Musical";
        Row other;other.id=show.nextId++;other.notes="Second notes";
        Cell cell;cell.id=show.nextId++;cell.file=second.getFullPathName();
        other.cells.push_back(cell);secondList.rows.push_back(other);
        show.cuelists.push_back(secondList);
        const auto target=directory.getChildFile("performance.cuelist");
        check(saveShow(show,target).wasOk(),"Multi-Cuelist save failed");
        Show loaded;
        check(loadShow(target,loaded).wasOk(),"Multi-Cuelist load failed");
        check(loaded.cuelists.size()==2 &&
              loaded.cuelists[0].name=="Preshow" &&
              loaded.cuelists[1].name=="Musical" &&
              loaded.rows[0].notes=="First notes" &&
              loaded.cuelists[1].rows[0].notes=="Second notes",
              "Cuelist names or notes were lost on disk");
        check(juce::File(loaded.rows[0].cells[0].file).existsAsFile() &&
              juce::File(loaded.cuelists[1].rows[0].cells[0].file).existsAsFile(),
              "Inactive Cuelist media was not copied into the project folder");
        check(directory.deleteRecursively(),"Cannot clean temporary Cuelist test");
    });

    test("Inactive Cuelist audio is decoded with the same PCM budget", [] {
        auto show=makeShow();
        const auto file1=juce::File::getCurrentWorkingDirectory()
                             .getChildFile("mockup-a.wav").getFullPathName();
        const auto file2=juce::File::getCurrentWorkingDirectory()
                             .getChildFile("mockup-b.wav").getFullPathName();
        show.rows[0].cells[0].file=file1;
        Cuelist other;
        other.name="Musical";
        Row row;
        row.id=show.nextId++;
        Cell cell;cell.id=show.nextId++;cell.file=file2;
        row.cells.push_back(cell);other.rows.push_back(row);
        show.cuelists.push_back(other);
        PreparedShowMedia prepared;
        int loads=0;
        const AudioClipLoader loader=[&](const juce::File&,
                   std::shared_ptr<const AudioClip>& output)->juce::Result {
            ++loads;
            auto audio=std::make_shared<AudioClip>();
            audio->samples.setSize(1,128);
            audio->samples.clear();
            output=std::move(audio);
            return juce::Result::ok();
        };
        check(prepareShowMedia(show,prepared,loader).wasOk(),
              "Media preparation for multiple Cuelists failed");
        check(loads==2 && prepared.clips.size()==2,
              "Media belonging to the inactive Cuelist was not loaded");
        auto corrupt=show;
        corrupt.cuelists[1].rows[0].id=show.rows[0].id;
        check(validateShow(corrupt).failed(),
              "Overlapping cue identities across Cuelists went undetected");
    });
}
