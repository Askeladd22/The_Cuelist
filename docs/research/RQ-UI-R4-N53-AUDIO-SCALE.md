# RQ-UI-R4 / N5.3 — Audio-column fader scale legibility

**Date:** 2026-11-14
**Baseline:** main@3fbd8592d0dd8e6257eb0b26ac95bb76e86d29fc;
VERSION.txt 0.9.73, proposed v0.9.74, .cuelist schema JSON v10.
**Issue:** https://github.com/Askeladd22/The_Cuelist/issues/200
**Scope:** paint-only native JUCE Audio column scale; no Master redesign.

## Scenario and evidence

The operator provided two tight real-application screenshots of the
Master gain/meter and an Audio column gain/meter, both at 0.0 dB. The
expanded images reveal faint tick labels (+6, 0, -6, -18, -36, -48)
and an unlabelled terminal bottom tick. No user-provided audio signal
or Windows DPI info was available.

Source comparison:
- `src/GainMeterFader.cpp` paints gain thumb, independent L/R dBFS
  bar/peak-hold, tick labels, gain value in one calibrated frame.
  With a bottom-up shortage, `−∞` can be dropped by a greedy
  top-to-bottom 13px label-density filter after another tick.
- `src/GainMeterFader.h` maps saved relative gain and detected dBFS
  to one Y coordinate; engine values and saved gain remain correct.
- `src/ColumnUtilities.cpp` instantiates the real fader for Audio
  strips. `src/MasterAudioLayout.h` and
  `docs/UI_V2_FEATURE_PARITY.md` expressly forbid modifying the
  Master Audio's appearance, dimensions and callbacks.
- `tests/GainMeterFaderTests.h` already checks dB/Y invertibility,
  silence, mouse/wheel, real stereo peaks and peak-hold; preserve it.

## Alternative solutions and decision

1. Increase font size in both Master and columns: rejected, no space
   in 92px fader labels and would violate Master visual parity.
2. Move the Master/column fader controls: rejected; pan/mute hitboxes
   and fixed Master geometry must remain original.
3. **Selected:** opt-in readable column scale. Priority `0 dB` and
   `−∞` always receive captions; +6 and intermediate negative
   values are chosen only when they fit without overlapping. Keep
   all tick marks and the exact gain/meter calibrated Y mapping.
   Modestly increase only the column's secondary-label contrast;
   Master continues to use the exact old path by default.

## Hypothesis and red team

H-N5.3: At logical fader heights 135, 148, 163, 195 and 260,
both +6/0 ceiling scales reserve `0` and `−∞`, all selected
labels stay >=13.5px apart, the +6 caption yields to zero if needed
and the L/R peak bars and real gain/callbacks are unchanged.
The default `GainMeterFader` (Master) snapshot remains pixel-identical
before and after toggling the optional column mode off.

Counterexamples: no audio input; peaks near clipping; digital silence;
expanded/collapsed columns; 0 dB ceiling; +6dB ceiling; fader with
negative gain and wheel precision; short 180px utility panel; dark
and Legacy themes; MIDI and OSC must not acquire a fake audio fader.

## Operator risk, tests and results

The most relevant operator error is to mistake the lower tick
for finite -48dB rather than the digital-silence endpoint, or to
confuse meter dBFS with gain dB. The tooltip already distinguishes
gain from L/R peak dBFS; a more extensive redesign would need
an approved change of Master parity and an operator test.

- New pure priority mask and JUCE rendering/callback tests:
  **added, execution ND until exact-SHA CTest**.
- Native workspace screenshots with Audio and Master at 1180x820,
  1647x895: **capture added, visual comparison pending**.
- Windows Release/CTest & version guard: **ND until CI**.
- Real Windows 100/125/150% DPI, Narrator, IME, hardware
  Audio/MIDI/OSC/LTC and operator error-rate measurement: **ND**.
- No proof of downstream MIDI/OSC timing and no LIVE certification.

## Recommendation and rollback

Deliver only a draft, small v0.9.74 PR; compare the actual native
JUCE before/after and do not merge if default Master changes.
Revert the PR to restore old column labels; no project migration.
The Esc/PANIC modal focus issue #191 remains independent.
