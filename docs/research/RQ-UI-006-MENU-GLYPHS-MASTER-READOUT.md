# RQ-UI-006 — Devices/View native menu glyphs and pinned Master gain caption

**2026-11-20. Baseline:** main@761cd34b2b19b84bb6de420a49631b2a69c47bd8, official 0.9.85, .cuelist JSON v10. **Target:** 0.9.86, small independent draft PR. Actual reference: four operator-provided screenshots of real JUCE UI.

## Scenario and evidence

- Photos 1/2: **Devices → MIDI Outputs / MIDI Inputs** popup sections show unreadable glyphs where the native `MIDI OUTPUT — ENABLE` and `MIDI INPUT — ENABLE` strings contain a Unicode em dash.
- Photo 3: **View → Appearance → Legacy** has the same glyph problem in `Legacy — Graphite / Copper`.
- Photo 4: the bottom left of **Master Audio** shows an overlapped/ambiguous `0.0 dB` gain caption on top of a rounded underlying element. The exact paint/layer cause cannot be conclusively determined from this tight crop. A dedicated opaque caption plate inside the real native `GainMeterFader` is the minimal testable remedy, with user review gate.
- Inspected source: `src/MainComponent.cpp::showOperatorMenu`, `src/Theme.h::Theme` PopupMenu drawing and sizing, `src/GainMeterFader.cpp::paint`, `src/MasterAudioLayout.h`, `src/ColumnUtilities.cpp`, `tests/UtilityPolishTests.h`, `tests/UiRefinementTests.h` and `scripts/test_native_operator_menu_parity.py`. Original base CMake already propagates MSVC `/utf-8`; switching globally from one source encoding is **not** an appropriate untested fix.
- Prior canonical research `docs/research/RQ-UI-R4-N53-AUDIO-SCALE.md` explicitly kept Master untouched for an *earlier* Audio-column-only request. Here the operator separately and explicitly shows a Master defect, so an isolated Master-only paint adjustment is now authorized.

## Hypothesis, counterexamples and options

**H1 (Menu):** replacing dash punctuation with ASCII in the three native labels removes dependency on unspecified font fallback while preserving the handlers and menu identities. Increasing popup text from the current 10pt embedded Exo 2 to at least 10.5pt, and updating measured row sizing consistently, improves legibility without changing any click targets.

**H2 (Master):** drawing a compact opaque neutral caption background within the existing 82px GainMeterFader Master lane will prevent underlying rail/stray art showing through at logical heights 135/163/220 and leave linear gain, L/R stereo peaks, -infinity, pan/limiter and MIDI/OSC transport unchanged.

**Red-team:** no inventing missing devices; long Windows MIDI device names, disabled input/output checkmarks, nested menu focus, accent legacy theme, clock, Fader Range mathematical glyphs (must continue using Noto Math), Master digital silence and 0.0dB readouts, native DPI 100/125/150%; narrow window with More menu and pointer. Physical live show hardware cannot be certified from screenshots.

**Chosen minimal:** ASCII label copy; native JUCE PopupMenu font and matching measurement, avoiding HTML mockups; Master only gain label plate with no new child or reparenting. **Rejected:** installing OS fonts, rewriting all Unicode support, changing MIDI I/O semantics, replacing JUCE popup menu, moving the physical Master channel and refactoring the audio callback. **Simplify:** retain existing renderer and native dispatch.

## PAUL PLAN/APPLY/UNIFY and CCteam QA gates

| Gate | Method | Result |
| --- | --- | --- |
| RED menu | Add assertion against real native MainComponent menu copy in existing Python parity script; code still old | NOT RUN |
| RED Master | Render actual JUCE GainMeterFader Master and check opaque dB readout region at 135/163/220 height | NOT RUN |
| RED typography | Native Theme popup-font minimum-height assertion | NOT RUN |
| GREEN | Windows Release/CTest and version/menu guard on final source SHA; CodeQL C++ on same SHA | NOT RUN |
| Test harness correction | First patch 74fe2ebd passed source menu parity and font but failed Master pixel test; pixel was read before the `Graphics` context was destroyed. Scoping the context before sampling is mandatory on Windows renderers. [Run 44732521036](https://github.com/Askeladd22/The_Cuelist/actions/runs/44732521036) | CORRECTED, rerun required |
| Source review | Manual Trail of Bits C++ safety audit (no input/routing code touched, graphics-only and text); identify nearby accidental complexity | PARTIAL |
| Native visual | Compare app Devices/MIDI submenu, View Appearance and Master at 100/125/150% Windows DPI; screenshot artefacts do not substitute visual inspection | ND |
| Hardware | External MIDI/OSC/audio/LTC, GO/STOP/PANIC while menus open | ND |

**Accept:** all 3 menu captions render without glyph replacement, native submenu items still select identical device IDs and options, current Master dB value is fully visible at normal/compact heights, no modal menu regression for keyboard control, version metadata aligned and full CI green. **No automatic merge.**

**Rollback:** revert this one PR; no show/project migration. Legacy Appearance palette settings continue to be kept in application preferences, not saved JSON. Note that improving the static Master caption surface is an informed minimal intervention; only real Windows DPI screenshots can establish that it fixes the exact overlapping pixels shown by the operator.

**Method:** PAUL Plan→Apply→Unify, SEED Application/Deep, Trail of Bits manual C++ review (not upstream scanner), Matt Pocock reproducible RED/GREEN, Simplify focused scope, CCteam QA native usability checks, Unlazy evidence ledger.
