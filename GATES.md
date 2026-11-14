# GATES — R5 title LIVE/VIEW and return-to-LIVE (native JUCE)

Base: verified main@084a0c5af2185e3d9589525857e40f0c41ee7d0a,
official VERSION.txt 0.9.75, project schema .cuelist JSON v10,
approved annotated screenshot image(2).png 2026-11-14.
Issue #204. Target branch feat/r5-cuelist-live-view-title-v0976,
proposed 0.9.76. Draft PR only; no automatic merge.

## Measurable operator scenario (do not infer transport from visuals)

A show operator views L09 ("Emergency") while L01 ("Pre-show")
owns global GO, possibly running. The current pinned Cue Grid displays
a small VIEW title and duplicates LIVE L01/name/GO 02 that the sidebar
already supplies. The screenshot crosses those duplicates and crosses
VIEW/BROWSING in the sidebar, but explicitly draws a red LIVE badge
beside a larger viewed Cuelist name. The white box before the tabs
asks for a *view navigation* action back to the actual LIVE Cuelist.

Design/intent:
- Title in the original 318px pinned name header becomes a 22 logical
  font tier with name truncation; adjacent red LIVE if VIEW==LIVE,
  orange VIEW if browsing a different Cuelist.
- Second line keeps Lxx *view locator* and cue/clip counts only;
  remove duplicate LIVE/name/GO labels. Sidebar retains LIVE/ON DECK,
  Lxx/name and NEXT GLOBAL GO but drops its duplicate VIEW block.
- New 32x27 native icon before first Cuelist tab invokes the existing
  activateCuelist(liveCuelist), never goNext/triggerRow/stopAll.
  Do not cache liveCuelist or consume Space/GO by focus.
- Preserve all row/column/BPM/AutoStop geometry and native
  move-left/right/collapse and top tab actions. Move Cuelist arrows
  to free second header line; don't remove or rename them.
- Sidebar recent projects receives freed space; GO/PANIC stay at
  their exact original coordinates and callback ownership.
- Do not alter any audio/MIDI/OSC/LTC output, JSON v10 serialization,
  engine state, mixer/master, inspector, timeline or Control Maps.

## Evidence gates
- [x] R5-A: screenshot original and annotated crops examined; canon
  UI_V2_FEATURE_PARITY, current CueViews, MainComponent, WorkspaceLayout,
  CueOperations, tests T03/T03B/T04 inspected; baseline pinned, #204 open.
- [ ] R5-B: pure fixed header semantic contract and sufficient
  title/label rectangles at 1180,1647,1920 including 64-char names.
  No overlap with existing 30px Collapse All icon or arrows.
- [ ] R5-C: actual JUCE return-to-LIVE icon before tabs, runtime tooltip,
  enabled iff VIEW!=LIVE. Mouse click invokes existing VIEW switch,
  keeping live owner, next global GO and acknowledged cue unchanged;
  after intentional LIVE takeover it follows the new LIVE index.
- [ ] R5-D: no duplicate pinned LIVE and GO text, no duplicated
  sidebar VIEW text; LIVE red and VIEW orange beside the larger name,
  Lxx locator and VIEW counts remain readable; sidebar GO text correct.
- [ ] R5-E: tests that 1/9/12 Cuelists, active overflow, long/identical
  names, name rename, movement, per-Cuelist row GO and return control
  work; existing T03/T03B/T04 updated to the new visual contract
  (never weaken GO/ownership semantics).
- [ ] R5-F: native JUCE screenshot fixture 1180x820, 1647x895,
  1920x1080, VIEW=LIVE and VIEW!=LIVE. Visual QA inspects PNG and
  arrow, typography/truncation, orange/red contrast and sidebar.
- [ ] R5-G: VERSION.txt/CMakeLists/CHANGELOG 0.9.76, version guard
  SUCCESS on exact final SHA.
- [ ] R5-H: Windows Release and full CTest on exact SHA, no failed
  checks; inspect real logs and artifact, not build script alone.
- [ ] R5-I: review no accidental loss of Undo/Redo, GO, STOP, PANIC,
  keyboard Space, Control Maps, audio/MIDI/OSC/LTC, scene v10.
- [ ] R5-J: Windows desktop DPI 100/125/150, Narrator/IME, keyboard
  under modal focus, hardware output bench: ND/not run. CI green
  is not a LIVE readiness certificate. Modal Esc/PANIC #191 open.

## Worst cases, rollback, status

If name is long and badge must be positioned next to an ellipsized
label, it must still fit before Collapse All. Wide 1920 must retain
nine visible tabs; 1180 must have overflow access to hidden LIVE.
A one-click view change may commit pending Inspector/inline draft per
existing activateCuelist semantics, but may not stop audio.
No new Native keyboard focus on return button to avoid stealing
Space (global GO); accessibility tooltip and name remain.
Revert this small feature branch/PR to roll back; no migration of
saved Show project schema, no automatic merge or release claim.
All tests NOT RUN until exact CI results. Do not execute inherited
Unlazy CHECK scripts.
