# RQ-UI-R5 — Cuelist title, LIVE/VIEW status and return navigation

**Baseline:** verified main@084a0c5af2185e3d9589525857e40f0c41ee7d0a, VERSION.txt 0.9.75 (2026-11-14), .cuelist JSON v10.
**Design source:** operator-annotated native screenshot image(2).png, 2048x1365, issue #204.
**Change:** proposed v0.9.76 on small draft PR.

## Scenario and observed defects

At a show with L01 owning LIVE and L09 under edit, the old
318x72 pinned title listed VIEW's small Cuelist name above a
second duplicate LIVE locator/name/GO. The sidebar repeated
VIEW/BROWSING even though tabs and Cue Grid showed the viewed
Cuelist. White arrows on the supplied screenshot request a larger
Cuelist name and a return-to-LIVE button *before the tabs*.
Red strike-through marks duplicate strings; the red LIVE box
beside the title is a required new state, not a deletion.

Actual model references: MainComponent::liveCuelist and
nextLiveGoCueForUi belong to transport; show.activeCuelist is the
browsed Cuelist. Existing CueOperations::activateCuelist(index)
is the tested VIEW-only navigation method; it can commit an active
Inspector/inline edit but does not GO/STOP/PANIC or transfer LIVE.

## Comparison, hypothesis and alternatives

Baseline pinned header: small name (nominal 18), duplicative
LIVE/GO second row, VIEW counts bottom. Baseline sidebar:
114px three-level LIVE/NEXT/VIEW card. User-approved R5:
nominal 22 name, red LIVE if VIEW=LIVE or orange VIEW when browsing,
badge immediately beside name with ellipsis on very long names.
Second row is an Lxx locator and VIEW cue/clip counts, with
reorder arrows in otherwise freed space; original Collapse All stays.
Sidebar card becomes 80px keeping LIVE name + NEXT GLOBAL
and frees 34px for Recent Projects. Native GO/PANIC remain fixed.

Alternative rejected: clicking LIVE tag to force ownership.
That would conflate navigation with transport (unsafe).
Alternative rejected: modal popup instead of one-click return,
because the currently LIVE tab can be hidden behind overflow and
the user explicitly asked for one action. Alternative rejected:
remove LIVE locator from sidebar; it is still needed to identify
the owner of the upcoming global GO when VIEW differs.

H-R5A: with hidden LIVE L01 and VIEW L09 at 1180x820, orange VIEW
appears beside the L09 title; click the new return icon and
the Cuelist title changes to red LIVE L01 while engine LIVE index,
armed live next cue and acknowledged cue ID remain unchanged.
Second press is disabled. After a Row GO explicitly transfers
LIVE to L02, the return icon must follow L02, not cached L01.

H-R5B: 64-char Cuelist names must ellipsize before a fully visible
LIVE/VIEW badge and never overlap reorder or Collapse All buttons;
both original cue-row height (62) and pinned header (72) are
unchanged. At 1920x1080 nine tabs remain inline, 1180 has overflow,
and Recent Open/All plus global GO/PANIC remain usable.

## Risk, red-team, test and actual evidence status

- Tests: real JUCE return native button callback, hidden LIVE,
  intentional Row GO takeover, model LIVE/NEXT invariants,
  long names, unchanged state of global GO while browsing,
  screenshot pixels for title states, geometry under resize.
  Code added; Windows CTest execution **ND at this stage**.
- Real offscreen native images for L01 LIVE vs L09 VIEW at
  1180x820, 1647x895 and 1920x1080; images capture specified,
  visual review of artifact **pending**.
- Windows Release/CTest/version guard: **ND** until exact final SHA.
- Manual Windows physical 100/125/150 DPI, keyboard/IME/Narrator,
  touch/hitbox response time and audio/MIDI/OSC/LTC hardware: **ND**.
- Source control safety: no audio thread, routing, GO, STOP, PANIC,
  Undo/Redo, Inspector, Timeline, Master or JSON v10 data shape edit.
  Existing tab rename/reorder and Recent actions stay reachable.
- Residual known risk: modal Esc/PANIC issue #191; unrelated.

**Recommendation:** accept the draft only after zero CTest failures
and real native PNG inspection on the final commit, followed by
human/operator verification before LIVE readiness. No automatic
merge. **Rollback:** revert PR; schema unchanged.
