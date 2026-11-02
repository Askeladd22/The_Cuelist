#pragma once
#include "LtcGenerator.h"
#include "AudioEngine.h"
#include <algorithm>
#include <cmath>

void ltcGeneratorTests() {
    using namespace cuelist;
    using namespace cuelist::ltc;
    test("N6 LTC timecode format validates rate, DF and forbidden labels",[] {
        Timecode tc;
        check(parseTimecode("01:23:45:23",Rate::fps24,tc).wasOk() &&
              tc.hour==1 && tc.minute==23 && tc.second==45 && tc.frame==23,
              "24fps start timecode parsing failed");
        check(parseTimecode("01:23:45:24",Rate::fps24,tc).failed(),
              "24fps accepts frame 24");
        check(parseTimecode("00:01:00;00",Rate::drop2997,tc).failed() &&
              parseTimecode("00:01:00;01",Rate::drop2997,tc).failed() &&
              parseTimecode("00:01:00;02",Rate::drop2997,tc).wasOk(),
              "29.97 drop-frame accepts skipped labels");
        check(parseTimecode("00:10:00;00",Rate::drop2997,tc).wasOk(),
              "29.97 drop-frame incorrectly skips tenth-minute labels");
        check(parseTimecode("00:01:00:00",Rate::nonDrop2997,tc).wasOk() &&
              parseTimecode("00:01:00;00",Rate::nonDrop2997,tc).failed(),
              "29.97 non-drop must retain all frame numbers and colon syntax");
        check(parseTimecode("00:00:00;00",Rate::fps25,tc).failed() &&
              parseTimecode("00:00:00:30",Rate::fps30,tc).failed() &&
              parseTimecode("junk",Rate::fps25,tc).failed(),
              "Malformed LTC text was accepted");
    });
    test("N6 LTC frame numbering handles 29.97 drop-frame minute and ten-minute rollover",[] {
        Timecode tc{0,0,59,29};
        increment(tc,Rate::drop2997);
        check(tc.hour==0 && tc.minute==1 && tc.second==0 && tc.frame==2,
              "Drop-frame did not skip first two non-tenth minute labels");
        tc={0,9,59,29};
        increment(tc,Rate::drop2997);
        check(tc.hour==0 && tc.minute==10 && tc.second==0 && tc.frame==0,
              "Drop-frame skipped the tenth-minute boundary incorrectly");
        tc={0,0,59,29};
        increment(tc,Rate::nonDrop2997);
        check(tc.minute==1 && tc.second==0 && tc.frame==0,
              "29.97 non-drop skipped frame labels");
        tc={23,59,59,29};
        increment(tc,Rate::fps30);
        check(tc.hour==0 && tc.minute==0 && tc.second==0 && tc.frame==0,
              "Non-drop LTC failed 24-hour rollover");
    });
    test("N6 SMPTE LTC uses correct 80-bit word, sync, BCD and phase parity",[] {
        for(const auto rate:{Rate::fps24,Rate::fps25,Rate::drop2997,
                             Rate::nonDrop2997,Rate::fps30}) {
            const auto bits=frameBits({12,34,56,12},rate);
            constexpr char sync[]="0011111111111101";
            for(int i=0;i<16;++i)
                check(bits[(size_t)(64+i)]==(sync[i]=='1'),
                      "LTC frame sync word is incorrect");
            const auto bcd=[&bits](int pos,int count){
                int value=0;
                for(int k=0;k<count;++k)
                    value|=((int)bits[(size_t)(pos+k)]<<k);
                return value;
            };
            check(bcd(0,4)==2 && bcd(8,2)==1 &&
                  bcd(16,4)==6 && bcd(24,3)==5 &&
                  bcd(32,4)==4 && bcd(40,3)==3 &&
                  bcd(48,4)==2 && bcd(56,2)==1 &&
                  bits[10]==(rate==Rate::drop2997 ? 1 : 0),
                  "LTC time address did not use BCD fields correctly");
            int zeros=0;
            for(const auto bit:bits) zeros+=bit==0;
            check(zeros%2==0,
                  "LTC biphase phase-correction/parity failed");
        }
    });
    test("LTC Audio menu eligibility forbids existing content and collapsed cells",[] {
        MainComponent ui(false);
        check(ui.canGenerateLtcTrack(0,0) &&
              !ui.canGenerateLtcTrack(-1,0) &&
              !ui.canGenerateLtcTrack(0,-1) &&
              !ui.canGenerateLtcTrack(0,100),
              "Generate LTC was offered outside an empty Audio cell");
        auto& cell=ui.show.rows[0].cells[0];
        cell.file="C:/media/existing.wav";
        check(!ui.canGenerateLtcTrack(0,0),
              "Generate LTC would replace existing Audio file");
        cell.file.clear();
        auto audio=std::make_shared<AudioClip>();
        audio->samples.setSize(1,128);audio->samples.clear();
        ui.clips[cell.id]=audio;
        check(!ui.canGenerateLtcTrack(0,0),
              "Generate LTC would replace loaded PCM");
        ui.clips.erase(cell.id);
        cell.midi.emplace();
        check(!ui.canGenerateLtcTrack(0,0),
              "Generate LTC would replace embedded MIDI media");
        cell.midi.reset();
        cell.osc.emplace();
        check(!ui.canGenerateLtcTrack(0,0),
              "Generate LTC would replace embedded OSC media");
        cell.osc.reset();
        ui.show.columns[0].collapsed=true;
        check(!ui.canGenerateLtcTrack(0,0),
              "Generate LTC is available in a collapsed column");
        ui.show.columns[0].collapsed=false;
        check(ui.canGenerateLtcTrack(0,0),
              "Restoring an empty Audio cell did not restore LTC action");
    });
    test("Generated LTC advances in actual WAV across frame boundaries at every rate",[] {
        const auto folder=juce::File::getSpecialLocation(
            juce::File::tempDirectory).getChildFile("TheCuelist-LtcTests");
        check(folder.createDirectory().wasOk(),"Cannot create LTC test directory");
        for(const auto rate:{Rate::fps24,Rate::fps25,Rate::drop2997,
                             Rate::nonDrop2997,Rate::fps30}) {
            Config cfg;
            cfg.rate=rate;
            cfg.start={0,0,59,nominalFps(rate)-2};
            cfg.durationSeconds=1.25;
            const auto file=folder.getNonexistentChildFile("ltc-progress",".wav");
            check(writeWav(file,cfg).wasOk(),"LTC writer failed for supported fps");
            const auto originalBytes=file.getSize();
            check(writeWav(file,cfg).failed() && file.getSize()==originalBytes,
                  "LTC generator unexpectedly overwrote an existing WAV");
            std::shared_ptr<const AudioClip> clip;
            check(loadClip(file,clip).wasOk() && clip &&
                  clip->sampleRate==48000 &&
                  clip->samples.getNumSamples()==60000,
                  "Generated LTC is not a full-length normal 48k Audio clip");
            auto time=cfg.start;
            for(int frame=0;frame<nominalFps(rate);++frame) {
                const int64_t first=((int64_t)frame*48000*denominator(rate)+
                                        numerator(rate)/2)/numerator(rate);
                const int64_t next=((int64_t)(frame+1)*48000*denominator(rate)+
                                        numerator(rate)/2)/numerator(rate);
                const int frameSamples=(int)(next-first);
                const auto expected=frameBits(time,rate);
                for(int bit=0;bit<80;++bit) {
                    const auto level=[&](int half) {
                        const int64_t relative=(int64_t)(2*half+1)*
                                               frameSamples/320;
                        return clip->samples.getSample(
                            0,(int)(first+relative))>0.f;
                    };
                    const bool midpoint=level(bit*2)!=level(bit*2+1);
                    check(midpoint==(expected[(size_t)bit]!=0),
                          "Waveform data does not advance through correct LTC frames");
                }
                increment(time,rate);
            }
            if(rate==Rate::drop2997)
                check(time.minute==1 && time.second==1 &&
                      time.frame==0,
                      "Drop-frame advancement across the first minute is wrong");
            file.deleteFile();
        }
    });

    test("N6 LTC WAV is real decodable Audio with biphase-mark transitions",[] {
        const auto folder=juce::File::getSpecialLocation(
            juce::File::tempDirectory).getChildFile("TheCuelist-LtcTests");
        check(folder.createDirectory().wasOk(),"Cannot create LTC test directory");
        const auto path=folder.getNonexistentChildFile("frame-check",".wav");
        Config cfg;cfg.rate=Rate::fps24;
        cfg.start={1,2,3,4};cfg.durationSeconds=.5;
        check(writeWav(path,cfg).wasOk() && path.existsAsFile(),
              "LTC writer did not produce a WAV");
        std::shared_ptr<const AudioClip> clip;
        check(loadClip(path,clip).wasOk() && clip!=nullptr &&
              clip->samples.getNumChannels()==1 &&
              clip->sampleRate==48000 &&
              clip->samples.getNumSamples()==24000,
              "LTC WAV cannot be imported by the original Audio loader");
        const auto expected=frameBits(cfg.start,cfg.rate);
        const int samplesPerFrame=48000/24;
        for(int i=0;i<80;++i) {
            // Inspect the centre of each half-bit. For BMC, bit=1 has a
            // mandatory extra transition at the midpoint of its bit cell.
            const auto sample=[&](int half) {
                const int n=(int)(((int64_t)(2*half+1)*samplesPerFrame)/320);
                return clip->samples.getSample(0,n);
            };
            const bool middleTransition=
                (sample(i*2)>0)!=(sample(i*2+1)>0);
            check(middleTransition==(expected[(size_t)i]!=0),
                  "LTC audio's midpoint polarity disagrees with encoded data");
        }
        path.deleteFile();
    });
}
