#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>

namespace cuelist::ltc {

// This is an AUDIO-source generator, deliberately independent of show-wide
// timecode, transport clock and any future timecode receiver.
enum class Rate { fps24, fps25, drop2997, fps30, nonDrop2997 };
struct Timecode {
    int hour=0, minute=0, second=0, frame=0;
};
struct Config {
    Rate rate=Rate::fps25;
    Timecode start;
    double durationSeconds=10.0;
};
inline int nominalFps(Rate rate) {
    return rate==Rate::fps24 ? 24 : rate==Rate::fps25 ? 25 : 30;
}
inline int numerator(Rate rate) {
    return (rate==Rate::drop2997 || rate==Rate::nonDrop2997)
        ? 30000 : nominalFps(rate);
}
inline int denominator(Rate rate) {
    return (rate==Rate::drop2997 || rate==Rate::nonDrop2997)
        ? 1001 : 1;
}
inline juce::Result parseTimecode(juce::String input,Rate rate,Timecode& out) {
    input=input.trim();
    if (input.length()!=11 || input[2]!=':' || input[5]!=':' ||
        (input[8]!=':' && input[8]!=';'))
        return juce::Result::fail("Start LTC must be HH:MM:SS:FF (or HH:MM:SS;FF at 29.97 DF).");
    for (int i:{0,3,6,9})
        if (!input.substring(i,i+2).containsOnly("0123456789"))
            return juce::Result::fail("Timecode fields must contain two decimal digits.");
    const int h=input.substring(0,2).getIntValue();
    const int m=input.substring(3,5).getIntValue();
    const int s=input.substring(6,8).getIntValue();
    const int f=input.substring(9,11).getIntValue();
    if (h>23 || m>59 || s>59 || f>=nominalFps(rate))
        return juce::Result::fail("Timecode must be within 00–23 h, 00–59 min/sec and the chosen frame rate.");
    if (rate==Rate::drop2997 && m%10!=0 && s==0 && f<2)
        return juce::Result::fail("29.97 drop-frame omits frame labels 00 and 01 at non-tenth minutes.");
    if (rate!=Rate::drop2997 && input[8]==';')
        return juce::Result::fail("Semicolon timecode notation is reserved for 29.97 drop-frame.");
    out={h,m,s,f};
    return juce::Result::ok();
}
inline juce::Result validate(const Config& cfg) {
    if (!std::isfinite(cfg.durationSeconds) ||
        cfg.durationSeconds<.1 || cfg.durationSeconds>600.0)
        return juce::Result::fail("LTC duration must be between 0.1 and 600 seconds (10 minutes per source).");
    const auto& t=cfg.start;
    if (t.hour<0 || t.hour>23 || t.minute<0 || t.minute>59 ||
        t.second<0 || t.second>59 || t.frame<0 || t.frame>=nominalFps(cfg.rate))
        return juce::Result::fail("Invalid LTC start timecode.");
    if (cfg.rate==Rate::drop2997 && t.minute%10!=0 &&
        t.second==0 && t.frame<2)
        return juce::Result::fail("Invalid skipped drop-frame label.");
    return juce::Result::ok();
}
inline void increment(Timecode& t,Rate rate) {
    if (++t.frame<nominalFps(rate)) return;
    t.frame=0;
    if (++t.second==60) {
        t.second=0;
        if (++t.minute==60) {
            t.minute=0;
            t.hour=(t.hour+1)%24;
        }
    }
    if (rate==Rate::drop2997 && t.second==0 && t.minute%10!=0)
        t.frame=2;
}
inline std::array<uint8_t,80> frameBits(Timecode t,Rate rate) {
    std::array<uint8_t,80> b{};
    const auto add=[&b](int first,int count,int value) {
        for (int k=0;k<count;++k)
            b[(size_t)(first+k)]=(uint8_t)((value>>k)&1);
    };
    add(0,4,t.frame%10); add(8,2,t.frame/10);
    b[10]=rate==Rate::drop2997 ? 1u : 0u;
    add(16,4,t.second%10); add(24,3,t.second/10);
    add(32,4,t.minute%10); add(40,3,t.minute/10);
    add(48,4,t.hour%10); add(56,2,t.hour/10);
    // SMPTE ST 12-1, 9.2: 16-bit sync word in transmission order.
    constexpr char sync[]="0011111111111101";
    for (int i=0;i<16;++i) b[(size_t)(64+i)]=sync[i]=='1' ? 1u : 0u;
    // The polarity correction location differs for 25fps (59) and
    // the 24/30 family (27). Keep an even number of zero bits per word.
    const int parityBit=rate==Rate::fps25 ? 59 : 27;
    int zeros=0;
    for (const auto bit:b) zeros+=bit==0 ? 1 : 0;
    if (zeros%2) b[(size_t)parityBit]=1;
    return b;
}

// Stream 48kHz mono 16-bit PCM WAV, so output is regular project Audio and
// remains available after relaunch. 29.97 is 30000/1001, not rounded 29.97.
inline juce::Result writeWav(const juce::File& destination,const Config& cfg) {
    if (const auto valid=validate(cfg);valid.failed()) return valid;
    if (destination.exists())
        return juce::Result::fail("LTC destination already exists. Nothing was overwritten.");
    if (const auto mkdir=destination.getParentDirectory().createDirectory();mkdir.failed())
        return mkdir;
    constexpr int sampleRate=48000,blockSize=4096;
    const int64_t totalSamples=(int64_t)std::llround(cfg.durationSeconds*sampleRate);
    if (totalSamples<=0)
        return juce::Result::fail("LTC duration is too short.");
    auto stream=destination.createOutputStream();
    if (!stream) return juce::Result::fail("Cannot create the generated LTC WAV.");
    std::unique_ptr<juce::OutputStream> outputStream(std::move(stream));
    juce::WavAudioFormat format;
    auto writer=format.createWriterFor(outputStream,
        juce::AudioFormatWriterOptions()
            .withSampleRate(sampleRate).withNumChannels(1).withBitsPerSample(16));
    if (!writer) {
        destination.deleteFile();
        return juce::Result::fail("Cannot initialise PCM WAV output.");
    }
    juce::AudioBuffer<float> block(1,blockSize);
    int filled=0;
    int64_t emitted=0,frameIndex=0;
    float polarity=1.f;
    Timecode code=cfg.start;
    bool ok=true;
    while (emitted<totalSamples && ok) {
        const auto bits=frameBits(code,cfg.rate);
        std::array<float,160> levels{};
        for (int i=0;i<80;++i) {
            polarity=-polarity;  // mandatory transition at bit boundary
            levels[(size_t)(i*2)]=polarity;
            if (bits[(size_t)i]) polarity=-polarity; // one: midpoint edge
            levels[(size_t)(i*2+1)]=polarity;
        }
        const int64_t nextFrameSample=
            ((frameIndex+1)*sampleRate*denominator(cfg.rate) +
             numerator(cfg.rate)/2)/numerator(cfg.rate);
        const int64_t firstSample=
            (frameIndex*sampleRate*denominator(cfg.rate) +
             numerator(cfg.rate)/2)/numerator(cfg.rate);
        const int frameSamples=(int)(nextFrameSample-firstSample);
        for (int n=0;n<frameSamples && emitted<totalSamples;++n,++emitted) {
            const int half=(int)((int64_t)n*160/frameSamples);
            block.setSample(0,filled++,levels[(size_t)std::min(159,half)]*.65f);
            if (filled==blockSize) {
                ok=writer->writeFromAudioSampleBuffer(block,0,filled);
                filled=0;
            }
        }
        ++frameIndex;
        increment(code,cfg.rate);
    }
    if (ok && filled>0) ok=writer->writeFromAudioSampleBuffer(block,0,filled);
    writer.reset(); // finalise the WAV header before subsequent loadClip
    if (!ok) {
        destination.deleteFile();
        return juce::Result::fail("Writing LTC audio failed; incomplete file removed.");
    }
    return juce::Result::ok();
}
} // namespace cuelist::ltc
