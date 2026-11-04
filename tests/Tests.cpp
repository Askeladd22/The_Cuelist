#include "ShowModel.h"
#include "MainComponent.h"
#include <cmath>
#include <iostream>
#include <functional>
#include <stdexcept>
using namespace cuelist;
int failures = 0, checks = 0;
void check(bool condition, const char* message) { ++checks; if (!condition) throw std::runtime_error(message); }
void near(double actual, double expected) { check(std::abs(actual - expected) < 0.00001, "Unexpected numeric result"); }
void test(const char* name, const std::function<void()>& body) {
    // Print the test currently executing immediately, even if the process
    // is terminated by native callback/GUI code before normal teardown.
    std::cerr << "RUN " << name << std::endl;
    try { body(); std::cout << "PASS " << name << std::endl; }
    catch (const std::exception& e) {
        ++failures; std::cerr << "FAIL " << name << ": " << e.what() << std::endl;
    }
}
#include "AudioTests.h"
#include "ModelV2Tests.h"
#include "CueViewTests.h"
#include "ModelV3Tests.h"
#include "TimelineV3Tests.h"
#include "ControlsV3Tests.h"
#include "CuelistGuiMockupTests.h"
#include "EngineV4Tests.h"
#include "ControlsV4Tests.h"
#include "PianoRollTests.h"
#include "MidiTests.h"
#include "OscTests.h"
#include "OscAutomationV9Tests.h"
#include "AutoStopV7Tests.h"
#include "MixerV8Tests.h"
#include "MasterBusRoutingTests.h"
#include "UtilityPolishTests.h"
#include "CueTransportUXTests.h"
#include "GainMeterFaderTests.h"
#include "UiRefinementTests.h"
#include "OperatorNavigationTests.h"
#include "RowActionsUiTests.h"
#include "ShowEditOperationsTests.h"
#include "WorkspaceLayoutTests.h"
#include "CueGridGeometryTests.h"
#include "CueGridN3VisualTests.h"
#include "ControlMappingV10Tests.h"
#include "ControlMapArchiveTests.h"
#include "ControlRuntimeTests.h"
#include "QuickLearnPersistenceTests.h"
#include "ControlDispatchPolicyTests.h"
#include "MidiSessionPolicyTests.h"
#include "ControlColumnTests.h"
#include "LtcGeneratorTests.h"
#include "LtcAcceptanceTests.h"
#include "StabilityV7Tests.h"
#include "ArchitectureV8Tests.h"
int main() {
    juce::ScopedJuceInitialiser_GUI initialiseJuce;
    // Exact graphite/copper tokens plus readable foregrounds on the native
    // dark surfaces. These are logical checks, not a Windows/DPI visual test.
    test("Industrial Orange design tokens match the approved specification", [] {
        const std::array<juce::Colour,20> actual{
            palette::swatch::bgMain,palette::swatch::bgPanel,
            palette::swatch::bgElevated,palette::swatch::bgHover,
            palette::swatch::bgSelected,palette::swatch::borderDefault,
            palette::swatch::accentPrimary,palette::swatch::accentHover,
            palette::swatch::accentPressed,palette::swatch::typeAudio,
            palette::swatch::typeMidi,palette::swatch::typeOsc,
            palette::swatch::typeControl,palette::swatch::statusGo,
            palette::swatch::statusPanic,palette::swatch::statusWarning,
            palette::swatch::statusInfo,palette::swatch::textPrimary,
            palette::swatch::textSecondary,palette::swatch::textMuted
        };
        const std::array<unsigned int,20> expected{
            0xff101619,0xff182227,0xff202c32,0xff29373d,
            0xff303b40,0xff35434a,0xffc66b42,0xffd47a4f,
            0xffa95534,0xff8fa8b2,0xffb79a72,0xffa18bb9,
            0xff7186a6,0xff8eae91,0xffc66e69,0xffd1a45b,
            0xff8fa8b2,0xffe5e7e3,0xffa5b0ae,0xff718087
        };
        for(size_t i=0;i<actual.size();++i)
            check(actual[i].getARGB()==expected[i],"Industrial Orange token mismatch");
        check(palette::type(0)==palette::swatch::typeAudio &&
              palette::type(1)==palette::swatch::typeMidi &&
              palette::type(2)==palette::swatch::typeOsc &&
              palette::type(3)==palette::swatch::typeControl,
              "Column colours must remain semantic and not use the global orange");
        const std::array<juce::Colour,7> savedCueColours{
            palette::swatch::cue1,palette::swatch::cue2,palette::swatch::cue3,
            palette::swatch::cue4,palette::swatch::cue5,palette::swatch::cue6,
            palette::swatch::cue7
        };
        const std::array<unsigned int,7> expectedCueColours{
            0xffe4a77e,0xffdebd88,0xffe48c7f,0xffb7a1bf,
            0xff8ea7b8,0xffa1b5a4,0xffd9b577
        };
        for(size_t i=0;i<savedCueColours.size();++i)
            check(savedCueColours[i].getARGB()==expectedCueColours[i],
                  "User-assignable cue colour changed in the theme update");
    });
    test("Industrial Orange readable primary actions and labels", [] {
        const auto linear=[](float channel) {
            return channel<=0.04045f ? channel/12.92f :
                   std::pow((channel+0.055f)/1.055f,2.4f);
        };
        const auto luminance=[&](juce::Colour c) {
            return 0.2126f*linear(c.getFloatRed())+
                   0.7152f*linear(c.getFloatGreen())+
                   0.0722f*linear(c.getFloatBlue());
        };
        const auto ratio=[&](juce::Colour a,juce::Colour b) {
            const auto x=luminance(a),y=luminance(b);
            return (std::max(x,y)+.05f)/(std::min(x,y)+.05f);
        };
        for(const auto surface:{palette::background,palette::input,
                                 palette::panel,palette::toolbar,
                                 palette::raised}) {
            check(ratio(palette::text,surface)>=4.5f,"Primary text contrast");
            check(ratio(palette::muted,surface)>=4.5f,"Secondary text contrast");
        }
        check(ratio(palette::teal,palette::raised)>=4.5f,
              "Accessible accent ink on selected graphite");
        check(ratio(palette::red,palette::raised)>=4.5f,
              "Accessible warning ink on selected graphite");
        check(ratio(palette::background,palette::go)>=4.5f,
              "Dark GO label on green");
        check(ratio(palette::background,palette::panic)>=4.5f,
              "Dark PANIC label on coral");
    });
    test("Global Undo/Redo restores cue colour and Auto Stop in order",[] {
        MainComponent ui(false);
        check(!ui.undoEdit(),"An empty Show has history.");
        ui.setRowColour(0,2);
        ui.setCellAutoStop(0,0,false);
        check(ui.show.rows[0].colour==2 &&
              !ui.show.rows[0].cells[0].stopOnNext,"Editing did not commit");
        check(ui.undoEdit() && ui.show.rows[0].cells[0].stopOnNext &&
              ui.show.rows[0].colour==2,"Cell Auto Stop Undo lost other edits");
        check(ui.undoEdit() && ui.show.rows[0].colour==0,
              "Cue colour Undo failed");
        check(ui.redoEdit() && ui.show.rows[0].colour==2,
              "Cue colour Redo failed");
        check(ui.redoEdit() && !ui.show.rows[0].cells[0].stopOnNext,
              "Cell Auto Stop Redo failed");
    });
    test("Global Undo/Redo restores structural edits and cell identities",[] {
        MainComponent ui(false);
        const int id=ui.show.rows[0].id;
        check(ui.duplicateRow(0),"Duplicating cue failed");
        check(ui.show.rows.size()==2,"Missing duplicate cue");
        check(ui.undoEdit() && ui.show.rows.size()==1 &&
              ui.show.rows[0].id==id,"Undo did not restore original cue structure");
        check(ui.redoEdit() && ui.show.rows.size()==2 &&
              validateShow(ui.show).wasOk(),"Redo did not restore valid duplicated cue");
    });
    test("Create and reorder Cuelists use the same global Undo history",[] {
        MainComponent ui(false);
        ui.createCuelist();
        check(ui.show.cuelists.size()==2,
              "Creating a Cuelist did not commit a structural edit");
        check(ui.undoEdit() && ui.show.cuelists.size()==1 &&
              validateShow(ui.show).wasOk(),
              "Creating a Cuelist could not be undone");
        check(ui.redoEdit() && ui.show.cuelists.size()==2,
              "Creating a Cuelist could not be redone");
        const auto name=ui.show.cuelists[0].name;
        check(ui.moveCuelist(0,1),"Could not reorder Cuelists");
        check(ui.undoEdit() && ui.show.cuelists[0].name==name,
              "Cuelist reorder Undo lost stable tab identity");
    });
    test("One continuous Master fader gesture is one Undo transaction",[] {
        MainComponent ui(false);
        const float initial=ui.show.masterGain;
        ui.setMasterAudio(.9f);
        ui.setMasterAudio(.85f);
        ui.setMasterAudio(.8f);
        check(std::abs(ui.show.masterGain-.8f)<.0001f,
              "Master fader moves did not apply");
        check(ui.undoEdit() &&
              std::abs(ui.show.masterGain-initial)<.0001f,
              "Continuous slider edits were not coalesced into one Undo");
        check(ui.redoEdit() &&
              std::abs(ui.show.masterGain-.8f)<.0001f,
              "Master fader coalesced Redo did not restore final value");
    });
    test("Undo on VIEW does not stop audio belonging to LIVE Cuelist",[] {
        MainComponent ui(false);
        auto clip=std::make_shared<AudioClip>();
        clip->sampleRate=100;clip->samples.setSize(1,500);
        for(int i=0;i<500;++i)clip->samples.setSample(0,i,.2f);
        const int original=ui.show.rows[0].cells[0].id;
        ui.clips[original]=clip;
        ui.engine.connected.store(true);
        ui.engine.publish(ui.makeLiveRenderState());
        ui.triggerRow(0);
        ui.engine.processBlock(nullptr,0,1,100);
        check(ui.engine.getCellPlayback(original).playing,"LIVE audio did not start");
        ui.createCuelist();
        check(ui.liveCuelistIndex()!=ui.show.activeCuelist,
              "Test did not isolate VIEW from LIVE");
        ui.setRowColour(0,2);
        check(ui.undoEdit() && ui.show.rows[0].colour==0,
              "VIEW-only edit was not undone");
        ui.engine.processBlock(nullptr,0,1,100);
        check(ui.engine.getCellPlayback(original).playing &&
              ui.liveCuelistIndex()!=ui.show.activeCuelist,
              "Undo on VIEW unexpectedly silenced LIVE playback");
    });
    pianoRollTests();
    registerMidiTests();
    oscTests();
    oscAutomationV9Tests();
    autoStopV7Tests();
    mixerV8Tests();
    masterBusRoutingTests();
    utilityPolishTests();
    cueTransportUXTests();
    gainMeterFaderTests();
    uiRefinementTests();
    operatorNavigationTests();
    rowActionsUiTests();
    showEditOperationsTests();
    workspaceLayoutTests();
    cueGridGeometryTests();
    cueGridN3VisualTests();
    controlMappingV10Tests();
    controlMapArchiveTests();
    controlRuntimeTests();
    quickLearnPersistenceTests();
    controlDispatchPolicyTests();
    midiSessionPolicyTests();
    controlColumnTests();
    ltcGeneratorTests();
    ltcAcceptanceTests();
    stabilityV7Tests();
    architectureV8Tests();
    cuelistGuiMockupTests();
    test("MIDI columns accept file drops and enable their importer", [] {
        MainComponent ui(false); addColumn(ui.show,ColumnType::midi,{}); ui.select(0,1);
        auto* grid=cueViewFixtures::find<CueGrid>(ui);
        check(grid->isInterestedInFileDrag({"notes.mid"}),"MIDI file drop rejected");
        bool importer=false;
        for (auto* child:ui.getChildren()) if (auto* button=dynamic_cast<juce::TextButton*>(child))
            if (button->getButtonText()=="Import MIDI") importer=button->isEnabled();
        check(importer,"MIDI importer unavailable");
    });
    test("tempo changes integrate seconds, independent of other rows", [] {
        near(secondsAtBeat({{0,120,4,4},{4,60,3,4}}, 6), 4);
        near(secondsAtBeat({{0,60,4,4}}, 6), 6);
    });
    test("new rows and columns stay rectangular with unique cell identities", [] {
        auto show = makeShow(); addRow(show, "Extra"); addColumn(show, ColumnType::audio, "FX");
        check(show.rows.size() >= 2, "Rows absent");
        for (const auto& row : show.rows) check(row.cells.size() == show.columns.size(), "Ragged row");
        check(validateShow(show).wasOk(), "Invalid generated show");
    });
    test("project roundtrip preserves path, behaviour, participation, tempo and routing", [] {
        auto show = makeShow();
        check(!show.rows.empty(), "No initial rows");
        auto& cell = show.rows[0].cells[0]; cell.file = "C:/Audio/test.wav"; cell.loop = true; cell.followsRow = false;
        cell.behaviour = Behaviour::pauseResume; show.rows[0].tempo.push_back({8,95,7,8}); show.columns[0].output = 2;
        Show restored; check(decodeShow(encodeShow(show), restored).wasOk(), "Roundtrip rejected");
        check(restored.rows[0].cells[0].file == cell.file, "Path lost");
        check(!restored.rows[0].cells[0].followsRow && restored.rows[0].cells[0].loop, "Flags lost");
        check(restored.rows[0].cells[0].behaviour == Behaviour::pauseResume, "Behaviour lost");
        near(restored.rows[0].tempo[1].bpm, 95); check(restored.columns[0].output == 2, "Routing lost");
    });
    test("malformed and future projects are rejected without replacing current show", [] {
        Show show; show.name = "Keep me";
        for (auto json : {"{", "{}", "{\"version\":99}", "{\"version\":1,\"rows\":null}"}) {
            check(decodeShow(json, show).failed(), "Bad project accepted");
            check(show.name == "Keep me", "Existing show replaced");
        }
    });
    test("invalid tempo and duplicate identity are rejected", [] {
        auto show = makeShow(); check(!show.rows.empty(), "No initial rows");
        show.rows[0].tempo[0].bpm = 0; check(validateShow(show).failed(), "Zero bpm accepted");
        show.rows[0].tempo[0].bpm = 120;
        show.rows[0].cells[0].id = show.rows[0].id; check(validateShow(show).failed(), "Duplicate identity accepted");
    });
    audioTests();
    modelV2Tests();
    cueViewTests();
    modelV3Tests();
    timelineV3Tests();
    controlsV3Tests();
    engineV4Tests();
    controlsV4Tests();
    test("Inspector exposes clip gain, while legacy playback modes remain stored but hidden",[] {
        MainComponent ui(false);ui.engine.connected.store(true);
        auto clip=std::make_shared<AudioClip>();clip->sampleRate=4;clip->samples.setSize(1,20);
        for(int i=0;i<20;++i) clip->samples.setSample(0,i,.01f*(i+1));
        ui.clips[ui.show.rows[0].cells[0].id]=clip;
        ui.engine.publish(makeRenderState(ui.show,ui.clips));
        auto* gain=dynamic_cast<juce::TextEditor*>(controlsV3::named(ui,"cellGainDb"));
        check(gain && gain->isVisible(),"Clip gain disappeared from Inspector");
        auto sample=[&] {
            float result=0;float* out[]{&result};
            ui.engine.collectRetired();ui.engine.processBlock(out,1,1,4);return result;
        };
        ui.triggerCell();near(sample(),.01);
        gain->setText("-6.0",false); gain->onReturnKey();
        near(sample(),.02*std::pow(10.0,-6.0/20.0));
        auto* hiddenMode=dynamic_cast<juce::ComboBox*>(controlsV3::named(ui,"cellBehaviour"));
        if(hiddenMode) check(!hiddenMode->isVisible(),"Legacy playback mode is still exposed");
        check(ui.show.rows[0].cells[0].behaviour==Behaviour::retrigger,
              "Retrigger default changed during Inspector simplification");
    });
    test("Enter commits names and GO advances without wrapping", [] {
        MainComponent ui(false); addRow(ui.show,{}); ui.engine.connected.store(true);
        for (auto* child : ui.getChildren()) if (child->getName()=="cueName") {
            auto* editor=dynamic_cast<juce::TextEditor*>(child); editor->setText("Scena",false); editor->onReturnKey();
        }
        check(ui.show.rows[0].name=="Scena","Enter did not commit");
        ui.goNext(); check(ui.nextCueIndex()==1,"First GO did not advance");
        ui.goNext(); check(ui.nextCueIndex()==2 && ui.selectedRow==1,"Second GO did not advance");
        ui.goNext(); check(ui.nextCueIndex()==2,"Sequence wrapped");
        ui.select(0,0); check(ui.nextCueIndex()==0,"Selection did not rearm");
    });
    test("Gain dB input applies to the model and structural controls preserve shape", [] {
        MainComponent ui(false);
        for (auto* child : ui.getChildren()) if (child->getName()=="cellGainDb") {
            auto* edit=dynamic_cast<juce::TextEditor*>(child);
            check(edit!=nullptr,"Gain input is not an editable text field");
            edit->setText("-6.0",false); edit->onReturnKey();
        }
        near(ui.show.rows[0].cells[0].gain,std::pow(10.0,-6.0/20.0));
        ui.setClipStart(0,2.37); near(ui.show.rows[0].cells[0].startSeconds,2.37);
        addRow(ui.show,{}); int id=ui.show.rows[0].id; ui.reorderRow(0,1); check(ui.show.rows[1].id==id,"Row move failed");
        addColumn(ui.show,ColumnType::audio,{}); id=ui.show.columns[0].id; ui.reorderColumn(0,1); check(ui.show.columns[1].id==id,"Column move failed");
        ui.deleteRow(1); ui.deleteRow(0); ui.deleteColumn(1); ui.deleteColumn(0);
        check(validateShow(ui.show).wasOk(),"Deleting final row/column broke model");
    });
    test("inspector drafts count as unsaved work", [] {
        MainComponent ui(false);
        for (auto* child : ui.getChildren()) if (child->getName()=="cueName") {
            auto* edit=dynamic_cast<juce::TextEditor*>(child); check(edit!=nullptr,"Editor absent");
            edit->setText("Nuovo nome",true); // TextEditor change callbacks are asynchronous; query must inspect actual draft.
        }
        check(ui.hasUnsavedChanges(),"Pending inspector edit not tracked");
    });
    test("changing selection commits pending cue name", [] {
        MainComponent ui(false);
        addRow(ui.show, {});
        for (auto* child : ui.getChildren()) if (child->getName()=="cueName") dynamic_cast<juce::TextEditor*>(child)->setText("Nuovo nome",false);
        ui.select(1,0); check(ui.show.rows[0].name=="Nuovo nome","Selection lost pending edit");
    });
    test("Escape in inspector bubbles to the Panic handler", [] {
        MainComponent ui(false); int editors=0;
        bool foundNotes=false;
        for (auto* child : ui.getChildren()) if (auto* edit=dynamic_cast<juce::TextEditor*>(child)) {
            // The shared inline rename editor deliberately consumes Escape
            // to cancel a rename, not to fire the global Panic action.
            if (edit->getName()=="inlineNameEditor") {
                check(!edit->isVisible(),"Inline editor must start hidden");
                continue;
            }
            ++editors;
            if (edit->getName()=="cueNotes") {
                foundNotes=true;
                check(edit->isMultiLine(),"Cue notes lost multiline input");
            }
            check(!edit->keyPressed(juce::KeyPress(juce::KeyPress::escapeKey)),
                  "Editor swallowed Escape instead of allowing global Panic");
        }
        check(editors==5 && foundNotes,
              "Expected cue name, cell name, BPM, Gain and multiline notes fields");
        check(ui.keyPressed(juce::KeyPress(juce::KeyPress::escapeKey)),
              "MainComponent no longer handles Escape as Panic");
    });
    test("WAV import and atomic project save work on disk with previous-version backup", [] {
        const auto folder=juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory().getChildFile("test-artifacts");
        check(folder.createDirectory().wasOk(),"Cannot create fixture directory");
        const auto wave=folder.getChildFile("verification.wav");
        juce::AudioBuffer<float> pcm(2,240000);
        for (int i=0;i<pcm.getNumSamples();++i) {
            const auto fade=std::min(1.0f,std::min(i/2400.0f,(240000-i)/2400.0f));
            pcm.setSample(0,i,0.06f*fade*std::sin((float)(i*2.0*juce::MathConstants<double>::pi*220.0/48000.0)));
            pcm.setSample(1,i,0.06f*fade*std::sin((float)(i*2.0*juce::MathConstants<double>::pi*330.0/48000.0)));
        }
        {
            auto stream=wave.createOutputStream(); check(stream!=nullptr,"No WAV stream");
            stream->setPosition(0); stream->truncate(); juce::WavAudioFormat format;
            std::unique_ptr<juce::OutputStream> outputStream(std::move(stream));
            auto writer=format.createWriterFor(outputStream,juce::AudioFormatWriterOptions().withSampleRate(48000).withNumChannels(2).withBitsPerSample(16));
            check(writer!=nullptr,"No WAV writer"); check(writer->writeFromAudioSampleBuffer(pcm,0,pcm.getNumSamples()),"WAV write failed");
        }
        std::shared_ptr<const AudioClip> loaded; check(loadClip(wave,loaded).wasOk(),"Valid WAV rejected");
        near(loaded->samples.getNumSamples()/loaded->sampleRate,5); check(loaded->samples.getNumChannels()==2,"Stereo lost");
        auto show=makeShow(); show.rows[0].cells[0].file=wave.getFullPathName(); show.rows[0].cells[0].name="Test stereo 5 secondi";
        const auto project=folder.getChildFile("verification.cuelist"); check(saveShow(show,project).wasOk(),"First save failed");
        show.name="Verifica audio"; check(saveShow(show,project).wasOk(),"Second save failed");
        Show restored; check(decodeShow(project.loadFileAsString(),restored).wasOk(),"Saved file unreadable");
        check(restored.name=="Verifica audio","Wrong saved content");
        check(decodeShow(project.getSiblingFile("verification.cuelist.bak").loadFileAsString(),restored).wasOk(),"Backup unreadable");
        check(restored.name=="Show 001","Backup did not preserve previous version");
        check(saveShow(show,folder.getChildFile("missing-parent").getChildFile("show.cuelist")).failed(),"Invalid destination accepted");
    });
    test("audio settings selector is destroyed before its audio manager", [] {
        juce::Component::SafePointer<juce::Component> dialog;
        {
            MainComponent ui(false);
            // The Settings -> Audio Devices menu uses this same action.
            // Direct invocation keeps the lifetime test independent of menu labels.
            ui.showDevices();
            dialog=juce::ModalComponentManager::getInstance()->getModalComponent(0);
            check(dialog!=nullptr,"Settings audio selector did not open");
        }
        check(dialog==nullptr,"Settings audio selector outlived audio manager");
    });
    if (failures==0) {
        // Native JUCE snapshots remain useful for manual visual inspection.
        // Explicit stages reveal teardown / rendering failures on Windows.
        std::cerr << "VISUAL QA: begin" << std::endl;
        const auto folder=juce::File::getSpecialLocation(juce::File::currentExecutableFile)
            .getParentDirectory().getChildFile("test-artifacts");
        MainComponent preview(false);
        preview.importFile(folder.getChildFile("verification.wav"));
        std::cerr << "VISUAL QA: imported" << std::endl;
        preview.setSize(1440,900);
        auto savePreview=[&](const juce::String& name) {
            auto stream=folder.getChildFile(name).createOutputStream();
            if(stream) {
                stream->setPosition(0);stream->truncate();
                juce::PNGImageFormat{}.writeImageToStream(
                    preview.createComponentSnapshot(preview.getLocalBounds()),*stream);
            }
        };
        savePreview("interface-preview.png");
        std::cerr << "VISUAL QA: initial snapshot" << std::endl;
        addColumn(preview.show,ColumnType::midi,"MIDI");
        addColumn(preview.show,ColumnType::osc,"OSC");
        cueViewFixtures::find<ColumnUtilities>(preview)->refresh();
        preview.setSize(1700,900);
        preview.engine.connected.store(true);
        preview.triggerCell();
        // Use normal audio callback sizes, not an artificial 96000 frames.
        juce::AudioBuffer<float> buffer(2,1024);
        for(int i=0;i<16;++i) {
            preview.engine.collectRetired();
            preview.engine.processBlock(buffer.getArrayOfWritePointers(),2,1024,48000);
        }
        std::cerr << "VISUAL QA: audio processed" << std::endl;
        savePreview("interface-mixed-progress.png");
        // Entire Q and Cell are simultaneously visible, no tab-switch.
        preview.setSize(1180,820);
        savePreview("interface-row-small.png");
        std::cerr << "VISUAL QA: all snapshots complete" << std::endl;
    }
    std::cout << checks << " assertions, " << failures << " failed tests\n";
    return failures == 0 ? 0 : 1;
}
