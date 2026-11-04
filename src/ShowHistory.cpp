#include "MainComponent.h"
#include <algorithm>
#include <utility>

namespace cuelist {

// All application actions share the same JUCE UndoManager. Pure model
// snapshots complement MIDI/OSC's existing gesture-specific actions.
class ShowUndoAction final : public juce::UndoableAction {
public:
    ShowUndoAction(MainComponent& ui,MainComponent::HistorySnapshot previous,
                   MainComponent::HistorySnapshot replacement,juce::String label)
        : owner(&ui),before(std::move(previous)),after(std::move(replacement)),
          description(std::move(label)) {
        const auto bytes=encodeShow(before.model).getNumBytesAsUTF8()+
                         encodeShow(after.model).getNumBytesAsUTF8();
        units=std::max(1,bytes/1024);
    }
    bool perform() override {
        if(first) {first=false;return true;} // UI already committed the edit.
        return owner && owner->restoreHistorySnapshot(after,"Redo: "+description);
    }
    bool undo() override {
        return owner && owner->restoreHistorySnapshot(before,"Undo: "+description);
    }
    int getSizeInUnits() override {return units;}
private:
    juce::Component::SafePointer<MainComponent> owner;
    MainComponent::HistorySnapshot before,after;
    juce::String description;
    int units=1;
    bool first=true;
};

void MainComponent::markHistoryBaseline() {
    historyBaseline.model=show;
    historyBaseline.media=clips; // shared_ptr values, never copied PCM samples
    historyBaselineJson=encodeShow(show);
    historyReady=true;
}

void MainComponent::checkpointHistory(const juce::String& description) {
    if(restoringHistory)return;
    if(!historyReady) {markHistoryBaseline();return;}
    const auto encoded=encodeShow(show);
    if(encoded==historyBaselineJson && clips==historyBaseline.media)return;
    HistorySnapshot after{show,clips};
    editHistory.beginNewTransaction(description);
    editHistory.perform(new ShowUndoAction(*this,historyBaseline,after,description));
    historyBaseline=std::move(after);
    historyBaselineJson=encoded;
}

bool MainComponent::restoreHistorySnapshot(const HistorySnapshot& snapshot,
                                           const juce::String& description) {
    if(auto valid=validateShow(snapshot.model);valid.failed()) {
        status="Undo snapshot rejected: "+valid.getErrorMessage();
        repaint();return false;
    }

    // A running Cuelist is identified by a stable cue ID, not the index of
    // the tab currently being inspected (which a snapshot may have reordered).
    const auto rowsAt=[](const Show& s,int index)->const std::vector<Row>& {
        return index==s.activeCuelist ? s.rows : s.cuelists[(size_t)index].rows;
    };
    int liveFirst=0,nextRowId=0;
    if(liveCuelist>=0 && liveCuelist<(int)show.cuelists.size()) {
        const auto& rows=rowsAt(show,liveCuelist);
        if(!rows.empty())liveFirst=rows.front().id;
        if(liveNextCue>=0 && liveNextCue<(int)rows.size())
            nextRowId=rows[(size_t)liveNextCue].id;
    }
    int targetLive=-1;
    for(int i=0;i<(int)snapshot.model.cuelists.size();++i) {
        const auto& rows=rowsAt(snapshot.model,i);
        if(!rows.empty() && rows.front().id==liveFirst && liveFirst>0) {
            targetLive=i;break;
        }
    }
    if(targetLive<0) targetLive=snapshot.model.activeCuelist;

    const auto liveModel=[&](const Show& base,int index) {
        Show result=base;
        result.rows=rowsAt(base,index);
        result.cuelists.clear();
        result.activeCuelist=0;
        result.nextId=0; // Dormant edits can allocate IDs without affecting LIVE.
        return result;
    };
    const auto oldLive=liveModel(show,juce::jlimit(0,
                            (int)show.cuelists.size()-1,liveCuelist));
    const auto newLive=liveModel(snapshot.model,targetLive);
    bool liveAffected=targetLive<0 ||
        encodeShow(oldLive)!=encodeShow(newLive);
    if(!liveAffected) {
        for(const auto& row:oldLive.rows)
            for(const auto& cell:row.cells) {
                const auto a=clips.find(cell.id);
                const auto b=snapshot.media.find(cell.id);
                const auto* oldPCM=a==clips.end()?nullptr:a->second.get();
                const auto* newPCM=b==snapshot.media.end()?nullptr:b->second.get();
                if(oldPCM!=newPCM) {liveAffected=true;break;}
            }
    }

    if(liveAffected) {
        engine.panic();
        resetActiveMapping();
    }
    closeMidiClip();closeOscClip();
    show=snapshot.model;
    clips=snapshot.media;
    liveCuelist=targetLive;
    const auto& newRows=rowsAt(show,liveCuelist);
    liveNextCue=0;
    if(nextRowId>0)
        for(int n=0;n<(int)newRows.size();++n)
            if(newRows[(size_t)n].id==nextRowId) {
                liveNextCue=n;break;
            }
    selectedRow=0;selectedColumn=0;
    nextCue=show.activeCuelist==liveCuelist ? liveNextCue : 0;
    refreshControlSettings();
    engine.publish(makeLiveRenderState(),!liveAffected);
    dirty=true;
    status=description+(liveAffected ?
           " LIVE transport stopped for a safe structural restore." :
           " LIVE transport preserved.");
    refresh();
    return true;
}
}
