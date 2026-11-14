# RQ-UX04A — Esc/PANIC and native inline TextEditor focus

## UX-04C correction — 2026-11-14 / PR following #190

**Observed failure:** the merged PR #190 compiled on Windows but its CTest
failed with `420418 assertions, 1 failed test`:
https://github.com/Askeladd22/The_Cuelist/actions/runs/43575358390
(`UX04A inline name Esc cancelled editing but missed PANIC`).

**Source-level diagnosis:** JUCE `TextEditor::keyPressed(Escape)` invokes
`escapePressed()`, which calls `postCommandMessage` to enqueue an
asynchronous escape callback. The failing CTest checked `onEscapeKey` side
effects before any GUI message-loop dispatch. This is a **false-negative
test timing pattern**, not evidence that physical Windows Esc never calls
PANIC. JUCE source:
https://github.com/juce-framework/JUCE/blob/master/modules/juce_gui_basics/widgets/juce_TextEditor.cpp

**Corrected test scope:** call the configured `onEscapeKey` callback
directly for each of the four native inline editors to assert the
Cancel + `MainComponent::stopAll()` contract. This is a **unit-level
callback test**, NOT a real Escape key delivery test; it cannot measure
queuing delay or behavior in a focused modal popup. We do not enable
nested JUCE modal loops (`JUCE_MODAL_LOOPS_PERMITTED=0` in CMake).
Real Windows/HID delivery and external audio/MIDI/OSC/LTC stop time:
**NOT RUN**, follow-up issue #191. Existing PR #190 production handlers
remain unchanged; UX-04D adds only the native project-name Label Escape
forwarding described below.


## UX-04D: project-name native Label Escape gap (same draft PR #192)

**Verified against project main@55a75314c512840d3bedc64a92e4442b6d8bf67a
and pinned JUCE 8.0.12:**
- `MainComponent::projectNameLabel` was a standard `juce::Label` with
  `setEditable(false,true,false)`. Its internal editor is modal and
  handles Esc through Label's virtual `textEditorEscapeKeyPressed`.
- Exact [JUCE 8.0.12 Label implementation](https://github.com/juce-framework/JUCE/blob/8.0.12/modules/juce_gui_basics/widgets/juce_Label.cpp)
  resets the pending text and calls `hideEditor(true)`, **without
  forwarding Esc to MainComponent**. `onEditorHide` fires for Enter,
  Esc and focus loss, so attaching PANIC there would cause spurious
  emergency stops on ordinary project rename.
- `onTextChange` fires for committed text, not Escape cancellation.
  The `onEscapeKey` callback of the nested TextEditor is too late to
  rely upon because Label's listener can destroy that editor first.
- The safe intervention preserves the native JUCE Label's UI and
  existing commit logic: a small derived `PanicAwareProjectLabel`
  overrides the virtual TextEditor::Listener Escape method, calls
  the base cancellation, then invokes the existing parent
  `MainComponent::stopAll()` through a callback.
- Source-derived hypothesis: a single first physical Escape while
  this Label editor owns focus will now cancel the name and call PANIC;
  the test proves only the **actual Label listener callback contract**.
  A Windows OS-delivered key and audio/MIDI/OSC/LTC bench are **ND**.

**Red-team and test plan:** a native test opens the real Label editor at
1180/1920, enters a long unsaved name, seeds the engine PANIC indicator,
calls the listener callback and asserts cancellation, Show/NEXT/LIVE
stability, cleared indicator and no lingering editor; an ordinary
focus-loss confirm must commit the name without stopping the show.
The test does not replace an OS key event, Narrator/IME interaction,
DPI scaling screenshots or downstream hardware measurement.

**Compatibility:** double-click editing, project file path, schema JSON
v10, audio callback and all existing control/transport semantics remain
unchanged. Rollback: revert the UX-04D commit or PR; no migration.
The modal `PopupMenu` and other DialogWindow focus risks remain in
[issue #191](https://github.com/Askeladd22/The_Cuelist/issues/191).

## Scenario, operator and scope

The CUElist 0.9.67; baseline main@99b3f77ff3805c5f4580c576cf48b46fabe34aa2.
The operator is editing the name of a Cuelist/cue/column/cell, a pinned
cue BPM, a Timeline marker, or a Quick Learn function while a LIVE show
may be playing. Pressing Esc is documented as global PANIC, but these
four native child TextEditors consume Esc and only close their local edit.
Requirement: a single Esc cancels the pending value *and* invokes the
existing global stopAll() path once; Enter must keep its existing commit
behavior. The toolbar, focus hierarchy, persisted JSON v10, VIEW/LIVE
selection, timing, audio thread and MIDI/OSC output routing are out of scope.

## Primary sources and evidence levels

1. **Verified source baseline:** `src/MainComponent.cpp` assigns
   `inlineNameEditor.setEscapeAndReturnKeysConsumed(true)` and an
   `onEscapeKey` callback that only calls cancelInlineRename() and
   grabKeyboardFocus(). `src/CueViews.cpp` has the consumed
   `bpmEditor` which only calls finishBpmEdit(false);
   `src/Timeline.cpp` has the consumed `markerEditor` which only
   calls finishMarkerEdit(false). Red-team review also found a fourth
   native `QuickControlTargets::renameEditor` in
   `src/QuickControlTargets.h`: it inherits JUCE TextEditor's
   consumed-Escape default, and its callback originally only called
   finishRename(false). The owner MainComponent declares `stopAll()`;
   its implementation calls engine.panic(),
   clears control output and resets active mapping.
2. **JUCE official API manual (not an executed test):**
   https://docs.juce.com/master/classjuce_1_1TextEditor.html —
   `setEscapeAndReturnKeysConsumed` controls propagation to parent;
   `onEscapeKey` is a supported callback.
3. **JUCE official popup manual:**
   https://docs.juce.com/master/classjuce_1_1PopupMenu.html —
   `showMenuAsync` displays a separate menu with cancellation
   result 0. Its Esc path is not covered by this patch.
4. **Baseline CI evidence:** PR #189,
   https://github.com/Askeladd22/The_Cuelist/actions/runs/43572950871
   succeeded for CTest/Windows Release. This does NOT reproduce the
   inline Esc bug or certify real device stop latency.
5. **Operator judgement:** a cue editing cancellation must not
   mask a panic request. Operator timing and hardware effects are ND.

## Comparison of behavior (source-derived; not live measurement)

| Context | Baseline | Intended UX-04A | Still not covered |
|---|---|---|---|
| MainComponent, no child interception | Esc -> stopAll() | Unchanged | Real keyboard latency |
| Name inline TextEditor | Esc -> cancel only | Esc -> cancel then stopAll() | Windows IME tests |
| Pinned BPM inline TextEditor | Esc -> cancel only | Esc -> cancel then stopAll() | DPI and visual focus |
| Timeline marker name/BPM editor | Esc -> cancel only | Esc -> cancel then stopAll() | External LTC sync |
| Quick Learn function rename editor | Esc -> cancel only | Esc -> cancel and call owner-provided panic callback | Quick Learn state resets on global PANIC |
| PopupMenu/showMenuAsync or modal dialogs | Escape typically dismisses popup | Unchanged | Modal Esc/PANIC is a separate high-risk RQ |
| Notes editor | Esc propagation explicitly enabled | Unchanged | Native focus/HID test |

## Hypothesis and red team

H-UX04A: forwarding consumed Esc to `MainComponent::stopAll()` after
rolling back the edit preserves the existing edit cancellation and
causes PANIC on one keypress. This is falsified by any native test where
(a) an uncommitted name/BPM/marker/Quick Learn rename survives or (b) a seeded
`engine.acknowledgedCueId` remains nonzero after Esc. Test must verify
VIEW/LIVE and NEXT unaffected.

Counterexamples:
- Moving only `setEscapeAndReturnKeysConsumed(false)` might propagate
  Esc to a parent that isn't MainComponent (Timeline/Overlay) or cause
  duplicated cancellation; use explicit existing stopAll instead.
- Running `stopAll` BEFORE cancellation can trigger UI state changes
  while the TextEditor still owns focus; cancel first.
- Calling `onEscapeKey` in isolation does not prove a physical key
  event reaches the text field. PR #190 tried `TextEditor::keyPressed`
  with an immediate assertion and failed because JUCE dispatch is
  asynchronous. The UX-04C callback-contract test avoids the false
  failure and explicitly leaves physical delivery NOT RUN.
- Replacing all PopupMenus is a separate architectural change, and
  this PR does not solve their modal key interception. The high-risk
  follow-up is tracked as GitHub issue #191.
- QuickControlTargets is a reusable list independent of MainComponent;
  passing an owner-side panic callback avoids hard-coding the transport
  into the list. stopAll() also ends Quick Learn's transient state.
- No work occurs in the realtime audio callback. A reset synthetic
  acknowledgment is not a timing measurement for downstream MIDI/OSC/LTC.

## Test and acceptance plan

1. Native CTest callback-contract regression: initialize all four
   editors; edit an unsaved value; seed engine acknowledgedCueId; call
   each configured `TextEditor::onEscapeKey` callback directly. Assert
   editor hidden, encoded Show and selected NEXT/LIVE unchanged, ack
   reset. Existing Enter tests must remain green. This does **not**
   exercise JUCE's queued physical keyboard dispatch.
2. Keep root Esc/PANIC and non-modal Shift+F10 tests green.
3. Windows Release + full CTest and version/changelog guard must pass
   at the exact PR commit SHA.
4. Manual operator: during actual audio/MIDI/OSC/LTC output and
   keyboard focus in each editor, press Esc once. Measure callback/stop
   latency and observe audio and external device behavior; verify
   100/125/150 DPI, IME, Narrator.
5. Perform independent popup/dialog audit with a user-visible test
   plan before proposing a general modal-input architecture.

## Results, residual gates and recommendation

- Baseline code diagnosis: VERIFIED from fetched source; no real hardware
  reproduction.
- UX-04A production patch in PR #190: merged, Windows Release compilation
  passed but the original immediate-assert CTest FAILED on the async path.
  The UX-04C callback test fix requires its own Windows CI verification.
- Manual Windows/editor focus: ND.
- Audio/MIDI/OSC/LTC hardware latency, downstream timing: ND.
- Accessibility Narrator and native window scaling: ND.
- Recommendation: scoped explicit forwarding for 3 inline TextEditor
  callbacks plus the Quick Learn owner's panic callback, retaining the
  current root stopAll and no global keyboard
  hooks. No claim of ready-for-LIVE until hardware verification.
- Rollback: revert UX-04A PR; no project data migration or schema change.

## Capability declaration (Skillroster / CCteam)

Skill routes verified from ChatGPT's installed skill catalog: Taste +
UI/UX Pro Max, Matt Pocock code-review/diagnosing-bugs, Context
Engineering, GStack Diff/Engineering/QA, SkillRoster, CCteam and Unlazy.
These are sequential specialist review lenses in this chat; no
SkillRoster executable ran, no local Skills inventory was modified,
and no independent teammate processes were started.
