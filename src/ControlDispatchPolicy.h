#pragma once
#include "ShowModel.h"
#include <cstddef>
#include <optional>
#include <set>

namespace cuelist {

// UI-thread physical-edge state, kept separate from binding and preset IDs.
// Record *all* events including unmatched releases, so a cue change cannot
// interpret an already-held GO/PANIC button as a fresh press.
// A saturated OSC tracker treats NEW presses as already held (fail closed for
// GO/PANIC). MIDI state is isolated so network traffic cannot evict its edges.
class PhysicalControlEdges final {
public:
    bool recordAndWasDown(const ControlEvent& event);
    void clear() noexcept { heldMidi.clear(); heldOsc.clear(); }
private:
    static constexpr std::size_t maxHeldOsc = 4096;
    // Only physically pressed identities need storage: release erases a key.
    std::set<juce::String> heldMidi, heldOsc;
};

// Model-only dispatch decisions. These neither enqueue output nor modify the
// project, and do not depend on the currently selected Inspector cell.
std::optional<float> mappedPickupTarget(const Show&, const ControlBinding&,
                                       std::optional<int> faderCeilingDb={}) noexcept;
// Fit a mapped gain with a wider stored range across the *entire* MIDI
// travel, rather than saturating at 25% when a 0..4 mapping feeds a 0dB fader.
float mappedFaderGain(const MappedControl&,int faderCeilingDb) noexcept;
int mappedTargetColumn(const Show&, int stableColumnId, ColumnType type) noexcept;
bool shouldApplyMappedValue(float oldValue, float requestedValue) noexcept;
int mappedMidiCcValue(float scaled) noexcept;

} // namespace cuelist
