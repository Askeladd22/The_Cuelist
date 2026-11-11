#pragma once
#include "ShowModel.h"
#include <memory>
#include <vector>

namespace cuelist {
// Beat positions use the exact TempoPoint mapping consumed by Timeline/MIDI/OSC.
// Events and waveforms are prepared before publication to the audio thread.
struct ClickEvent {
    double seconds=0;
    bool accent=false;
};
struct ClickPulseBank {
    static constexpr double sampleRate=48000.0;
    std::vector<float> normal, accented;
    double lengthSeconds() const noexcept {
        return normal.size()/sampleRate;
    }
};
std::vector<ClickEvent> makeClickEvents(const std::vector<TempoPoint>&,
                                       const ClickTrackConfig&);
std::shared_ptr<const ClickPulseBank> makeClickPulseBank(ClickSound);
}
