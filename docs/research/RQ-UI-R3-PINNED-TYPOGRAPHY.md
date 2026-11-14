# RQ-UI-R3 — Pinned Cue Grid and BPM/Auto Stop typography

**Scenario:** during live rehearsal, an operator must identify the LIVE Cuelist
and next global GO even when viewing a different Cuelist, and read BPM
and Auto Stop values at a glance. User provided three native screenshots
of Master Audio, BPM/Auto Stop and Cue Grid as evidence of undersized text.

**Baseline:** main@2f028502559da351dba665af7ca6f1f2fa75d7e0,
VERSION.txt 0.9.72, 2026-11-14; proposed 0.9.73.
**Issue:** https://github.com/Askeladd22/The_Cuelist/issues/198

## Sources examined and design comparison
- `src/Theme.h`: `font(size,bold)` uses embedded Exo 2 for
  nominal sizes <=14.5, Hind above that; point height is
  `max(10,size*0.75)`. Thus nominal sizes 10–13 are identical
  in height (10 pt). No font files are added or redistributed.
- `src/CueViews.cpp`: pinned Cue header uses LIVE font 11,
  LIVE name font 12, GO font 12; BPM and AUTO STOP headings font 13.
  `label()` uses JUCE drawText with an ellipsis when text exceeds
  its existing native rectangle.
- `src/CueGridGeometry.h`: 318px name lane, 72px heading, 62px
  rows and 184px fixed metric strip.
- `src/MasterAudioLayout.h`, `src/ColumnUtilities.cpp` and
  `docs/UI_V2_FEATURE_PARITY.md`: Master geometry and native controls
  are explicitly protected. The user crop shows MAIN OUT/LIMITER
  left-aligned and PAN/MUTE centred over their own controls;
  this is sensible, not a reason to justify all strings.
- `docs/UI_V2_VISUAL_DESIGN_PRINCIPLES.md`: semantic emphasis
  rather than decorative type; keep default font minimum 10pt.

## Hypothesis, counterexamples and risk
H-R3: increase these operator-status and metric labels from nominal
11–13 (clamped 10pt) to nominal 15 (Hind semibold 11.25pt),
without changing any rectangle or interaction. Actual JUCE glyph widths
of LIVE L12, GO 12, END, BPM, AUTO STOP must fit the available text
rectangles with >=4px spare. Preserve second-tier VIEW counts and
row status at current compact scale.

Full paragraph-style justification or blanket centring is rejected:
it creates irregular spacing in very short control labels and
makes cue titles harder to scan. Preserve left alignment for
variable names, centre BPM and the metric headings in their lanes,
right-align GO where originally defined.

Red team: long similar 64-character LIVE Cuelist names (ellipsis may
apply only to the variable name), LIVE hidden in tab overflow, END,
12 Cuelists, both colour themes, 1180x820 and 1647x895, extreme width,
actual Windows high DPI, accessibility and focus. Enlarging text must
never clip the safety-critical GO/END indication.

Risk for operator: a cut-off GO target or wrong visual emphasis
could imply a false next cue while GO still controls the real LIVE
Cuelist. The fix therefore does not change input targeting or any
state, model, transport or audio callback.

## Checks and actual results
- Native text-metrics CTest in `tests/UiRefinementTests.h`:
  test size, font family, shaped width of fixed strings, geometry;
  execution **ND until exact-SHA Windows CI**.
- Native screenshot files `interface-r3-pinned-typography-1180.png`
  and `interface-r3-pinned-typography-1647.png` from the
  existing nine-Cuelist REAL JUCE fixture; manually inspect after
  artifact upload. This is not a physical desktop/DPI test.
- Windows Release/full CTest and version/changelog guard: **ND**.
- Windows mouse/keyboard and DPI 100/125/150%, IME/Narrator and
  external Audio/MIDI/OSC/LTC with timing measurement: **NOT RUN**.
- Operator A/B time to identify next LIVE GO and error rate:
  **NOT RUN**.
- Unresolved modal Escape/PANIC issue #191 remains separate.

## Recommendation and rollback
Small draft PR only; review the exact final screenshot and test
before approving. Do not merge automatically. Rollback by reverting
the isolated font-size changes (or entire PR); schema JSON v10 and
runtime control logic do not change.
