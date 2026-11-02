#include "MainComponent.h"
#include "OperatorReportDialog.h"
#include "ProjectSafety.h"
#include "ProjectMediaLoader.h"
#include "ControlLayerBuilder.h"

namespace cuelist {
// Project I/O, safe activation, recovery and operator preflight remain
// UI-thread-owned; background media preparation stays in ProjectLoadWorker.

void MainComponent::withDiscardConfirmation(std::function<void()> action) {
    if (!hasUnsavedChanges()) { action(); return; }
    // Only the explicitly labelled destructive button can invoke the action.
    // Cancel, Escape and closing the dialog leave the show untouched.
    showUnsavedChangesConfirmation(*this,theme,
        [safe=juce::Component::SafePointer<MainComponent>(this),
         action=std::move(action)] {
            if (safe && action) action();
        });
}

void MainComponent::requestQuit() { withDiscardConfirmation([this] { stopAll(); juce::JUCEApplication::getInstance()->quit(); }); }

void MainComponent::saveProject() {
    if (projectLoader) projectLoader->cancel();
    if (!applyInspector()) return;
    chooser=std::make_unique<juce::FileChooser>("Save show",projectFile==juce::File{} ? juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory().getChildFile("Show.cuelist") : projectFile,"*.cuelist");
    chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,
        [safe=juce::Component::SafePointer<MainComponent>(this)](const juce::FileChooser& fc) {
            if (!safe || fc.getResult()==juce::File{}) return;
            auto file=fc.getResult(); auto result=saveShow(safe->show,file);
            if (result.failed()) { safe->message(result.getErrorMessage()); return; }
            Show portable; if (loadShow(file,portable).wasOk()) safe->show=std::move(portable);
            safe->projectFile=file; safe->dirty=false; safe->status="Saved: "+file.getFullPathName();
            safe->rememberRecentProject(file);
            if (safe->recoveryEnabled) {
                if (safe->recoveryWriter) safe->recoveryWriter->flush();
                ProjectRecovery::discard(ProjectRecovery::defaultDirectory());
                safe->preserveDeferredRecovery=false;
            }
            safe->repaint();
        });
}

void MainComponent::showPreflight() {
    juce::StringArray outputs;
    for(const auto& d:juce::MidiOutput::getAvailableDevices()) outputs.add(d.identifier);
    const auto report=preflightShow(show,engine.connected.load(),outputs,midiInput.connected());
    const auto icon=report.criticalCount()>0 ? juce::MessageBoxIconType::WarningIcon :
        report.warningCount()>0 ? juce::MessageBoxIconType::InfoIcon :
        juce::MessageBoxIconType::NoIcon;
    juce::AlertWindow::showMessageBoxAsync(icon,"Show preflight",report.describe(),"Close",this);
    status=report.criticalCount()>0 ? "Preflight: critical problems detected." :
           report.warningCount()>0 ? "Preflight: warnings to review." :
           "Preflight complete: no critical project errors.";
    repaint();
}

void MainComponent::showDiagnostics() {
    using Status=OperatorReportRow::Status;
    const auto stats=engine.diagnostics();
    const auto online=engine.connected.load();
    const auto midiConnected=midiInput.connected();
    const auto count=[](auto value){ return juce::String(value); };
    const auto drops=[](auto value){
        return value==0 ? Status::neutral : Status::negative;
    };
    showOperatorReport(*this,theme,"Diagnostics",
        "Engine diagnostics · current process",{
            {"AUDIO",{
                {"Device",online?"ONLINE":"OFFLINE",
                    online?Status::positive:Status::caution},
                {"Active audio / MIDI / OSC cells",count(engine.activeVoices.load())},
                {"Callback commands dropped",count(stats.audioCommandsDropped),
                    drops(stats.audioCommandsDropped)}
            }},
            {"MIDI",{
                {"Input",midiConnected?"CONNECTED":"DISCONNECTED",
                    midiConnected?Status::positive:Status::caution},
                {"Input messages dropped",count(midiInput.droppedCount()),
                    drops(midiInput.droppedCount())},
                {"Reconnect attempts",count(midiInputReconnectAttempts)},
                {"Output messages sent",count(stats.midiOutputSent)},
                {"Output messages dropped",count(stats.midiOutputDropped),
                    drops(stats.midiOutputDropped)},
                {"Dispatches over 10 ms late",count(stats.midiLateOver10ms),
                    stats.midiLateOver10ms==0?Status::neutral:Status::caution}
            }},
            {"OSC",{
                {"UDP packets sent",count(stats.oscSent)},
                {"UDP drops / send failures",count(stats.oscDropped),
                    drops(stats.oscDropped)},
                {"Dispatches over 10 ms late",count(stats.oscLateOver10ms),
                    stats.oscLateOver10ms==0?Status::neutral:Status::caution}
            }},
            {"RECOVERY",{
                {"I/O failures",count(recoveryWriter?recoveryWriter->failedWrites():0u),
                    drops(recoveryWriter?recoveryWriter->failedWrites():0u)}
            }}
        },
        "Counters are local to this process. UDP packets sent do not "
        "confirm delivery to a remote receiver.");
}

bool MainComponent::activateProject(Show candidate,const juce::File& source,bool recovering) {
    if (projectLoader) projectLoader->cancel();
    PreparedShowMedia prepared;
    const auto preparedResult=prepareShowMedia(candidate,prepared);
    if(preparedResult.failed()) {
        message(preparedResult.getErrorMessage()+
                " The current show has not been replaced.");
        return false;
    }
    return commitPreparedProject(std::move(candidate),source,recovering,std::move(prepared));
}

bool MainComponent::commitPreparedProject(Show candidate,const juce::File& source,
                                           bool recovering,PreparedShowMedia prepared) {
    // Migration is transactional and happens before stopping any playing
    // show. A locked legacy Control Column becomes Default Map functions;
    // dormant Q sets are archived, never unexpectedly activated.
    bool migratedLegacy=false;
    const auto migration=migrateLegacyControlColumns(candidate,migratedLegacy);
    if(migration.failed()) {
        message(migration.getErrorMessage()+
                " The original project remains loaded and unchanged.");
        return false;
    }
    // Only the UI thread may interrupt playback or touch the AudioEngine.
    // Decode and memory-budget checks have already completed transactionally.
    auto loaded=std::move(prepared.clips);
    auto issues=std::move(prepared.issues);
    // Only after all memory checks pass do we stop the previous show.
    engine.panic();
    for(const auto& column:candidate.columns)
        if(column.type==ColumnType::midi && column.midiOutputId.isNotEmpty()) {
            const auto opened=engine.setMidiOutput(column.midiOutputId);
            if(opened.failed()) issues.add(opened.getErrorMessage());
        }
    closeMidiClip();closeOscClip();editHistory.clearUndoHistory();
    show=std::move(candidate);clips=std::move(loaded);
    projectFile=recovering ? juce::File{} : source;
    dirty=recovering || migratedLegacy;
    if(!recovering) rememberRecentProject(source);
    selectedRow=selectedColumn=nextCue=0;
    resetActiveMapping();refreshControlSettings();
    engine.publish(makeRenderState(show,clips));refresh();
    status=recovering ? "Recovered unsaved show. Use Save to choose a project filename." :
                        migratedLegacy ?
                            "Legacy Control Columns migrated to Default Map; review imported functions and Save to persist." :
                            "Project opened: "+source.getFileName();
    if(!recovering && recoveryEnabled) {
        if (recoveryWriter) recoveryWriter->flush();
        ProjectRecovery::discard(ProjectRecovery::defaultDirectory());
        preserveDeferredRecovery=false;
    }
    if(!issues.isEmpty())
        message(juce::String(issues.size())+" missing media or MIDI output issues.\n\n"+
                issues.joinIntoString("\n").substring(0,1500));
    return true;
}

void MainComponent::offerCrashRecovery() {
    if(!ProjectRecovery::exists(ProjectRecovery::defaultDirectory())) return;
    auto safe=juce::Component::SafePointer<MainComponent>(this);
    juce::AlertWindow::showAsync(
        juce::MessageBoxOptions().withIconType(juce::MessageBoxIconType::QuestionIcon)
            .withTitle("Recover an unsaved show?")
            .withMessage("A recovery checkpoint from an interrupted session was found. "
                         "Restoring does not overwrite the original project or any media. "
                         "Playback will not start automatically.")
            .withButton("Restore").withButton("Discard checkpoint").withButton("Later")
            .withAssociatedComponent(this),[safe](int answer) {
        if(!safe) return;
        if(answer==1) {
            Show candidate;
            const auto result=ProjectRecovery::restore(ProjectRecovery::defaultDirectory(),candidate);
            if(result.failed()) safe->message(result.getErrorMessage());
            else safe->activateProject(std::move(candidate),{},true);
        } else if(answer==2) {
            if (safe->recoveryWriter) safe->recoveryWriter->flush();
            ProjectRecovery::discard(ProjectRecovery::defaultDirectory());
        }
        else {
            safe->preserveDeferredRecovery=true;
            safe->status="Recovery deferred: previous checkpoint preserved until you decide.";
            safe->repaint();
        }
    });
}


void MainComponent::restoreRecentProjects() {
    recentProjects.clear();
    if(!appPreferences) return;
    for(int i=0;i<maxRecentProjects;++i) {
        const auto path=appPreferences->getValue(
            "recentProject"+juce::String(i)).trim();
        if(path.isEmpty()) continue;
        const juce::File file(path);
        // Do not offer nonexistent files as live actions (e.g. a detached
        // removable drive). The last known preference can be recovered later.
        if(file.existsAsFile() &&
           !recentProjects.contains(file.getFullPathName(),true))
            recentProjects.add(file.getFullPathName());
    }
    refreshRecentProjectButtons();
}
void MainComponent::rememberRecentProject(const juce::File& file) {
    if(file==juce::File{} || !file.existsAsFile()) return;
    const auto path=file.getFullPathName();
    for(int i=recentProjects.size();--i>=0;)
        if(recentProjects[i].equalsIgnoreCase(path))
            recentProjects.remove(i);
    recentProjects.insert(0,path);
    while(recentProjects.size()>maxRecentProjects)
        recentProjects.remove(recentProjects.size()-1);
    if(appPreferences) {
        for(int i=0;i<maxRecentProjects;++i) {
            const auto key="recentProject"+juce::String(i);
            if(i<recentProjects.size())
                appPreferences->setValue(key,recentProjects[i]);
            else
                appPreferences->removeValue(key);
        }
        appPreferences->saveIfNeeded();
    }
    refreshRecentProjectButtons();
    resized();
    repaint();
}
void MainComponent::refreshRecentProjectButtons() {
    for(int i=0;i<visibleRecentProjects;++i) {
        if(!recentProjectButtons[(size_t)i] ||
           !recentProjectDetails[(size_t)i]) continue;
        auto& button=*recentProjectButtons[(size_t)i];
        auto& detail=*recentProjectDetails[(size_t)i];
        if(i>=recentProjects.size()) {
            button.setButtonText({});
            detail.setText({},juce::dontSendNotification);
            continue;
        }
        const juce::File file(recentProjects[i]);
        const auto modified=file.getLastModificationTime().toString(
            true,false,false,true);
        button.setButtonText(file.getFileNameWithoutExtension());
        detail.setText(file.getParentDirectory().getFileName()+" · "+modified,
                       juce::dontSendNotification);
        button.setTooltip(file.getFullPathName()+"\nModified: "+modified);
    }
}
void MainComponent::requestProjectLoad(const juce::File& file) {
    if(!file.existsAsFile()) {
        message("Project file unavailable: "+file.getFullPathName());
        return;
    }
    if(!projectLoader)
        projectLoader=std::make_unique<ProjectLoadWorker>();
    projectLoader->request(file);
    status="Preparing project in background: "+file.getFileName()+
        ". Current show stays loaded until preparation succeeds.";
    repaint();
}
void MainComponent::openRecentProject(int index) {
    if(index<0 || index>=recentProjects.size()) return;
    const juce::File file(recentProjects[index]);
    if(!file.existsAsFile()) {
        message("Project file unavailable: "+file.getFullPathName());
        return;
    }
    withDiscardConfirmation(
        [safe=juce::Component::SafePointer<MainComponent>(this),file] {
            if(safe) safe->requestProjectLoad(file);
        });
}
void MainComponent::showAllRecentProjects() {
    juce::PopupMenu menu;
    menu.addItem(1,"Open...");
    if(!recentProjects.isEmpty()) menu.addSeparator();
    for(int i=0;i<recentProjects.size();++i)
        menu.addItem(100+i,juce::File(recentProjects[i]).getFileNameWithoutExtension());
    auto safe=juce::Component::SafePointer<MainComponent>(this);
    menu.showMenuAsync(
        juce::PopupMenu::Options().withTargetComponent(&recentMoreButton),
        [safe](int choice) {
            if(!safe) return;
            if(choice==1)
                safe->withDiscardConfirmation([safe] {
                    if(safe) safe->openProject();
                });
            else if(choice>=100) safe->openRecentProject(choice-100);
        });
}

void MainComponent::openProject() {
    chooser=std::make_unique<juce::FileChooser>("Open show",projectFile,"*.cuelist;*.json;*.bak");
    chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
        [safe=juce::Component::SafePointer<MainComponent>(this)](const juce::FileChooser& fc) {
            if(!safe || fc.getResult()==juce::File{}) return;
            safe->requestProjectLoad(fc.getResult());
        });
}

} // namespace cuelist
