# GATES — R2 native operator LIVE / NEXT GO / VIEW status panel

BASE: verified main@0a2b0b997f0e889ecd6ab2cf0f12e264b6aec0aa;
VERSION.txt 0.9.71; JUCE 8.0.12; .cuelist schema JSON v10.
ISSUE: https://github.com/Askeladd22/The_Cuelist/issues/196
BRANCH: feat/r2-live-view-operator-sidebar-v0972; proposed 0.9.72.
Never auto-merge, change transport execution semantics, or certify LIVE.

## Operator hypothesis / counterexamples

H-R2: at minimum 1180×820 and desktop 1647×895, the sidebar next
to GO/PANIC distinctly identifies the live Cuelist locator/name,
next global GO target, and separately viewed Cuelist locator/name;
the native GO/PANIC hitboxes, shortcuts, triggering, JSON v10 and
VIEW/LIVE state transitions remain unchanged. For identical long
names, L01/L09 disambiguate; on VIEW=LIVE no misleading second active
transport is painted. No false 'playing' indicator: LIVE marks owning
transport, not guaranteed audio output.

Source baseline: MainComponent.cpp painted a 51px ON DECK card
with only a 94px name. Recent Projects panel filled sidebar until
height-349; GO/PANIC native controls at height-282 and -228.
User screenshot and exact source provided visual/operator evidence.

GStack red-team:
- hidden LIVE in overflow remains L01; browsing L09 doesn't change GO
- next GO 'END' must not imply GO will launch a cue
- 1180x820 may lose one/two recent list rows; all remain accessible
  via real All control, not hidden when capacity drops
- 64-character similar names, Unicode, 32:9, collapsed Inspector,
  12 Cuelists, silence vs playback.
- avoid relocating GO/PANIC, duplicating clickable status controls,
  introducing audio-thread or network activity, or changing saved Show.

## Gates, status
- [x] R2-1 baseline/source/spec reviewed: project docs V2 visual/parity,
  MainComponent paint/resized, CueGrid LIVE header, functions returning
  actual LIVE/next; issue #196 published.
- [ ] R2-2 introduce one shared pure SidebarOperatorLayout for recent
  boundary, status card and text rectangles; no paint vs resized drift.
- [ ] R2-3 real JUCE UI: quiet card, larger LIVE name, NEXT GLOBAL GO,
  distinct VIEW locator/name, proper GO END semantic; original GO/PANIC
  still same clickable controls and callbacks; recent More fully reachable.
- [ ] R2-4 pure geometry and native component tests at 1180/1647/1920/
  3840 with hitboxes and no overlaps; hidden LIVE fixture checks saved
  Show, LIVE/NEXT, global GO tooltip and returns when VIEW==LIVE.
- [ ] R2-5 native offscreen JUCE snapshots at 1180 and 1647 with
  LIVE hidden behind tab overflow and END state; human visual review
  for heading/ellipsis and transport spacing. Not DPI OS proof.
- [ ] R2-6 version 0.9.72 in VERSION, CMake and CHANGELOG, Windows
  version guard SUCCESS on exact final SHA.
- [ ] R2-7 Windows Release and full CTest on exact final PR SHA:
  0 failing, screenshot artifact uploaded; log checked.
- [ ] R2-8 GStack/Matt source diff review: no changes to mixer, master,
  cue row/grid geometry, Control Mapping, MIDI/OSC/LTC or engine, no
  silent loss of the Recent Open/All shortcuts or Show state.
- [ ] R2-9 physical Windows keyboard/IME/Narrator/DPI 100/125/150,
  user/operator A/B identification time and actual hardware
  Audio/MIDI/OSC/LTC: ND/NOT RUN; not ready-for-LIVE.
- [ ] R2-10 modal Esc/PANIC issue #191 separately unresolved.

## Tests and rollback

Test seams are pure calculateWorkspaceLayout() for geometry,
MainComponent paint/native controls and existing RQ001 T03 fixture
for VIEW/LIVE/GO identity. No untrusted CHECK or inherited scripts
executed. Candidate CI checks are from repository's existing
Windows Release / Version workflows, whose pass/fail must be read
from exact run. Rollback: revert focused draft PR before or after
manual merge; no persisted schema migration or output behavior change.
