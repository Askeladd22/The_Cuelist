# GATES — UX-04C: correct false-negative async TextEditor Escape regression

BASE: main@55a75314c512840d3bedc64a92e4442b6d8bf67a,
VERSION.txt 0.9.68, JSON schema v10.
OWNS: GATES.md; tests/UiRefinementTests.h;
docs/research/RQ-UX04-ESC-PANIC-FOCUS.md; CHANGELOG.md;
VERSION.txt; CMakeLists.txt.
NO PRODUCTION FILES MODIFIED. No changes to transport, GO/STOP/PANIC,
Audio/MIDI/OSC/LTC, VIEW/LIVE, UI geometry or keyboard routing.

## Failed baseline and independent source
PR #190 merged despite failed Windows Release/CTest:
https://github.com/Askeladd22/The_Cuelist/actions/runs/43575358390
Job 132388287950: Release app compiled but CTest failed:
420418 assertions, 1 failed test (UX04A inline name Esc...).
This is an OBSERVED CI failure. Source diagnosis:
JUCE TextEditor::escapePressed() posts an asynchronous
TextEditorDefs::escapeKeyMessageId with postCommandMessage; its
onEscapeKey runs later when the message thread dispatches the message.
Original test invokes TextEditor::keyPressed(Escape) but checks immediately
before processing the queued callback. This is a SOURCE-DERIVED
test timing defect, not proof that physical Windows Esc fails to PANIC.
Official JUCE sources:
https://github.com/juce-framework/JUCE/blob/master/modules/juce_gui_basics/widgets/juce_TextEditor.cpp
https://docs.juce.com/master/classjuce_1_1Component.html

## Hypothesis and review roles
H-UX04C: test the actual configured onEscapeKey callbacks synchronously
rather than claiming to test message-loop delivery without pumping JUCE's
queue. Existing callbacks should cancel the four pending edits and invoke
MainComponent::stopAll() exactly once; seeded acknowledgedCueId becomes 0,
saved Show/VIEW/LIVE/NEXT and uncommitted values are unchanged. The test
is red-capable against missing callback-to-PANIC forwarding.
- Matt Pocock: debug assertion timing; standards/spec remain distinct.
- GStack: counterexample — physical OS Esc event and async delivery
  remain UNTESTED, cannot be claimed PASS.
- Taste/UI-UX: operator promise Cancel + PANIC remains unchanged.
- Unlazy: commit-specific CTest and gate ledger.
- Context Engineering: pin main and document known prior CI failed.
- CCteam: one reviewer/test lens, not spawned agents.
- SkillRoster: catalog only; no CLI, scan or local roster changes.

## Gates
- [x] A1 baseline: main SHA 55a75314; PR #190 merged; failed Windows job
  132388287950; 0.9.68; actual failure line verified.
- [x] A2 mechanism: inspected JUCE TextEditor::escapePressed(),
  Component::postCommandMessage async, and disabled modal loops in CMake.
- [ ] A3 narrow test: use configured onEscapeKey callbacks for four
  inline editors, label honestly "callback-contract test". Assert
  unsaved data discarded, Show and NEXT/LIVE unchanged, ack cleared.
  No new product implementation; no unpumped keyPressed call.
- [ ] A4 Windows Release and ALL CTest: PASS at exact branch head SHA;
  verify no previous failures, and artifact capture is not skipped.
- [ ] A5 Version/changelog guard: PASS at exact head SHA; VERSION.txt,
  CMake 0.9.69 and CHANGELOG all synchronized.
- [ ] A6 source and diff review: only test+doc+version changes; no
  audio realtime, model or input dispatch behavior modified.
- [ ] A7 human Windows physical keyboard Esc in all four editors,
  queued callback scheduling, 100/125/150% DPI, Narrator, active
  external Audio/MIDI/OSC/LTC: **NOT RUN** until bench evidence.
- [ ] A8 residual modal focus issue #191: unverified, must not
  be silently declared fixed.

## Red-team and rollback
Calling onEscapeKey directly verifies callback wiring and stopAll, not
the platform event path, queue latency or physical downstream stop.
Do not add a nested event loop to tests: JUCE_MODAL_LOOPS_PERMITTED=0
is an intentional product configuration, and re-entrant loops distort
lifetime/focus. A separate event-loop-driven integration test requires
a scoped design and real Windows/HID bench under issue #191.
Rollback: revert this PR (no migration); no auto merge.
No inherited CHECK command auto-run or local SkillRoster CLI execution.
