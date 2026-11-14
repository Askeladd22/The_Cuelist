# GATES — UX-04C/D: async Escape regression and native project Label

BASE: verified main@55a75314c512840d3bedc64a92e4442b6d8bf67a,
VERSION.txt 0.9.68; existing draft PR #192 is the 0.9.69 release slice.
PROJECT FORMAT: JSON schema v10; no migration.
OWNED: tests/UiRefinementTests.h, src/MainComponent.h,
src/MainComponent.cpp, GATES.md, CHANGELOG.md,
docs/research/RQ-UX04-ESC-PANIC-FOCUS.md,
docs/complete-user-guide.md, VERSION.txt, CMakeLists.txt.
NO audio-thread, transport, MIDI/OSC/LTC, Cue Grid, Control Map, Mixer,
Timeline playback, Undo/Redo or VIEW/LIVE model changes.

## Verified baseline and rationale

- PR #190 merged into main 0.9.68 but Windows CTest failed:
  https://github.com/Askeladd22/The_Cuelist/actions/runs/43575358390
  (420418 assertions, UX04A async Escape test false-negative).
- PR #192 at ae6c44842f7051d0f05e61d441827fe9a3a35fc8
  passed Windows Release + CTest: 420488 assertions, 0 failures,
  https://github.com/Askeladd22/The_Cuelist/actions/runs/43576532585
  This was a correct unit-level callback test, NOT an OS key delivery test.
  It precedes the new Label change and does not prove later commits.
- Exact pinned JUCE 8.0.12 TextEditor calls postCommandMessage for
  Escape and later handleCommandMessage dispatches callbacks:
  https://github.com/juce-framework/JUCE/blob/8.0.12/modules/juce_gui_basics/widgets/juce_TextEditor.cpp
- Exact pinned JUCE Label::textEditorEscapeKeyPressed resets/hides
  editor but does NOT call parent PANIC:
  https://github.com/juce-framework/JUCE/blob/8.0.12/modules/juce_gui_basics/widgets/juce_Label.cpp
- MainComponent::projectNameLabel.setEditable(false,true,false)
  owns a fifth native inline text-edit focus path that is not covered
  by PR #190's four TextEditor callbacks. This is a source-derived
  *operator risk*, not a physical Windows reproduction.

## Hypotheses, counters and operator risk

H-UX04C: direct callback-contract tests prove existing four JUCE
onEscapeKey handlers discard unsaved work and call stopAll; physical
message dispatch remains not measured.
H-UX04D: override only the project-name Label's TextEditor::Listener
Escape event, first perform JUCE's native discard/hide, then call
MainComponent::stopAll() through owner callback exactly once.
A red-capable regression should seed engine.acknowledgedCueId,
start actual Label::showEditor, provide unsaved name, invoke the
real overridden Label TextEditor listener callback, and assert the
pending edit is discarded, PANIC indicator reset, VIEW/LIVE/nextCue
unchanged, editor/modal state closed. Separately, Enter/focus-commit
must still rename the project with no PANIC.

- Red-team: Label::onEditorHide fires for Enter, Esc and focus loss;
  attaching PANIC to it is WRONG. Its onTextChange isn't called on
  cancelled text; attaching PANIC to it is WRONG. A TextEditor onEscapeKey
  assigned from onEditorShow may not run because Label's listener
  destroys the editor before invoking it. Override the listener method.
- Do not modify JUCE itself, hook Windows globally, add fake UI, or
  start nested modal loops (JUCE_MODAL_LOOPS_PERMITTED=0).
- The project label's visible name stays narrow (~132px at 1180):
  do not change geometry or font in this safety-focused slice.
  Long Unicode names and DPI remain accessibility bench targets.
- Event-first proof: internal ack zero is not external OSC/MIDI/LTC
  timing certification or physical keyboard transport latency.

## Gates and evidence
- [x] U1 baseline: exact main/PR version, failed #190 CTest and green
  #192 pre-change CI verified; pinned JUCE 8.0.12 source read.
- [x] U2 UI source audit: project Label edit/commit/escape flow read,
  operator Esc/PANIC contract and counters recorded.
- [ ] U3 test: native Label callback contract red-capable; verify
  cancel+PANIC, preserved Show and LIVE/NEXT, no modal/editor left;
  non-Escape commit still works without PANIC. Use callback method
  since synthetic keyPressed is asynchronous without event dispatcher.
- [ ] U4 implementation: only a native juce::Label subclass for
  projectNameLabel, callback to existing stopAll, no duplicate PANIC,
  no new UI, preserved tooltip/accessibility, rollback possible.
- [ ] U5 review: inspect precise patch; compare spec/standards (Matt
  Pocock), GStack red-team edge cases, Taste/Emil text/focus behavior;
  no cross-thread or JSON v10 change.
- [ ] U6 CI: Windows Release and CTest fully PASS for EXACT new PR
  head SHA; confirm UX04A and UX04D PASS, total assertions and
  screenshots, including whether artifact upload succeeded.
- [ ] U7 guard: VERSION.txt and CMake 0.9.69, changelog same,
  guard SUCCESS for exact new SHA.
- [ ] U8 REAL Windows physical keyboard Esc in project editor,
  OS focus handoff, Windows 100/125/150% DPI, Unicode and long
  strings, Narrator, IME, active Audio/MIDI/OSC/LTC bench: ND.
- [ ] U9 modal PopupMenu/DialogWindow Escape remains unresolved
  issue #191, not fixed by this PR. No LIVE certification.

## Rollback / safe integration

PR #192 remains draft. This amendment intentionally keeps one 0.9.69
PR rather than creating a conflicting second release PR while main
is at 0.9.68. Do not auto-merge. Rollback = revert PR if merged,
or drop only UX-04D commits if Label behavior fails review; no
persisted project schema change. No local SkillRoster or agent
processes; no inherited CHECK command or hook automatically executed.
