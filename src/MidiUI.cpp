#include "MainComponent.h"
#include "PianoRoll.h"

namespace cuelist {
namespace {
bool sameMidiClip(const MidiClip& a,const MidiClip& b) {
    if (a.lengthBeats!=b.lengthBeats || a.notes.size()!=b.notes.size()) return false;
    for (size_t i=0;i<a.notes.size();++i) {
        const auto& x=a.notes[i]; const auto& y=b.notes[i];
        if (x.beat!=y.beat || x.length!=y.length || x.pitch!=y.pitch
            || x.velocity!=y.velocity || x.channel!=y.channel) return false;
    }
    return true;
}

class FloatingPianoRoll final : public juce::DocumentWindow {
public:
    explicit FloatingPianoRoll(std::function<void()> onClose)
        : juce::DocumentWindow("The Cuelist - Piano Roll",palette::panel,
                               juce::DocumentWindow::closeButton,true),
          close(std::move(onClose)) {}
    void closeButtonPressed() override { if (close) close(); }
private:
    std::function<void()> close;
};
}

// This history action records the logical gesture, not its intermediate drag frames.
// The model already contains the new state when perform() is called the first time.
class MidiUndoAction final : public juce::UndoableAction {
public:
    MidiUndoAction(MainComponent& p,int cell,MidiClip oldValue,MidiClip newValue)
        : owner(&p),id(cell),before(std::move(oldValue)),after(std::move(newValue)) {}
    bool perform() override {
        if (first) { first=false; return true; }
        if (!owner) return false;
        owner->restoreMidiClip(id,after); return true;
    }
    bool undo() override {
        if (!owner) return false;
        owner->restoreMidiClip(id,before); return true;
    }
    int getSizeInUnits() override { return 1+(int)(before.notes.size()+after.notes.size())/64; }
private:
    juce::Component::SafePointer<MainComponent> owner;
    int id;
    MidiClip before,after;
    bool first=true;
};

bool MainComponent::hasClip(int row,int column) const {
    if (row<0 || column<0 || row>=(int)show.rows.size() || column>=(int)show.columns.size()) return false;
    const auto& cell=show.rows[(size_t)row].cells[(size_t)column];
    const auto type=show.columns[(size_t)column].type;
    if (type==ColumnType::control) {
        const auto& col=show.columns[(size_t)column];
        return col.controlLocked ? !col.controlDefaultSet.bindings.empty() :
            cell.controlSet.has_value() && !cell.controlSet->bindings.empty();
    }
    return type==ColumnType::midi ? cell.midi.has_value()
        : type==ColumnType::osc ? cell.osc.has_value() :
            cell.click.has_value() || clips.count(cell.id)>0;
}
double MainComponent::clipDuration(int row,int column) const {
    if (!hasClip(row,column)) return 0;
    const auto& cue=show.rows[(size_t)row]; const auto& cell=cue.cells[(size_t)column];
    if (cell.midi) return secondsAtBeat(cue.tempo,cell.midi->lengthBeats);
    if (cell.osc) return cell.osc->useBeats ? secondsAtBeat(cue.tempo,cell.osc->length) : cell.osc->length;
    if (cell.click) return cell.click->durationSeconds;
    const auto clip=clips.find(cell.id);
    return clip!=clips.end() && clip->second->sampleRate>0 ? clip->second->samples.getNumSamples()/clip->second->sampleRate : 0;
}
void MainComponent::editMidiClip() {
    if (show.columns[selectedColumn].type!=ColumnType::midi || !applyInspector()) return;
    closeOscClip();
    auto& cell=show.rows[selectedRow].cells[selectedColumn];
    if (!cell.midi) {
        cell.midi=MidiClip{};
        dirty=true; engine.publish(makeLiveRenderState());
    }
    const int id=cell.id;
    if (clipEditor) {
        if (clipEditorCellId!=id) {
            clipEditorCellId=id; clipEditor->loadClip(*cell.midi);
        }
        clipCollapsed=false;
        clipCollapseButton.setButtonText("Collapse");
        if (clipWindow) clipWindow->toFront(true);
        else { resized(); repaint(); }
        return;
    }
    clipEditorCellId=id;
    auto safe=juce::Component::SafePointer<MainComponent>(this);
    clipEditor=std::make_unique<PianoRollEditor>(*cell.midi,
        [safe](const MidiClip& before,const MidiClip& after,bool complete) {
            if (safe && safe->clipEditorCellId>0)
                safe->midiClipEdited(safe->clipEditorCellId,before,after,complete);
        });
    clipEditor->onDetach=[safe] {
        if (!safe) return;
        if (safe->isClipEditorFloating()) safe->dockMidiClip();
        else safe->detachMidiClip();
    };
    clipEditor->onUndo=[safe] { if (safe) safe->undoEdit(); };
    clipEditor->onRedo=[safe] { if (safe) safe->redoEdit(); };
    clipEditor->setFloating(false);
    addAndMakeVisible(*clipEditor);
    clipCollapsed=false; clipCollapseButton.setButtonText("Collapse");
    resized(); repaint();
}
void MainComponent::syncClipEditor() {
    if (!clipEditor) return;
    if (show.columns[selectedColumn].type!=ColumnType::midi) { closeMidiClip(); return; }
    const auto& cell=show.rows[selectedRow].cells[selectedColumn];
    if (clipEditorCellId==cell.id) return;
    clipEditorCellId=cell.id;
    clipEditor->loadClip(cell.midi.value_or(MidiClip{}));
}
void MainComponent::detachMidiClip() {
    if (!clipEditor || clipWindow) return;
    if (clipEditor->getParentComponent()==this) removeChildComponent(clipEditor.get());
    auto safe=juce::Component::SafePointer<MainComponent>(this);
    auto* window=new FloatingPianoRoll([safe] {
        juce::MessageManager::callAsync([safe] { if (safe) safe->dockMidiClip(); });
    });
    clipWindow.reset(window);
    window->setUsingNativeTitleBar(true);
    window->setResizable(true,true);
    window->setResizeLimits(760,320,1800,1200);
    window->setContentNonOwned(clipEditor.get(),true);
    window->centreWithSize(1000,620);
    clipEditor->setFloating(true);
    window->setVisible(true); window->toFront(true);
    resized(); repaint();
}
void MainComponent::dockMidiClip() {
    if (!clipEditor || !clipWindow) return;
    clipWindow->clearContentComponent();
    clipWindow->setVisible(false);
    clipWindow.reset();
    addAndMakeVisible(*clipEditor);
    clipEditor->setFloating(false);
    clipCollapsed=false; clipCollapseButton.setButtonText("Collapse");
    resized(); repaint();
}
void MainComponent::closeMidiClip() {
    if (clipWindow) {
        clipWindow->clearContentComponent();
        clipWindow->setVisible(false);
        clipWindow.reset();
    }
    if (clipEditor) {
        if (clipEditor->getParentComponent()==this) removeChildComponent(clipEditor.get());
        clipEditor.reset();
    }
    clipEditorCellId=-1;
    clipCollapsed=false;
    resized(); repaint();
}
void MainComponent::midiClipEdited(int cellId,const MidiClip& before,const MidiClip& after,bool complete) {
    if (sameMidiClip(before,after) || validateMidiClip(after).failed()) return;
    for (auto& row:show.rows)
        for (size_t c=0;c<row.cells.size();++c) {
            auto& cell=row.cells[c];
            if (cell.id!=cellId || show.columns[c].type!=ColumnType::midi) continue;
            if (!cell.midi || !sameMidiClip(*cell.midi,after)) {
                cell.midi=after; cell.file.clear(); dirty=true;
                engine.publish(makeLiveRenderState());
                status="MIDI clip updated. Save the show to keep your changes.";
                timeline.repaint(); grid.repaint(); repaint();
            }
            if (complete) {
                editHistory.beginNewTransaction("Edit MIDI clip");
                editHistory.perform(new MidiUndoAction(*this,cellId,before,after));
                markHistoryBaseline(); // Already stored by gesture history, not duplicated.
            }
            return;
        }
}
void MainComponent::restoreMidiClip(int cellId,const MidiClip& snapshot) {
    // The original cell may now be in an inactive VIEW: do not restore a
    // different cell just because the operator changed tabs.
    const auto target=findStoredClipCell(cellId,ColumnType::midi);
    if (!target.cell) return;
    if (target.live) {engine.panic();resetActiveMapping();}
    target.cell->midi=snapshot;
    dirty=true;
    engine.publish(makeLiveRenderState(),!target.live);
    if (clipEditor && clipEditorCellId==cellId)
        clipEditor->loadClip(snapshot);
    status="MIDI edit reverted.";
    grid.repaint();timeline.repaint();repaint();
}
bool MainComponent::undoEdit() {
    checkpointHistory("Edit Show");
    if(!editHistory.canUndo())return false;
    restoringHistory=true;
    const bool ok=editHistory.undo();
    restoringHistory=false;
    if(ok)markHistoryBaseline();
    return ok;
}
bool MainComponent::redoEdit() {
    checkpointHistory("Edit Show");
    if(!editHistory.canRedo())return false;
    restoringHistory=true;
    const bool ok=editHistory.redo();
    restoringHistory=false;
    if(ok)markHistoryBaseline();
    return ok;
}
void MainComponent::setMidiOutput(int column,const juce::String& identifier) {
    if (column<0 || column>=(int)show.columns.size() ||
        show.columns[column].type!=ColumnType::midi)return;
    if(show.columns[column].midiOutputId==identifier)return;
    // This is a routing edit, not a transport command. Never emit PANIC,
    // reset the audio callback or force-reopen an already working device.
    const auto ready=identifier.isEmpty() ? juce::Result::ok() :
        engine.setMidiOutput(identifier);
    // Retain the explicitly selected ID even if it is currently unplugged:
    // audio and the cue clock keep running, MIDI messages are skipped, and
    // the exact device can reconnect later (never silently substituted).
    show.columns[(size_t)column].midiOutputId=identifier;
    changed(identifier.isEmpty() ? "MIDI output disabled; audio continues." :
        ready.wasOk() ? "MIDI output changed without stopping playback." :
        "MIDI output unavailable; audio continues and MIDI is skipped.");
    utilities.refresh();
}
}
