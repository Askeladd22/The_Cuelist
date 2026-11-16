#pragma once
#include "AudioEngine.h"
#include "MainComponent.h"

void audioTimelineEditTests() {
    test("Audio trim applies source offset and both linear fades on real PCM output", [] {
        auto show=makeShow();
        auto& cell=show.rows[0].cells[0];
        cell.audioOffsetSeconds=0.5;
        cell.audioDurationSeconds=1.5;
        cell.fadeInSeconds=0.5;
        cell.fadeOutSeconds=0.5;
        auto clip=std::make_shared<AudioClip>();
        clip->sampleRate=4;
        clip->samples.setSize(1,12);
        for(int i=0;i<12;++i)clip->samples.setSample(0,i,(float)(i+1)/12.0f);
        ClipMap clips {{cell.id,clip}};
        const auto region=resolvedAudioRegion(cell,*clip);
        check(region.offsetFrames==2 && region.lengthFrames==6,
              "Audio region must resolve to source frames [2..8)");
        auto state=makeRenderState(show,clips);
        state->trigger(cell.id,false);
        juce::AudioBuffer<float> out(2,6);
        state->render(out.getArrayOfWritePointers(),2,6,4);
        const double expected[]={0,1.0/6.0,5.0/12.0,0.5,7.0/24.0,0};
        for(int frame=0;frame<6;++frame)for(int ch=0;ch<2;++ch)
            near(out.getSample(ch,frame),expected[frame]);
        check(!state->voices[0].playing,"Trimmed Audio must stop at the new right edge");
        state->render(out.getArrayOfWritePointers(),2,6,4);
        near(out.getMagnitude(0,6),0);
    });
    test("Trimmed Audio loops its region and reapplies fades without changing source", [] {
        auto show=makeShow();
        auto& cell=show.rows[0].cells[0];
        cell.loop=true;cell.audioOffsetSeconds=.5;
        cell.audioDurationSeconds=1.5;cell.fadeInSeconds=.5;cell.fadeOutSeconds=.5;
        auto clip=std::make_shared<AudioClip>();
        clip->sampleRate=4;clip->samples.setSize(1,16);
        for(int i=0;i<16;++i)clip->samples.setSample(0,i,(float)(i+1)/16);
        ClipMap clips {{cell.id,clip}};
        auto state=makeRenderState(show,clips);
        state->trigger(cell.id,false);
        juce::AudioBuffer<float> out(2,7);
        state->render(out.getArrayOfWritePointers(),2,7,4);
        near(out.getSample(0,0),0);
        near(out.getSample(0,5),0); // fade-out at trimmed right edge
        near(out.getSample(0,6),0); // next iteration fades in again
        check(state->voices[0].playing && clip->samples.getSample(0,2)>0,
              "Looping trim must not mutate the source PCM or stop the voice");
    });
    test("Edited Audio trims and fades survive JSON v10 roundtrip; old projects default to full media", [] {
        auto show=makeShow();
        auto& cell=show.rows[0].cells[0];
        cell.audioOffsetSeconds=.5;cell.audioDurationSeconds=3;
        cell.fadeInSeconds=.4;cell.fadeOutSeconds=1.2;
        Show decoded;
        const auto json=encodeShow(show);
        check(json.contains("\"version\": 10") || json.contains("\"version\":10"),
              "An Audio edit changed the project schema");
        check(decodeShow(json,decoded).wasOk(),"Edited v10 Audio properties did not decode");
        near(decoded.rows[0].cells[0].audioOffsetSeconds,.5);
        near(decoded.rows[0].cells[0].audioDurationSeconds,3);
        near(decoded.rows[0].cells[0].fadeInSeconds,.4);
        near(decoded.rows[0].cells[0].fadeOutSeconds,1.2);
        auto legacy=juce::JSON::parse(json);
        auto rows=legacy.getProperty("rows",{});
        auto cells=rows.getArray()->getReference(0).getProperty("cells",{});
        auto* props=cells.getArray()->getReference(0).getDynamicObject();
        for(const auto* key:{"audioOffsetSeconds","audioDurationSeconds",
                             "fadeInSeconds","fadeOutSeconds"})
            props->removeProperty(juce::Identifier(key));
        check(decodeShow(juce::JSON::toString(legacy),decoded).wasOk(),
              "Legacy v10 with no Audio edit fields did not load");
        near(decoded.rows[0].cells[0].audioOffsetSeconds,0);
        near(decoded.rows[0].cells[0].audioDurationSeconds,0);
        near(decoded.rows[0].cells[0].fadeInSeconds,0);
        near(decoded.rows[0].cells[0].fadeOutSeconds,0);
        auto invalid=show;
        invalid.rows[0].cells[0].fadeInSeconds=4;
        check(validateShow(invalid).failed(),"Out-of-range fade accepted");
        invalid=show;invalid.rows[0].cells[0].audioOffsetSeconds=-1;
        check(validateShow(invalid).failed(),"Negative Audio offset accepted");
    });
    test("Audio trim commit uses current clip length and keeps unrelated LIVE playback", [] {
        MainComponent ui(false);
        auto clip=std::make_shared<AudioClip>();
        clip->sampleRate=4;clip->samples.setSize(1,32);
        for(int i=0;i<32;++i)clip->samples.setSample(0,i,.25f);
        const int cellId=ui.show.rows[0].cells[0].id;
        ui.clips[cellId]=clip;
        // Manually mounted test media must enter global history before the
        // trim gesture; a real Import action creates that checkpoint.
        ui.commitModelEditForTest("Prepare PCM fixture");
        check(ui.engine.send(cellId,false),"Cannot start Audio test voice");
        juce::AudioBuffer<float> out(2,1);
        ui.engine.processBlock(out.getArrayOfWritePointers(),2,1,4);
        check(ui.engine.getCellPlayback(cellId).playing,"Setup voice not LIVE");
        const auto oldCuelist=ui.liveCuelistIndex();
        check(ui.setAudioClipRegion(0,0,0,1,4,.5,.5),
              "Valid trim/fade rejected");
        ui.engine.processBlock(out.getArrayOfWritePointers(),2,1,4);
        check(ui.engine.getCellPlayback(cellId).playing,
              "Audio region edit stopped an ongoing LIVE voice");
        check(ui.liveCuelistIndex()==oldCuelist,"Trim switched LIVE Cuelist");
        check(!ui.setAudioClipRegion(0,0,0,7.999,4,.5,.5),
              "Impossible source boundary was silently accepted");
        const auto before=encodeShow(ui.show);
        check(ui.undoEdit(),"Audio region global Undo unavailable");
        ui.engine.processBlock(out.getArrayOfWritePointers(),2,1,4);
        check(ui.engine.getCellPlayback(cellId).playing,
              "Undo of a pure Audio trim sent PANIC to LIVE");
        check(ui.redoEdit(),"Audio region global Redo unavailable");
        ui.engine.processBlock(out.getArrayOfWritePointers(),2,1,4);
        check(ui.engine.getCellPlayback(cellId).playing,
              "Redo of a pure Audio trim sent PANIC to LIVE");
        check(encodeShow(ui.show)==before,"Redo did not restore exact Audio edit");
    });
}
