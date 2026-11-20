# RQ-UI-007 — Master 0.0 dB native state evidence matrix

**Baseline:** main@60b73dc128f602caf794f9f5b1c37a85409f00fa, 0.9.86; **proposal:** 0.9.87 on a separate small draft PR, unchanged JSON schema v10.

## Operator scenario and sources
The user's fourth photo shows a mysterious dark rounded overlay behind/near the Master 0.0 dB readout. `src/GainMeterFader.cpp` paints gain on the same instrument as calibrated L/R peak bars; `src/MasterAudioLayout.h` and `src/ColumnUtilities.cpp` pin the fader. `src/QuickLearnHotspot.h` overlays the full fader *only while Quick Learn is active*. No source code inspected so far explains a *small* rounded element in exactly that user photo.

The user supplied no full-frame metadata or 100/125/150% DPI measurements; do not identify the element without a reproduction. Previous merged [PR #216](https://github.com/Askeladd22/The_Cuelist/pull/216) fixed menu glyph labels with actual RED→GREEN Windows CI but the new Master opaque paint had **no visible difference in the ordinary 1180×820 native CI snapshot**.

**Measured pixel comparison:** v0.9.85 Windows [run 44729568289](https://github.com/Askeladd22/The_Cuelist/actions/runs/44729568289) versus v0.9.86 [run 44734835024](https://github.com/Askeladd22/The_Cuelist/actions/runs/44734835024), `interface-n54-annotated-gain-1180.png`, RGB Master region [182,367,500,574]: **0 changed pixels**. The overall screenshots differ elsewhere, so this is not a file-reuse assumption. Visual artifact bytes were inspected with Pillow, not merely the upload status.

## PAUL PLAN, SEED hypothesis, red-team
**H1:** if an internal JUCE state overlay is responsible, screenshots with the real Master child hierarchy in inactive, requested keyboard focus, MIDI Learn and OSC Learn will show a reproducible difference at 1180×820 or 1647×895, in at least one theme. A stable screenshot fixture can then isolate the rendering/hitbox cause. This is **instrumentation**, not a claim of a failing production test or a confirmed root cause.

**H2:** in idle, no Master sibling Pan/Mute/Main Output/Limiter should geometrically intersect the lower 25px readout region of the Master gain fader. The Quick Learn hotspot deliberately overlays the fader *only while learning*. Changing hit targets before proof could break safe mapping and is out of scope.

**Counterexamples:** font fallback/OS IME, desktop tooltip or another application window captured over the Master; a different local build than main; Windows display scaling versus JUCE offscreen raster; runtime animation and popup windows not included in `createComponentSnapshot`. An offscreen focus request is not a validated physical keyboard focus or DPI test.

## APPLY scope and test plan
Add a small real-JUCE, nonmodal fixture to the existing `tests/Tests.cpp` visual QA section:
- Two logical window sizes 1180×820, 1647×895
- Two palettes Industrial Orange, Legacy
- Four modes idle, requested-focus, MIDI Learn, OSC Learn
- 16 deterministically named PNG screenshots uploaded by existing Windows GitHub Actions artifact step, without HTML mockups or a second screenshot subsystem.
- Assert the real Master `GainMeterFader` and `QuickLearnHotspot` exist with proper bounds; no other native Master control overlaps the readout; quick-learning state changes do not affect gain or LIVE armed cue; restored palette.
- No change to menu handlers, graphics code, native input devices, audio callback, MIDI/OSC route or .cuelist JSON.

## QA ledger, limitations and rollback
| Gate | Method | Status |
| --- | --- | --- |
| RED for screenshot anomaly | Actual user state not reproducible from existing idle fixture; no speculative failing assertion | ND |
| Geometry and state fixtures | Native JUCE Windows Release/CTest on PR head | NOT RUN |
| Native artifact images | Upload and inspect 16 PNG states; determine whether dark overlay reproduces | NOT RUN |
| CodeQL and version guard | Exact SHA on PR | NOT RUN |
| Real desktop at 100/125/150% DPI | Windows operator manual testing | ND |
| Real hardware audio/MIDI/OSC/LTC and GO/PANIC while mapping | Separate physical bench | ND |

**Trail of Bits manual C++ review:** ensure lifetime-safe child pointers, no RTC allocation changes, no cast/overflow or signal path changes. This is *not* the upstream scanner. **Matt Pocock:** feedback loop where failure is observable; the current state is hypothesis/instrumentation, not fabricated RED. **Simplify:** do not create a second renderer or speculative graphics UI. **CCteam QA:** inspect actual artifact screenshots, distinguish simulated and physical. **Unlazy:** complete test/screenshot ledger, mark remaining gates ND.

**Operator risk:** ambiguous gain feedback can lead to level setting errors, distinct from a peak meter or loudspeaker mute. No LIVE certification. **Rollback:** revert single PR; no project migration. **Issue:** https://github.com/Askeladd22/The_Cuelist/issues/217. **No automatic merge.**
