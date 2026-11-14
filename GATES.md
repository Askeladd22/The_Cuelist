# GATES — N4.2 Native Inspector Structural Layout (RQ-UI-R1)

**Baseline:** main@48c69fff872028f3efafecca86a4c1a55f98772c,
official VERSION.txt 0.9.69, JUCE 8.0.12, schema .cuelist JSON v10.
**Scope:** first structural UI/UX increment of issue #193;
Inspector Cue/Cell/Notes visual hierarchy and native component bounds.
Feature branch feat/n42-inspector-layout-v0970; target version 0.9.70.
Do not change GO/STOP/PANIC, LIVE/VIEW, Undo/Redo, Control Mappings,
MIDI/OSC, LTC, playback, CueGridGeometry 62/72/318/152, fixed BPM/Auto
Stop 184, original Master Audio 318, mixer/timeline/transport geometry,
Project JSON v10 or any audio-thread callback.

## Evidence / constraints

- User screenshot of 1647×895 Windows application: Inspector uses small
  scaled text, large vertical gaps and notes pushed toward bottom.
- Source: WorkspaceLayout::inspectorScale goes to 0.72; MainComponent
  paint scales labels via AffineTransform while resized() scales child
  controls separately. The current interface still has historical
  pixel locations.
- Canonical docs: docs/UI_V2_VISUAL_DESIGN_PRINCIPLES.md explicitly
  allows later Inspector N4.2 with screenshot/keyboard regression,
  and documents "one surface per functional region", embedded Hind/
  Exo 2 >=10pt. docs/UI_V2_FEATURE_PARITY.md prohibits removing
  features and redesigning Master.
- Main.cpp Window::setResizeLimits(1180,820,3840,2160): 820 is
  production minimum height (NOT a proposed 720px layout).
- Baseline Windows CI of PR #192 at final head completed success
  (run 43578069868); this does not validate N4.2.

## User stories and falsifiable hypothesis

Operator: "when I select a cue and cell, I can read and edit the
cue name, BPM, loop, cell name, gain, follows-row, loop, MIDI/OSC edit,
audio/MIDI import and cue notes without scanning a tiny transformed
form or losing critical controls".

H-N4.2: pure shared InspectorContentLayout uses **one source of
rectangles** for text and real child widgets. At 1180×820 and
1647×895: (1) all visible fields have stable >=26px controls;
(2) no overlaps between captions, active controls and Notes;
(3) Cue Notes textbox has >=70px height at 820, grows at 895;
(4) no inspector-wide affine scale (text remains 12–14pt);
(5) existing per-type visibility, operations, toggle state,
keyboard callbacks, LIVE/VIEW and JSON v10 unchanged.

## Review routing

- Matt Pocock: test seam = existing pure calculateWorkspaceLayout
  and actual JUCE MainComponent fields; standards/spec review after.
- GStack Engineering: inspect ownership and modal/focus/LIVE regressions.
- Taste + UI/UX Pro Max: hierarchy/content over decoration; no
  website-only patterns, not another palette pass.
- Emil Design Engineering/Break UI: long names, 1180/1647/1920/3840,
  empty project, MIDI dual actions, OSC/Control single action,
  Inspector collapsed, short 820px height.
- Unlazy: mark PASS only with exact commit-bound GitHub CI/log output;
  never execute inherited CHECK scripts/hook without explicit approval.

## Gates

- [x] R1 baseline, canonical visual spec, feature parity, window min
  size and current Inspector source examined; issue #193 opened.
- [ ] R2 document inventory of every visible native editor/callback/
  tooltip and conditional type/action; preserve in new bounds.
- [ ] R3 native implementation: one pure layout model with no
  duplicate paint/resized coordinates, no blanket inspector scale,
  accessible label and textbox sizes; no new UI shell/HTML.
- [ ] R4 tests: new pure rectangle assertions at 1180x820, 1440x900,
  1647x895, 1920x1080, ultrawide plus JUCE component bounds,
  visibility/selection/non-overlap, collapsed rail, v10 state.
  Expected: fail on old layout because fields are scaled and notes
  short; pass on new layout (actual red/green run required for TDD claim).
- [ ] R5 screenshot output: native JUCE off-screen snapshots at
  1180x820 and 1647x895 with each type (audio/MIDI/OSC/Control),
  plus a compact project and heavy cue matrix. Compare to user-supplied
  baseline and inspect PNGs; these do NOT prove Windows DPI.
- [ ] R6 Windows Release + all CTest on exact PR SHA: SUCCESS, 0 failed,
  diagnostics for specific N4.2 tests, screenshots artifact uploaded.
- [ ] R7 VERSION.txt and CMakeLists.txt 0.9.70 and CHANGELOG entry,
  version guard SUCCESS on exact head.
- [ ] R8 source diff review: Master and Cue Grid geometry unchanged,
  no lost functionality, selection, focus/IME/keyboard and cue notes
  persistence; links to exact PR patch.
- [ ] R9 Windows DPI 100/125/150 with physical mouse/keyboard,
  Narrator, long Unicode names, external Audio/MIDI/OSC/LTC,
  timing and operator A/B latency: **NOT RUN** until human bench.
- [ ] R10 unresolved issue #191 modal PopupMenu Esc/PANIC remains
  separate; no certified readiness LIVE.

## Rollback

Small draft PR, no automatic merge. Revert the PR or revert the
InspectorContentLayout call sites, restoring the previous coordinates.
No JSON migrations or changes to external device behavior.
