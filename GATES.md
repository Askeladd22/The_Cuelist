# GATES — N5.3 Audio column fader scale readability

Baseline main@3fbd8592d0dd8e6257eb0b26ac95bb76e86d29fc,
official VERSION.txt=0.9.73, JUCE native, .cuelist JSON schema v10.
Issue #200, draft branch fix/n53-audio-strip-scale-labels-v0974,
proposed release v0.9.74. No automatic merge.

## Evidence, scope and operator hypothesis

The two operator images show tiny +6/0/-6/-18/-36/-48 labels and no
visible -infinity at the bottom tick in both master and column strips.
GainMeterFader::paint() iterates tick labels from highest to lowest,
skipping any less than 13px from the previously painted label.
At certain compact heights this causes the last -60 (=digital silence)
text to be omitted after an intermediate label; the final tick
itself still renders. Current numeric labels are at muted alpha .66.
The gain cursor and actual L/R peak bars share a calibrated Y axis,
but represent different units (relative dB vs signal dBFS).
This is a visual affordance issue, not proof of wrong engine output.

**Canonical conflict:** docs/UI_V2_FEATURE_PARITY.md explicitly forbids
redesigning the original pinned Master Audio appearance/geometries.
Therefore implement a column-only, opt-in typography and label priority
on the existing native GainMeterFader; default remains exact legacy
appearance for Master. No geometry/hitbox, peak meter, mouse,
keyboard, wheel, audio-thread or serialized Show changes.

H-N5.3: at expanded audio-strip heights 135/148/163/195/260 logical
pixels, on +6 and 0 ceilings, the labels 0 dB and -infinity
are ALWAYS selected, all printed labels respect >=13px spacing,
and the optional top +6 is omitted when it conflicts with 0 dB.
Other labels are filled by priority; secondary text has stronger
contrast. The Master continues drawing identical pixels.

## Intermediate evidence and red-team correction

First candidate `9674b722`: Windows build/CTest
[run 43603342673](https://github.com/Askeladd22/The_Cuelist/actions/runs/43603342673)
passed (420872 assertions, 0 failures) and screenshots uploaded.
Baseline 0.9.73 vs this candidate at 1180: **Master 0 changed pixels,
Audio column 241 changed pixels**. Human visual inspection found the
new terminal `−∞` caption near/overlapping the Audio MUTE heading.
A subsequent patch raises only that label 11px and adds a rectangle
collision test against MUTE and PAN. **Intermediate CI is not evidence
for the subsequent patch.** Re-run exact final head and inspect
before/after again before closing N5.3-5, N5.3-7.

## Gates
- [x] N5.3-1 verified main/SHA/version, read user screenshots (full
  and enlarged), GainMeterFader h/cpp, ColumnUtilities, Master layout,
  parity docs and existing tests; issue #200 created.
- [ ] N5.3-2 implement pure fixed-size label selection (no allocation),
  opt-in column-style while preserving default Master path exactly.
- [ ] N5.3-3 opt-in only for actual Audio ColumnUtilities::Strip gain,
  not masterSlider; midi/osc/control strips do not gain fake faders.
- [ ] N5.3-4 test priority at 135/148/163/195/260 heights and
  +6/0 ceiling; 0 and -infinity always, spacing >=13; unchanged
  input transform, gain/peak hold, callbacks, master default, no
  child widget. Native bitmap comparison of default mode before/after
  opt-in and restored default where possible.
- [ ] N5.3-5 real native JUCE screenshots at 1180x820 / 1647x895,
  inspect column gain and Master; preserve all controls/outputs.
- [ ] N5.3-6 VERSION.txt and CMakeLists.txt 0.9.74,
  CHANGELOG.md entry and guard SUCCESS for exact head.
- [ ] N5.3-7 Windows Release/full CTest 0 failures for exact
  latest SHA; diagnose new failures, no test weakening.
- [ ] N5.3-8 Matt Pocock standards/spec and GStack diff review:
  no MasterAudioLayout or ColumnUtilities::paint changes, no audio
  routing, PANIC, GO, LIVE/VIEW, CueGrid, format or engine changes.
- [ ] N5.3-9 physical Windows 100/125/150% DPI/IME/Narrator,
  external Audio/MIDI/OSC/LTC, timing, operator A/B gain adjustment
  tasks: ND / NOT RUN; no ready-for-LIVE claim.
- [ ] N5.3-10 unrelated modal Escape/PANIC issue #191 remains open.

## Red team
Worst case: h=135 short rail, +6 versus 0 text crowding, -48
versus -infinity at foot, tooltip still explains gain dB vs peak
dBFS, both ceiling settings, digital silence, negative dB, near-clip
peak values; on Legacy/Industrial Orange themes and collapsed
column with compact fader, no clipping. No passive input intercept or
style allocation in audio callback.

## Rollback
Revert small v0.9.74 PR; old default rendering is preserved and
there are no project migration steps or external device changes.
Do not execute inherited repo scripts or CHECKs without explicit
approval; use only observed GitHub Actions data as CI evidence.
