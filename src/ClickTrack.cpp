#include "ClickTrack.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace cuelist {
std::vector<ClickEvent> makeClickEvents(const std::vector<TempoPoint>& tempo,
                                       const ClickTrackConfig& config) {
    std::vector<ClickEvent> events;
    if (tempo.empty() || !std::isfinite(config.durationSeconds) ||
        config.durationSeconds<0.1 || config.durationSeconds>600 ||
        config.subdivision<1 || config.subdivision>4) return events;
    // Max 600 s at 999 BPM x four subdivisions: less than 40,000 pulses.
    events.reserve((size_t)std::min(40000.0,
        config.durationSeconds*999.0/60.0*config.subdivision+1.0));
    size_t segment=0;
    int previousBar=-1;
    int numerator=tempo.front().numerator,denominator=tempo.front().denominator;
    double barAnchor=0;
    for (int i=0;i<40000;++i) {
        const double beat=(double)i/config.subdivision;
        const double second=secondsAtBeat(tempo,beat);
        if (second>=config.durationSeconds-1.0e-10) break;
        while (segment+1<tempo.size() && tempo[segment+1].beat<=beat+1e-10)
            ++segment;
        const auto& point=tempo[segment];
        if (point.numerator!=numerator || point.denominator!=denominator) {
            // A meter change starts a new measure at its next integer beat.
            // A BPM-only marker must NOT reset an existing bar's downbeat.
            numerator=point.numerator;
            denominator=point.denominator;
            barAnchor=std::ceil(point.beat-1e-10);
            previousBar=-1;
        }
        const double barLength=numerator*4.0/denominator;
        const int bar=beat>=barAnchor-1e-10 && barLength>0
            ? (int)std::floor((beat-barAnchor+1e-10)/barLength) : -1;
        const bool downbeat=i==0 || (bar>=0 && bar!=previousBar);
        events.push_back({second,config.accent && downbeat});
        previousBar=bar;
    }
    return events;
}
namespace {
float deterministicNoise(unsigned sample) noexcept {
    uint32_t state=(uint32_t)sample*1664525u+1013904223u;
    state^=state>>15; state*=2246822519u;
    return ((float)(state & 65535u)/32767.5f)-1.0f;
}
float pulseAt(ClickSound sound,double time,unsigned index,bool accent) {
    constexpr double tau=6.283185307179586476925286766559;
    const double pitch=accent?1.18:1.0;
    const double noise=deterministicNoise(index);
    const double amplitude=accent?0.72:0.44;
    double waveform=0;
    switch(sound) {
        case ClickSound::classicClick:
            waveform=(0.72*noise+0.28*std::sin(tau*2400*pitch*time))
                *std::exp(-time*210.0);
            break;
        case ClickSound::woodblock:
            waveform=(0.72*std::sin(tau*690*pitch*time)+
                      0.28*std::sin(tau*1140*pitch*time))
                *std::exp(-time*82.0);
            break;
        case ClickSound::electronicBeep:
            waveform=(0.86*std::sin(tau*950*pitch*time)+
                      0.14*std::sin(tau*1900*pitch*time))
                *std::exp(-time*46.0) * std::min(1.0,time*1600.0);
            break;
        case ClickSound::rimshot:
            waveform=(0.62*noise+0.38*std::sin(tau*1750*pitch*time))
                *std::exp(-time*125.0);
            break;
    }
    return (float)(amplitude*waveform);
}
} // namespace
std::shared_ptr<const ClickPulseBank> makeClickPulseBank(ClickSound sound) {
    if ((int)sound<0 || (int)sound>3) return {};
    auto pulse=std::make_shared<ClickPulseBank>();
    constexpr int samples=2880; // 60 ms, all four tails end within this bound
    pulse->normal.resize(samples);
    pulse->accented.resize(samples);
    for(int i=0;i<samples;++i) {
        const double time=(double)i/ClickPulseBank::sampleRate;
        pulse->normal[(size_t)i]=pulseAt(sound,time,(unsigned)i,false);
        pulse->accented[(size_t)i]=pulseAt(sound,time,(unsigned)i,true);
    }
    return pulse;
}
// Stream a standard 48 kHz / 16-bit mono WAV, never synthesise click
// samples on the real-time callback for newly generated Audio cells.
juce::Result writeClickTrackWav(const juce::File& destination,
                                const std::vector<TempoPoint>& tempo,
                                const ClickTrackConfig& config) {
    if (tempo.empty() || !std::isfinite(config.durationSeconds) ||
        config.durationSeconds<0.1 || config.durationSeconds>600 ||
        config.subdivision<1 || config.subdivision>4 ||
        (int)config.sound<0 || (int)config.sound>3)
        return juce::Result::fail("Invalid Click Track configuration.");
    for (const auto& p:tempo)
        if (!std::isfinite(p.beat) || !std::isfinite(p.bpm) || p.bpm<=0 ||
            p.numerator<1 || p.denominator<1)
            return juce::Result::fail("Invalid Click Track tempo map.");
    const auto events=makeClickEvents(tempo,config);
    const auto pulses=makeClickPulseBank(config.sound);
    if (events.empty() || !pulses)
        return juce::Result::fail("Could not prepare Click Track audio.");
    for (const auto& event:events)
        if (!std::isfinite(event.seconds) || event.seconds<0 ||
            event.seconds>=config.durationSeconds)
            return juce::Result::fail("Invalid Click Track event time.");
    if (destination.exists())
        return juce::Result::fail("Click WAV already exists; existing audio was not overwritten.");
    if (const auto result=destination.getParentDirectory().createDirectory();result.failed())
        return result;
    auto stream=destination.createOutputStream();
    if (!stream) return juce::Result::fail("Cannot create the Click Track WAV.");
    juce::WavAudioFormat format;
    auto writer=format.createWriterFor(std::move(stream),
        juce::AudioFormatWriterOptions()
            .withSampleRate(ClickPulseBank::sampleRate)
            .withNumChannels(1).withBitsPerSample(16));
    if (!writer) {
        destination.deleteFile();
        return juce::Result::fail("Cannot initialise Click Track PCM WAV output.");
    }
    constexpr int blockSize=4096;
    const auto totalSamples=(int64_t)std::llround(config.durationSeconds*ClickPulseBank::sampleRate);
    juce::AudioBuffer<float> block(1,blockSize);
    size_t firstEvent=0;
    bool written=true;
    for (int64_t offset=0;offset<totalSamples && written;offset+=blockSize) {
        const int count=(int)std::min((int64_t)blockSize,totalSamples-offset);
        block.clear();
        while (firstEvent<events.size() &&
               (int64_t)std::llround(events[firstEvent].seconds*ClickPulseBank::sampleRate)
                   +(int64_t)pulses->normal.size()<=offset)
            ++firstEvent;
        for (size_t i=firstEvent;i<events.size();++i) {
            const auto start=(int64_t)std::llround(events[i].seconds*ClickPulseBank::sampleRate);
            if (start>=offset+count) break;
            const auto& pulse=events[i].accent ? pulses->accented : pulses->normal;
            const auto from=std::max(offset,start);
            const auto to=std::min(offset+count,start+(int64_t)pulse.size());
            for (int64_t frame=from;frame<to;++frame) {
                const int at=(int)(frame-offset);
                block.setSample(0,at,block.getSample(0,at)+pulse[(size_t)(frame-start)]);
            }
        }
        for (int i=0;i<count;++i)
            block.setSample(0,i,juce::jlimit(-1.0f,1.0f,block.getSample(0,i)));
        written=writer->writeFromAudioSampleBuffer(block,0,count);
    }
    writer.reset(); // Finalise the WAV header before loadClip tries to read it.
    if (!written) {
        destination.deleteFile();
        return juce::Result::fail("Writing Click Track WAV failed; incomplete file removed.");
    }
    return juce::Result::ok();
}
} // namespace cuelist
