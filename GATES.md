# Gates: UX-03C — Non-modal column keyboard actions and PANIC safety

OWNS: GATES.md, src/MainComponent.cpp, src/MainComponent.h, src/CueOperations.cpp, src/CueViews.cpp, tests/UiRefinementTests.h, docs/complete-user-guide.md, CHANGELOG.md, CMakeLists.txt, VERSION.txt

Scope: Replace only the Shift+F10-created modal column popup in native JUCE with a keyboard-operable, non-modal header mode; preserve global Esc/PANIC and Space/GO.

These are **manual evidence gates** rather than executable Unlazy `CHECK:` commands because this connector can inspect GitHub CI but cannot execute arbitrary approved local Windows tests. CI results must be linked to a specific head SHA. No inherited checks or hooks may run.

- [ ] G1: Shift+F10 shows an explicit, visible native column action hint, does not create a PopupMenu or change keyboard focus, and a click on the hint cannot activate obscured column controls.
  EVIDENCE: pending

- [ ] G2: With column actions active, Esc still executes MainComponent::stopAll and Space still executes MainComponent::goNext; no Show/VIEW/LIVE mutation at mode entry.
  EVIDENCE: pending

- [ ] G3: The selected column is tracked by stable ID, and Delete requires two different keys (5 then D). Repeated 5, a missing target, or cancellation must not delete another column.
  EVIDENCE: pending

- [ ] G4: Rename, Move left/right and Collapse/Expand reuse existing functions; all previous mouse controls, keyboard shortcuts, font and 72px header size stay intact, including JSON v10 compatibility.
  EVIDENCE: pending

- [ ] G5: Windows Release and CTest (including added mode and regression tests) and version/changelog guards succeed for this exact PR head, with screenshot artifacts reviewed before merge.
  EVIDENCE: pending

- [ ] G6: Manual Windows 100/125/150% DPI, Narrator, real keyboard with menu open, and external Audio/MIDI/OSC/LTC bench validate PANIC/GO beyond the simulated tests.
  EVIDENCE: pending — release/live gate; do not claim LIVE readiness from CI

Rollback: revert the small PR; no schema migration, changes limited to native UI and regression tests.
