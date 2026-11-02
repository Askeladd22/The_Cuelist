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
        tc={23,59,59,29};
        increment(tc,Rate::fps30);
        check(tc.hour==0 && tc.minute==0 && tc.second==0 && tc.frame==0,
              "Non-drop LTC failed 24-hour rollover");
    });
    test("N6 SMPTE LTC uses correct 80-bit word, sync, BCD and phase parity",[] {
        for(const auto rate:{Rate::fps24,Rate::fps25,Rate::drop2997,Rate::fps30}) {
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
