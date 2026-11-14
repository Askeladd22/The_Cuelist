# RQ-UX04A — Esc/PANIC and native inline TextEditor focus

## Scenario, operator and scope

The CUElist 0.9.67; baseline main@99b3f77ff3805c5f4580c576cf48b46fabe34aa2.
The operator is editing the name of a Cuelist/cue/column/cell, a pinned
cue BPM, or a Timeline marker while a LIVE show may be playing.
Pressing Esc is documented as global PANIC, but these three child
TextEditors currently consume Esc and only close their local edit.
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
   calls finishMarkerEdit(false). Their shared owner MainComponent
   declares `stopAll()`; its implementation calls engine.panic(),
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
| PopupMenu/showMenuAsync or modal dialogs | Escape typically dismisses popup | Unchanged | Modal Esc/PANIC is a separate high-risk RQ |
| Notes editor | Esc propagation explicitly enabled | Unchanged | Native focus/HID test |

## Hypothesis and red team

H-UX04A: forwarding consumed Esc to `MainComponent::stopAll()` after
rolling back the edit preserves the existing edit cancellation and
causes PANIC on one keypress. This is falsified by any native test where
(a) an uncommitted name/BPM/marker change survives or (b) a seeded
`engine.acknowledgedCueId` remains nonzero after Esc. Test must verify
VIEW/LIVE and NEXT unaffected.

Counterexamples:
- Moving only `setEscapeAndReturnKeysConsumed(false)` might propagate
  Esc to a parent that isn't MainComponent (Timeline/Overlay) or cause
  duplicated cancellation; use explicit existing stopAll instead.
- Running `stopAll` BEFORE cancellation can trigger UI state changes
  while the TextEditor still owns focus; cancel first.
- Calling `onEscapeKey` in isolation does not prove a physical key
  event reaches the text field; tests also call TextEditor::keyPressed
  with Escape, but physical Windows/hardware still require a real bench.
- Replacing all PopupMenus is a separate architectural change, and
  this PR does not solve their modal key interception.
- No work occurs in the realtime audio callback. A reset synthetic
  acknowledgment is not a timing measurement for downstream MIDI/OSC/LTC.

## Test and acceptance plan

1. Native CTest regression: initialize each editor; edit an
   unsaved value; seed engine acknowledgedCueId; call TextEditor::keyPressed
   (Esc). Assert editor hidden, encoded Show and selected NEXT/LIVE
   unchanged, ack reset. Existing Enter tests must remain green.
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
- UX-04A patch and tests: see the PR diff; **new CI status must be checked**
  after creation. Do not mark passed in advance.
- Manual Windows/editor focus: ND.
- Audio/MIDI/OSC/LTC hardware latency, downstream timing: ND.
- Accessibility Narrator and native window scaling: ND.
- Recommendation: scoped explicit forwarding for the 3 inline TextEditor
  callbacks, retaining the current root stopAll and no global keyboard
  hooks. No claim of ready-for-LIVE until hardware verification.
- Rollback: revert UX-04A PR; no project data migration or schema change.

## Capability declaration (Skillroster / CCteam)

Skill routes verified from ChatGPT's installed skill catalog: Taste +
UI/UX Pro Max, Matt Pocock code-review/diagnosing-bugs, Context
Engineering, GStack Diff/Engineering/QA, SkillRoster, CCteam and Unlazy.
These are sequential specialist review lenses in this chat; no
SkillRoster executable ran, no local Skills inventory was modified,
and no independent teammate processes were started.
