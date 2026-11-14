# GATES — UX-04A: Esc/PANIC while editing native inline fields

BASE: `main@99b3f77ff3805c5f4580c576cf48b46fabe34aa2`,
VERSION.txt `0.9.67`, JSON file schema v10.
SCOPE: Four existing native TextEditor inline flows; no new controls,
modal workarounds, changes to root keyboard handler or audio thread.
OWNS: `GATES.md`, `src/MainComponent.cpp`,
`src/CueOperations.cpp`, `src/CueViews.cpp`, `src/Timeline.cpp`,
`src/QuickControlTargets.h`,
`tests/UiRefinementTests.h`, `docs/complete-user-guide.md`,
`docs/research/RQ-UX04-ESC-PANIC-FOCUS.md`,
`VERSION.txt`, `CMakeLists.txt`, `CHANGELOG.md`.

## Roles and reasoning budget

- Taste/UI-UX: Esc/PANIC must be predictable in every editing state,
  without throwing away editing Cancel or shifting cue selection.
- Matt Pocock: spec vs standards review, explicit behavior-contract regression.
- GStack: high-severity show-safety red-team and failure-mode analysis.
- Context Engineering: pinned SHA/version/schema/VIEW-LIVE invariants.
- CCteam: researcher -> native implementation -> reviewer -> QA -> custodian
  are review lenses, NOT independent spawned agents.
- SkillRoster: verified installed-skill catalog for routing; no local
  SkillRoster CLI run or roster mutation.
- Unlazy: all proof must come from commit-matched CI/test evidence; no
  inherited shell CHECK may run without explicit approval.

## Problem and falsifiable hypothesis

The parent MainComponent::keyPressed(Escape) calls stopAll(), which invokes
engine.panic() and clears MIDI/OSC output. Three explicit inline
TextEditors setEscapeAndReturnKeysConsumed(true) and only cancel on Esc;
a fourth Quick Learn editor inherits JUCE TextEditor's consumed-Escape
default and also cancels without panic:
- MainComponent::inlineNameEditor for Cuelist/cue/column/cell names;
- CueMetricsOverlay::bpmEditor;
- Timeline::markerEditor for name, BPM and conversions;
- QuickControlTargets::renameEditor for Quick Learn function names.
The fourth path was surfaced by the GStack counterexample audit.
According to official JUCE TextEditor documentation, consumed Escape
does not propagate to parent. These specific paths are source-verified;
physical Windows playback reproduction is ND.

H-UX04A: calling the same MainComponent::stopAll() *after* cancelling the
four edits sends one PANIC without committing any pending name/tempo
changes. A regression test must assert editor closed, unsaved edits
discarded, unchanged saved Show and LIVE, and acknowledgedCueId cleared.
Contrast against normal Enter still committing the same editors.

## Gates

- [x] G1 source+spec: confirmed 4 consumed inline-edit paths, root stopAll,
  test fixtures and official JUCE TextEditor reference;
  https://docs.juce.com/master/classjuce_1_1TextEditor.html
- [ ] G2 regression: test actual TextEditor::keyPressed(Escape) for
  name/BPM/timeline marker/Quick Learn, assert no pending changes, ack cleared,
  stable LIVE and model; Enter remains unaffected. Tests would
  fail on old main because ack stays nonzero.
  CHECK: GitHub Windows Release CTest for PR head (no local check approved).
  EXPECT: CuelistTests PASS and zero failed tests.
- [ ] G3 implementation: three existing handlers forward stopAll;
  QuickControlTargets forwards via an explicit owner callback; all after
  cancel, no duplicate PANIC and no audio-thread changes;
  tooltip/user guide accurately documents Esc cancel+PANIC.
  EVIDENCE: reviewed diff and CTest.
- [ ] G4 code review: standards/spec and GStack red-team, no
  inadvertent selection/VIEW/LIVE/Undo/Redo/JSON v10 changes.
  EVIDENCE: review of exact PR patch and code.
- [ ] G5 CI: Windows Release, complete CTest, version/changelog
  guard green on exact head SHA.
  EVIDENCE: pending GitHub Actions.
- [ ] G6 manual: real Windows focus/IME, Narrator, active
  audio/MIDI/OSC/LTC, Esc during playback, DPI 100/125/150,
  measure actual audio panic latency.
  EVIDENCE: ND; CI green != ready-for-LIVE.
- [ ] G7 related-modal audit: document remaining PopupMenu/LaunchOptions
  paths and do NOT claim Esc/PANIC fixed in native modal windows.
  EVIDENCE: docs/research RQ and issue #191 (modal focus audit).

## Stop conditions, counterexamples and rollback

Do not implement a system-global key hook, modify JUCE itself,
replace modal menus, or invent timing guarantees. If inherited
editing contracts or CI break, leave PR draft and inspect logs.
The tests use a synthetic engine acknowledgement as a narrow indicator,
not a measured external panic. Fixing inline text editors does not
fix PopupMenu's modal MenuWindow keyboard interception.
Rollback: revert PR, no project schema migration or engine state changes.
No skill/plugin roster was mutated; no independent CCTeam agents ran.
