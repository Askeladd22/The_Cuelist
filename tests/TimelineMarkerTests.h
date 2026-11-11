#pragma once

// RQ-005: absolute cue-local markers, v10 persistence and transport continuity.
void timelineMarkerTests() {
    using namespace cueViewFixtures;
    test("legacy tempo points import as absolute cue-local Tempo Markers", [] {
        auto show=makeShow();
        auto& cue=show.rows[0];
        cue.tempo={{0,120,4,4},{4,60,3,4},{6,95,7,8}};
        importLegacyTempoMarkers(cue);
        check(cue.markers.size()==2,"Legacy BPM points were lost");
        near(cue.markers[0].seconds,2.0);
        near(cue.markers[1].seconds,4.0);
        check(cue.markers[0].numerator==3 &&
              cue.markers[1].denominator==8,"Legacy meter was lost");
        cue.tempo.front().bpm=90;
        check(rebuildTempoFromMarkers(cue),"Tempo rebuild failed");
        near(cue.markers[0].seconds,2.0);
        near(cue.markers[1].seconds,4.0);
        near(secondsAtBeat(cue.tempo,cue.tempo[1].beat),2.0);
        near(secondsAtBeat(cue.tempo,cue.tempo[2].beat),4.0);
        check(validateShow(show).wasOk(),"Recalculated cue rejected");
    });
    test("v10 roundtrip preserves marker names types and absolute positions", [] {
        auto show=makeShow();
        addRow(show,"Other cue");
        auto& cue=show.rows[0];
        cue.markers.push_back({1,"LX 24",5.375,false,120,4,4});
        cue.markers.push_back({2,"Accelerando",15.0,true,128,4,4});
        cue.markers.push_back({3,"Ritardando",32.5,true,95,4,4});
        check(rebuildTempoFromMarkers(cue),"Tempo Map rebuild failed");
        check(validateShow(show).wasOk(),"Marker show invalid");
        const auto json=encodeShow(show);
        check(json.contains("\"version\": 10") || json.contains("\"version\":10"),
              "Persisted schema is not v10");
        Show restored;
        check(decodeShow(json,restored).wasOk(),"v10 marker roundtrip failed");
        check(restored.rows.size()==2 &&
              restored.rows[1].markers.empty(),"Markers leaked to other cue");
        const auto& markers=restored.rows[0].markers;
        check(markers.size()==3 && markers[0].name=="LX 24" &&
              !markers[0].changesTempo && markers[2].changesTempo,
              "Names or types changed during roundtrip");
        near(markers[0].seconds,5.375);
        near(markers[1].seconds,15.0);
        near(markers[2].seconds,32.5);
        near(restored.rows[0].tempo[1].bpm,128);
        near(restored.rows[0].tempo[2].bpm,95);
    });
    test("Generic Marker never changes tempo and removing Tempo Marker restores prior tempo", [] {
        auto show=makeShow();
        auto& cue=show.rows[0];
        cue.markers.push_back({1,"Pause",4.75,false,120,4,4});
        check(rebuildTempoFromMarkers(cue) && cue.tempo.size()==1,
              "Generic Marker incorrectly changed BPM");
        cue.markers.push_back({2,"Faster",8.0,true,140,4,4});
        cue.markers.push_back({3,"Slow",12.0,true,90,4,4});
        check(rebuildTempoFromMarkers(cue) && cue.tempo.size()==3,
              "Tempo Markers not applied");
        cue.markers.erase(cue.markers.begin()+1);
        check(rebuildTempoFromMarkers(cue) && cue.tempo.size()==2,
              "Deleting Tempo Marker did not restore previous BPM");
        near(cue.tempo[0].bpm,120);
        near(cue.tempo[1].bpm,90);
        near(secondsAtBeat(cue.tempo,cue.tempo[1].beat),12.0);
        cue.markers[1].changesTempo=false;
        check(rebuildTempoFromMarkers(cue) && cue.tempo.size()==1,
              "Converting Tempo Marker to generic changed initial BPM");
    });
    test("native Timeline double-click creates Generic Marker with Snap and inline rename", [] {
        MainComponent ui(false);
        auto* timeline=find<Timeline>(ui);
        check(timeline!=nullptr,"Timeline not found");
        timeline->setSize(800,260);
        const auto point=juce::Point<float>{350,90};
        timeline->mouseDoubleClick(event(*timeline,point,point,2));
        check(ui.show.rows[0].markers.size()==1,"Double click did not create marker");
        const auto marker=ui.show.rows[0].markers[0];
        check(!marker.changesTempo && marker.id>0,"New marker is not generic");
        near(marker.seconds,3.25); // 210px / 640px * 10s = 3.28125, snap = 3.25
        auto* editor=find<juce::TextEditor>(*timeline);
        check(editor && editor->getName()=="timelineMarkerInlineEditor" &&
              editor->isVisible(),"Marker inline editor did not open");
        editor->setText("Blackout",false);
        editor->onReturnKey();
        check(ui.show.rows[0].markers[0].name=="Blackout","Marker rename failed");
        check(ui.show.rows[0].tempo.size()==1,"Generic marker changed playback tempo");
    });
    test("compact native Timeline still exposes editable cue Markers", [] {
        MainComponent ui(false);
        auto* timeline=find<Timeline>(ui);
        check(timeline!=nullptr,"Timeline missing");
        timeline->setSize(800,140);
        const auto preview=timeline->createComponentSnapshot(timeline->getLocalBounds());
        check(preview.isValid(),"Compact marker lane snapshot invalid");
        const juce::Point<float> at{350,90};
        timeline->mouseDoubleClick(event(*timeline,at,at,2));
        check(ui.show.rows[0].markers.size()==1,
              "Compact Timeline made marker lane non-interactive");
        check(ui.show.rows[0].markers[0].name.isNotEmpty(),
              "Compact marker creation did not produce a usable marker");
    });
    test("clicking a marker label edits the marker instead of creating another", [] {
        MainComponent ui(false);
        auto* timeline=find<Timeline>(ui);check(timeline!=nullptr,"Timeline missing");
        timeline->setSize(800,260);
        const juce::Point<float> at{350,90};
        timeline->mouseDoubleClick(event(*timeline,at,at,2));
        auto* editor=find<juce::TextEditor>(*timeline);
        check(editor && editor->isVisible(),"New marker did not enter inline editing");
        editor->setText("Scene end",false);
        editor->onReturnKey();
        // The marker commit refreshes MainComponent and restores the real
        // workspace bounds. Locate the painted label at the CURRENT width,
        // not the artificial 800 px test size used before the edit.
        constexpr int rulerLeft=140, rulerRightInset=20;
        const int pixels=timeline->getWidth()-rulerLeft-rulerRightInset;
        check(pixels>0,"Timeline viewport disappeared after marker rename");
        const float markerX=(float)rulerLeft+
            (float)(pixels*ui.show.rows[0].markers[0].seconds/10.0);
        const juce::Point<float> onLabel{markerX+20.0f,90};
        timeline->mouseDoubleClick(event(*timeline,onLabel,onLabel,2));
        check(ui.show.rows[0].markers.size()==1,
              "Double-click on visible marker label created a duplicate");
        check(editor->isVisible(),"Double-click on marker label did not rename inline");
        editor->onEscapeKey();
    });
    test("free Snap-off marker movement does not alter PCM offsets", [] {
        MainComponent ui(false);
        auto* timeline=find<Timeline>(ui);check(timeline!=nullptr,"Timeline missing");
        timeline->setSize(800,260);
        juce::TextButton* magnet=nullptr;
        for(auto* child:timeline->getChildren())
            if(child->getName()=="timelineSnapToggle")
                magnet=dynamic_cast<juce::TextButton*>(child);
        check(magnet && magnet->getToggleState(),"Snap default missing");
        magnet->onClick();
        check(!magnet->getToggleState(),"Snap OFF did not activate");
        const juce::Point<float> clickAt{350,90};
        timeline->mouseDoubleClick(event(*timeline,clickAt,clickAt,2));
        near(ui.show.rows[0].markers[0].seconds,3.28125);
        auto* editor=find<juce::TextEditor>(*timeline);
        check(editor!=nullptr,"Marker editor missing");
        editor->onReturnKey();
        // After changed()/refresh(), the Timeline returns to the parent
        // workspace width. Hit the marker's CURRENT glyph position.
        constexpr int rulerLeft=140, rulerRightInset=20;
        const int pixels=timeline->getWidth()-rulerLeft-rulerRightInset;
        check(pixels>0,"Timeline viewport disappeared after marker creation");
        const double initial=ui.show.rows[0].markers[0].seconds;
        const float markerX=(float)rulerLeft+(float)(pixels*initial/10.0);
        const juce::Point<float> down{markerX,90},up{markerX+64.0f,90};
        timeline->mouseDown(event(*timeline,down,down));
        timeline->mouseDrag(event(*timeline,up,down,1,true));
        timeline->mouseUp(event(*timeline,up,down,1,true));
        near(ui.show.rows[0].markers[0].seconds,initial+640.0/pixels);
        near(ui.show.rows[0].cells[0].startSeconds,0);
    });
    test("marker lane remains interactive at compact Timeline height", [] {
        MainComponent ui(false);
        auto* timeline=find<Timeline>(ui);
        check(timeline!=nullptr,"Timeline missing");
        timeline->setSize(800,150);
        timeline->mouseDoubleClick(event(*timeline,{350.0f,90.0f},
                                             {350.0f,90.0f},2));
        check(ui.show.rows[0].markers.size()==1,
              "Compact Timeline did not permit marker creation");
        auto before=timeline->createComponentSnapshot(timeline->getLocalBounds());
        check(before.isValid(),"Compact Timeline snapshot failed");
        auto* editor=find<juce::TextEditor>(*timeline);
        check(editor && editor->isVisible(),
              "Compact Timeline did not expose inline marker name");
        editor->setText("Compact marker",false);
        editor->onReturnKey();
        check(ui.show.rows[0].markers[0].name=="Compact marker",
              "Compact marker rename failed");
    });
    test("tempo-map republish never stops active PCM MIDI OSC voices", [] {
        auto show=makeShow();
        addColumn(show,ColumnType::midi,"MIDI");
        addColumn(show,ColumnType::osc,"OSC");
        auto& cue=show.rows[0];
        cue.cells[1].midi=MidiClip{24,{{0,12,60,90,1}}};
        OscClip osc; osc.useBeats=true; osc.length=24;
        osc.events.push_back({2,"/stage",{}});
        cue.cells[2].osc=osc;
        auto audio=std::make_shared<AudioClip>();
        audio->sampleRate=48000;
        audio->samples.setSize(1,48000*4);
        audio->samples.clear();
        ClipMap clips;
        clips[cue.cells[0].id]=audio;
        AudioEngine engine;
        engine.monitoredRowId.store(cue.id);
        juce::AudioBuffer<float> output(2,64);
        engine.publish(makeRenderState(show,clips));
        check(engine.send(cue.id,true),"GO queue rejected");
        engine.processBlock(output.getArrayOfWritePointers(),2,64,48000);
        const double before=engine.playheadSeconds.load();
        check(before>0 && engine.rowPlaying.load(),"Cue not playing before tempo edit");
        for (auto& cell:cue.cells)
            check(engine.getCellPlayback(cell.id).playing,"Voice did not start");
        cue.markers.push_back({1,"Tempo",0.001,true,80,4,4});
        check(rebuildTempoFromMarkers(cue),"Tempo Map rebuild rejected");
        engine.publish(makeRenderState(show,clips),true,true);
        engine.processBlock(output.getArrayOfWritePointers(),2,64,48000);
        check(engine.rowPlaying.load() && engine.playheadSeconds.load()>before,
              "Tempo edit stopped or rewound LIVE transport");
        for (auto& cell:cue.cells)
            check(engine.getCellPlayback(cell.id).playing,
                  "Tempo map snapshot interrupted an active Audio/MIDI/OSC cell");
    });
}
