#pragma once
#include "ClickTrack.h"

// Procedural audio: independent of WAV media, sharing Timeline's beat-to-time conversion.
void clickTrackTests() {
    using namespace cuelist;
    test("Click Track persists as optional JSON v10 Audio cell without a file", [] {
        auto show=makeShow();
        auto& cell=show.rows[0].cells[0];
        cell.click=ClickTrackConfig{ClickSound::woodblock,true,3,6.0};
        cell.name="Count-in";
        check(validateShow(show).wasOk(),"Valid Click Track rejected");
        const auto json=encodeShow(show);
        check(json.contains("\"version\": 10") || json.contains("\"version\":10"),
              "Click Track altered JSON schema version");
        Show restored;
        check(decodeShow(json,restored).wasOk(),"Click Track v10 restore failed");
        const auto& value=restored.rows[0].cells[0];
        check(value.click.has_value() && value.file.isEmpty(),"Procedural source or configuration lost");
        check(value.click->sound==ClickSound::woodblock && value.click->accent &&
              value.click->subdivision==3,"Click Track parameters lost");
        near(value.click->durationSeconds,6.0);
    });
    test("Click Track subdivides beats through non-aligned Tempo Markers", [] {
        const std::vector<TempoPoint> tempo{{0,120,4,4},{2.5,60,4,4}};
        ClickTrackConfig config{ClickSound::classicClick,true,2,3.0};
        const auto events=makeClickEvents(tempo,config);
        check(events.size()==9,"Unexpected click event count across BPM change");
        near(events[0].seconds,0.0);
        near(events[5].seconds,1.25);
        near(events[6].seconds,1.75);
        near(events[8].seconds,2.75);
        for(size_t i=1;i<events.size();++i)
            check(events[i].seconds>events[i-1].seconds,"Click clock not strictly increasing");
    });
    test("Click Track subdivisions and meter accents follow the musical grid", [] {
        const std::vector<TempoPoint> tempo{{0,120,3,4}};
        for(int subdivision=1;subdivision<=4;++subdivision) {
            ClickTrackConfig config{ClickSound::electronicBeep,true,subdivision,3.1};
            auto events=makeClickEvents(tempo,config);
            check(events.size()==(size_t)(6*subdivision+1),"Wrong subdivision count");
            check(events.front().accent,"Missing initial downbeat");
            int accents=0;
            for(const auto& event:events) accents+=(int)event.accent;
            check(accents==3,"Accent must mark the first beat of every 3/4 bar");
            config.accent=false;
            for(const auto& event:makeClickEvents(tempo,config))
                check(!event.accent,"Accent OFF emitted an accented pulse");
        }
    });
    test("Click Track validates sources and finite bounded configuration", [] {
        auto show=makeShow();
        auto& cell=show.rows[0].cells[0];
        cell.click=ClickTrackConfig{};
        cell.click->durationSeconds=0;
        check(validateShow(show).failed(),"Zero Click Track duration accepted");
        cell.click->durationSeconds=5;
        cell.click->subdivision=5;
        check(validateShow(show).failed(),"Invalid Click Track subdivision accepted");
        cell.click->subdivision=1;
        cell.file="C:/existing.wav";
        check(validateShow(show).failed(),"Click Track overlaid existing file");
    });
    test("Click Track Audio engine follows tempo edits without restarting LIVE", [] {
        auto show=makeShow();
        auto& row=show.rows[0];
        row.cells[0].click=ClickTrackConfig{ClickSound::rimshot,true,4,4.0};
        AudioEngine engine;
        engine.publish(makeRenderState(show,{}));
        check(engine.send(row.id,true),"Click Track row GO rejected");
        juce::AudioBuffer<float> output(2,1024);
        engine.processBlock(output.getArrayOfWritePointers(),2,1024,48000);
        check(engine.getCellPlayback(row.cells[0].id).playing,"Click Track voice did not start");
        float peak=output.getMagnitude(0,1024);
        check(peak>0.001f,"Click Track produced no Audio");
        const auto before=engine.getCellPlayback(row.cells[0].id).positionSeconds;
        row.markers.push_back({1,"Tempo",0.011,true,76,4,4});
        check(rebuildTempoFromMarkers(row),"Tempo update rejected");
        engine.publish(makeRenderState(show,{}),true,true);
        engine.processBlock(output.getArrayOfWritePointers(),2,1024,48000);
        const auto after=engine.getCellPlayback(row.cells[0].id);
        check(after.playing && engine.rowPlaying.load(),"Tempo edit stopped Click Track LIVE");
        check(after.positionSeconds>before,"Tempo edit rewound Click Track LIVE");
    });
}
