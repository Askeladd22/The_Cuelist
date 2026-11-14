# GATES — UX-03E: protect native column-action overlay from multi-click leakage

BASE: main@36b6cec743fe49697a345369f51bfd2767440ed7; VERSION.txt=0.9.65.
SCOPE: Only mouse gesture handling in PinnedColumnHeaders (native JUCE);
  tests and operator guide. No audio, MIDI, OSC, LTC, timeline, grid model,
  VIEW/LIVE, JSON v10 or transport changes.
OWNS: GATES.md, src/MainComponent.h, src/CueViews.cpp,
  tests/UiRefinementTests.h, docs/complete-user-guide.md,
  VERSION.txt, CMakeLists.txt, CHANGELOG.md.

## Context and prior gate state
PR #186 merged; Windows Release/CTest success on head
0341baaf40fa883f1ecb4b59cc745a4330ae2ae1:
https://github.com/Askeladd22/The_Cuelist/actions/runs/43570017283
Native screenshot artifact uploaded (does NOT itself constitute human review).
Manual Windows DPI/Narrator/hardware and the broader ESC/PANIC popup audit
remain ND and are NOT silently considered completed.

## Measurable hypothesis and negative control
H-UX03E: A mouse press used to dismiss the Shift+F10 non-modal column
overlay must consume the rest of that same native JUCE multi-click sequence,
including mouseDoubleClick; a fresh independent click sequence must retain
existing double-click rename. The current main has a mouseDown guard but
mouseDoubleClick dispatches unconditionally to grid.doubleClickColumnHeader().
This is a source-derived risk, not an observed hardware reproduction.
JUCE MouseEvent::getNumberOfClicks() identifies a multi-click sequence.

## Gates — evidence required
- [x] E1 BASELINE: main SHA, version and preceding Windows CTest verified.
  EVIDENCE: GitHub main ref, VERSION.txt, #186 workflow 43570017283.
- [x] E2 CODE DIAGNOSIS: The two native handlers use separate paths, with
  no overlay guard in mouseDoubleClick on the verified baseline.
  EVIDENCE: src/CueViews.cpp::PinnedColumnHeaders::mouseDown/mouseDoubleClick;
  src/CueViews.cpp::CueGrid::doubleClickColumnHeader (inline rename).
- [ ] E3 NATIVE TEST: Simulated overlay + first click + second mouseDown
  (clicks=2) + mouseDoubleClick must not invoke rename or alter Show,
  selectedColumn, nextCue or LIVE. Direct double-click over overlay also
  consumed; fresh ordinary double-click still opens original inline editor.
  EVIDENCE: pending full Windows CTest on new PR SHA.
- [ ] E4 IMPLEMENTATION: No duplicate native controls or focus hooks;
  consume only the overlay-dismissal multi-click sequence and reset on
  fresh click count=1. Normal icon actions, collapse, drag, wheel and
  keyboard transport unchanged.
  EVIDENCE: pending code review and regression results.
- [ ] E5 WINDOWS CI: version/changelog guard, Release and complete CTest
  are green on PR head; capture actual generated screenshot artifact.
  EVIDENCE: pending head-specific GitHub Actions links.
- [ ] E6 HUMAN/LIVE: inspect native compact/wide screenshots; Windows
  real mouse double-click, keyboard layout, DPI 100/125/150, Narrator and
  hardware Audio/MIDI/OSC/LTC with show active.
  EVIDENCE: ND until measured; CI green != ready for LIVE.

## Red team
Double-clicks recognized by the native platform may be delivered in
multiple callbacks. Tests simulate events; real Windows timing/order must
be independently checked. Do not block later, unrelated double-click
rename; do not allow a hidden Delete/Collapse under an overlay. Other
JUCE PopupMenu Escape/PANIC pathways require a separate audit.

Merge policy: small draft PR; never auto merge. Rollback: revert this PR,
no project-schema migration. No inherited shell CHECK commands are approved.
