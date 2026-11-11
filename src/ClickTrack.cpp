#include "ClickTrack.h"
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
} // namespace cuelist
