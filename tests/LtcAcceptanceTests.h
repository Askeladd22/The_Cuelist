#pragma once
#include "LtcGenerator.h"
#include "AudioEngine.h"
#include <array>
#include <cmath>

// Test-only PCM receiver: intentionally does not call frameBits() or increment().
// It recovers bits by detecting BMC mid-cell transitions from the actual WAV.
void ltcAcceptanceTests() {
    using namespace cuelist;
    using namespace cuelist::ltc;
    test("Independent PCM decoder recovers LTC time address, flags and sync at each frame rate",[] {
        const auto folder=juce::File::getSpecialLocation(
            juce::File::tempDirectory).getChildFile("TheCuelist-LtcAcceptance");
        check(folder.createDirectory().wasOk(),"Cannot create LTC acceptance test directory");
        for (const auto rate:{Rate::fps24,Rate::fps25,Rate::drop2997,
                              Rate::nonDrop2997,Rate::fps30}) {
            Config cfg;
            cfg.rate=rate;
            cfg.start={12,0,59,nominalFps(rate)-2};
            cfg.durationSeconds=.25;
            const auto file=folder.getNonexistentChildFile("decode",".wav");
            check(writeWav(file,cfg).wasOk(),"LTC WAV fixture generation failed");
            std::shared_ptr<const AudioClip> clip;
            check(loadClip(file,clip).wasOk() && clip!=nullptr,
                  "Independent LTC receiver cannot load generated WAV");
            check(clip->samples.getNumChannels()==1 &&
                  clip->sampleRate==48000,"LTC source is not mono PCM 48kHz");
            const auto* pcm=clip->samples.getReadPointer(0);
            const auto fps=nominalFps(rate);
            for(int frame=0;frame<4;++frame) {
                // Receiver computes frame boundaries from elapsed PCM time,
                // not using generator.frameBits() or generator.increment().
                const auto first=(int64_t)std::llround(
                    (double)frame*48000.0*denominator(rate)/numerator(rate));
                const auto last=(int64_t)std::llround(
                    (double)(frame+1)*48000.0*denominator(rate)/numerator(rate));
                check(last>first && last<=clip->samples.getNumSamples(),
                      "Expected LTC frame lies beyond generated WAV");
                const auto phase=[&](int half) {
                    const auto sample=first+
                        ((int64_t)(2*half+1)*(last-first))/320;
                    return pcm[(size_t)sample]>=0.0f;
                };
                std::array<unsigned char,80> bits{};
                for(int bit=0;bit<80;++bit)
                    bits[(size_t)bit]=(unsigned char)(
                        phase(2*bit)!=phase(2*bit+1));
                const auto bcd=[&](int start,int width) {
                    int value=0;
                    for(int bit=0;bit<width;++bit)
                        value+=(int)bits[(size_t)(start+bit)]<<bit;
                    return value;
                };
                const int tcFrame=bcd(0,4)+10*bcd(8,2);
                const int tcSecond=bcd(16,4)+10*bcd(24,3);
                const int tcMinute=bcd(32,4)+10*bcd(40,3);
                const int tcHour=bcd(48,4)+10*bcd(56,2);
                const int expectedFrame=frame<2 ? fps-2+frame :
                    frame-2+(rate==Rate::drop2997 ? 2 : 0);
                check(tcHour==12 && tcMinute==(frame<2 ? 0 : 1) &&
                      tcSecond==(frame<2 ? 59 : 0) &&
                      tcFrame==expectedFrame,
                      "Independent LTC decoding found wrong frame numbering");
                check(bits[10]==(rate==Rate::drop2997 ? 1 : 0),
                      "Drop-frame bit disagrees with selected LTC rate");
                constexpr char sync[]="0011111111111101";
                for(int i=0;i<16;++i)
                    check(bits[(size_t)(64+i)]==(unsigned char)(sync[i]-'0'),
                          "Independent LTC receiver lost SMPTE sync word");
                int zeros=0;
                for(auto bit:bits) zeros+=(bit==0);
                check((zeros&1)==0,"Independent LTC receiver found invalid phase parity");
            }
            file.deleteFile();
        }
    });
    test("Generated LTC is normal Audio on an isolated Direct Out pair",[] {
        const auto folder=juce::File::getSpecialLocation(
            juce::File::tempDirectory).getChildFile("TheCuelist-LtcAcceptance");
        const auto file=folder.getNonexistentChildFile("direct-output",".wav");
        Config cfg;cfg.rate=Rate::fps25;
        cfg.start={1,0,0,0};cfg.durationSeconds=.25;
        check(writeWav(file,cfg).wasOk(),"Cannot create LTC Direct Out fixture");
        std::shared_ptr<const AudioClip> clip;
        check(loadClip(file,clip).wasOk() && clip!=nullptr,
              "LTC Direct Out fixture is not playable Audio");
        auto show=makeShow();
        show.masterMuted=true; // Direct Out must not depend on Master state.
        show.columns[0].masterSend=false;
        show.columns[0].directOutputEnabled=true;
        show.columns[0].directPostFader=false;
        show.columns[0].output=2;
        show.columns[0].pan=0;
        show.columns[0].gain=1;
        show.rows[0].gain=1;
        show.rows[0].cells[0].gain=1;
        show.rows[0].cells[0].file=file.getFullPathName();
        ClipMap media; media[show.rows[0].cells[0].id]=clip;
        check(validateShow(show).wasOk(),"Dedicated LTC route invalid");
        for (double deviceRate:{48000.0,44100.0}) {
            auto state=makeRenderState(show,media);
            check(state->voices.size()==1 && state->voices[0].clip==clip,
                  "LTC source did not become a regular Audio voice");
            state->trigger(show.rows[0].cells[0].id,false);
            juce::AudioBuffer<float> out(4,2048);
            state->render(out.getArrayOfWritePointers(),4,2048,deviceRate);
            check(state->voices[0].playing,"Direct LTC transport stopped prematurely");
            for(int sample:{100,300,700,1500,2000}) {
                check(std::abs(out.getSample(0,sample))<1.0e-6f &&
                      std::abs(out.getSample(1,sample))<1.0e-6f,
                      "LTC leaked into Main output");
                check(std::abs(out.getSample(2,sample)-
                               out.getSample(3,sample))<1.0e-6f,
                      "LTC Direct Out channels do not share the mono waveform");
            }
            check(out.getMagnitude(2,2048)>.3f &&
                  out.getMagnitude(3,2048)>.3f,
                  "LTC audio disappeared on Direct Out at hardware sample rate");
            if (deviceRate==48000.0)
                for (int sample:{100,300,700,1500,2000})
                    check(std::abs(out.getSample(2,sample)-
                        clip->samples.getSample(0,sample))<1.0e-5f,
                        "48kHz Direct Out altered the LTC waveform");
        }
        file.deleteFile();
    });
}
