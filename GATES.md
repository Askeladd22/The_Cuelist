# Gates: UX-03D — Correct collapsed-column keyboard test

BASE: `main@0b7a045d4c4eff9e3f045ba2ba81c3b1f8f09b9d`, VERSION.txt=0.9.64.
SCOPE: Repair the UX-03C test contract and clarify the operator guide.
OWNS: tests/UiRefinementTests.h, docs/complete-user-guide.md,
      GATES.md, VERSION.txt, CMakeLists.txt, CHANGELOG.md.
No changes to application source, UI, transport or JSON schema v10.

## Incident evidence (carried forward from merged PR #185)
- Windows CI: https://github.com/Askeladd22/The_Cuelist/actions/runs/43526445383
- Windows Release compilation passed; CTest FAILED (420393 assertions,
  1 failed test: UX03C column keys preserve identity and require D confirmation).
- Screenshot artifact upload skipped after CTest failure.
- The prior UX-03C GATES.md listed G1-G6 as pending even though #185 merged.
  Do not infer success from that merge; the independent LIVE gate remains open.

## Acceptance gates (manual evidence ledger)

This GitHub connector can inspect code and CI but cannot execute local
Windows/CTest commands. No inherited CHECK is approved.

- [x] G1 — Root cause is supported by the checked source and CI log.
  Collapsing the selected column calls toggleColumnCollapsed(), which can move
  selectedColumn to the first expanded column. toggleSelectedColumnKeyboardActions()
  captures the currently selected column ID for each new invocation.
  EVIDENCE: src/CueOperations.cpp, tests/UiRefinementTests.h,
  workflow job 132233272048.

- [ ] G2 — The regression test asserts selection reassignment, checks the
  currently targeted column label, explicitly reselects the collapsed MIDI
  column, then verifies 5 -> D removes only its stable ID and preserves LIVE.
  EVIDENCE: updated source reviewed; CTest execution on new SHA pending.

- [ ] G3 — Windows Release + complete CTest + version/changelog guard pass at
  the PR head SHA, including earlier Esc PANIC, Space GO, repeated 5 and stale
  target regressions. The previous #185 CTest was RED.
  EVIDENCE: pending new GitHub Actions run.

- [ ] G4 — Review genuine JUCE screenshots (1180x820 and 1920x1080 action
  strip and armed Delete) for truncation, contrast and false affordances.
  EVIDENCE: pending artifact; #185 screenshot upload was skipped.

- [ ] G5 — On real Windows, verify 100/125/150% DPI, actual keyboard / IME,
  Narrator accessibility, GO/PANIC during active playback and external
  Audio/MIDI/OSC/LTC hardware. Simulated CI is not readiness-for-LIVE.
  EVIDENCE: ND, requires operator/hardware verification.

## Red team and release boundary

The fix changes only a false assumption in a test. It does NOT guarantee
that other pre-existing JUCE popups allow Esc/PANIC; audit separately.
If CI remains red, record the exact failing assertion and do not merge.
Release/live readiness remains blocked by G4/G5 until independently checked.

Rollback: revert UX-03D PR; no schema migration or native UX changes.
