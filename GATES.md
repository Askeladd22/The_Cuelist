# GATES — R3 / pinned Cue Grid and BPM/Auto Stop typography

Baseline: main@2f028502559da351dba665af7ca6f1f2fa75d7e0,
VERSION.txt 0.9.72, JUCE native 8, schema .cuelist JSON v10.
Issue #198; feature branch fix/r3-pinned-header-typography-v0973.
Target version 0.9.73. This is a typography-only UI increment.

## Operator problem and falsifiable hypothesis

User supplied three native crops: Master Audio, BPM/Auto Stop, Cue Grid.
They ask whether the characters should be justified. Design decision:
NO full/space-distributed justification; left align scan targets
(names, titles), centre numbers in their metric lanes and control
captions over real controls. Do not modify MasterAudioLayout or
ColumnUtilities::paint; its current alignment already follows this
rule and it is explicitly protected by canonical V2 feature parity.

Source-diagnosed defect: Theme.h clamps point height to
max(10.0, nominalSize*0.75). Thus labels using 10–13 nominal
sizes all render as 10pt. The existing Cue Grid LIVE locator/name
and next GO use nominal 11–12, while BPM/AUTO STOP headings use 13.
A stronger operative hierarchy requires *actual* size > 13.333
without a global font-system overhaul.

H-R3: font(15,true) yields >10pt native Hind Semibold (for labels
that currently render 10pt Exo 2) and shaped text
LIVE L12 / GO 12 / BPM / AUTO STOP fits its existing rectangle
without ellipsis; the second-tier labels remain legible.
No change to any geometry/hitboxes, transport state, routing, Master,
Cue row, .cuelist v10 or existing typography fallback contracts.

## Source and review

- [x] Matt Pocock: reviewed Theme.h font routing and label function,
  CueViews.cpp pinned header and metrics, CueGridGeometry.h,
  MasterAudioLayout.h, ColumnUtilities.cpp, tests UiRefinementTests
  and canonical design/parity documents. Baseline and issue fixed.
- [x] GStack/Taste/Emil: screenshot audit; all-justified text or
  global blanket centering rejected. Master left/centre pattern
  intentionally preserved. UI output must be inspected at 1180
  and 1647 in actual JUCE offscreen rendering.
- [ ] Native C++ code: only pinned Cue Grid LIVE/GO labels and
  BPM/AUTO STOP headings; no font globals/engine/model changes.
- [ ] Regression: test the exact shaped string widths, family and
  point height; ensure header 72, row 62, name lane 318,
  pinned metrics 184, Master 318 remain original.
- [ ] Native screenshot: show LIVE L01 while VIEW L09 at 1180/1647,
  plus at least one normal 1440/1920 view. Inspect PNG,
  especially no ellipsis on GO/locator or Auto Stop text.
- [ ] VERSION.txt and CMakeLists.txt=0.9.73, CHANGELOG.md updated,
  guard on exact PR SHA SUCCESS.
- [ ] Windows Release & full CTest on exact PR SHA PASS, logs checked,
  screenshot artifact from that same SHA.
- [ ] Diff review: no changes to ColumnUtilities/MasterAudioLayout,
  main GO/STOP/PANIC, VIEW/LIVE, MIDI/OSC/LTC, Quick Learn,
  Undo/Redo, timeline, Mixer, project v10.
- [ ] Native Windows DPI 100/125/150%, Narrator/IME, external hardware
  Audio/MIDI/OSC/LTC, operator A/B task time and accessibility
  performance: NOT RUN / ND. CI green != ready for LIVE.
- [ ] Issue #191 modal first Esc/PANIC remains unresolved.

## Red team / rollback

Stress two similarly named 64-character Cuelists and 12 tabs;
check ellipsis is confined to variable names, not fixed-status
labels. Check END instead of GO and both Industrial Orange/Legacy.
If glyph width exceeds a fixed label box, narrow the typography
locally rather than truncate a safety-relevant status. Keep any
snapshot updates actual JUCE, no HTML replacement. This entire
change is reversible by reverting the draft PR, no JSON migration.
Never auto merge. Do not execute inherited CHECKs/hooks.
