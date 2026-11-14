# RQ-UI-R1 / N4.2 — Inspector native structural redesign

**Date:** 2026-11-14
**Scenario:** a Windows show operator selects one Cue and one Audio/MIDI/OSC/Control cell while VIEW may differ from LIVE.
**Source baseline:** `main@48c69fff872028f3efafecca86a4c1a55f98772c`,
`VERSION.txt=0.9.69`, JUCE 8.0.12, project JSON v10.
**Tracking:** [issue #193](https://github.com/Askeladd22/The_Cuelist/issues/193).
**Status:** reversible N4.2 native C++ proposal, not a finished redesign.

## 1. The observed issue / user evidence

The operator-provided 1647x895 screenshot has unusually small text,
long gaps inside Inspector and Notes anchored at the very bottom.
Existing source `WorkspaceLayout::inspectorScale` uses a common
transform of 0.72–0.92, and `MainComponent::paint` applies an
`AffineTransform` only to Inspector labels while `resized()`
independently scales the actual JUCE child controls. This separation
causes duplicated coordinates and makes controlled reflow difficult.

The screenshot is a near-empty show, not a measured 30-cue/11-column
stress test. The visual problem is observed; a claimed reduction in
operator errors is NOT measured.

## 2. Sources, decisions and comparisons

- `docs/UI_V2_VISUAL_DESIGN_PRINCIPLES.md`: single functional surface,
  restrained dividers, embedded Hind/Exo 2 >=10pt, no gratuitous cards,
  *N4.2 compact Inspector layout only after native test/screenshot*.
- `docs/UI_V2_FEATURE_PARITY.md`: preserve exact original Master Audio
  component, all real workflow callbacks, controls, GO/PANIC,
  Control Mapping, Undo/Redo, JSON v10; no HTML substitution.
- `docs/UI_V2_REDESIGN_REMAINING_WORK.md`: Inspector N4 listed as a
  high-priority issue with historical coordinates/scaling.
- `src/WorkspaceLayout.{h,cpp}`, `src/MainComponent.cpp`,
  `src/InspectorController.cpp`: authoritative current implementation.
- `src/Main.cpp`: the actual main window has minimum 1180x820.
- `tests/WorkspaceLayoutTests.h`: existing native component/geometry
  seam (to avoid testing a fake layout).
- JUCE APIs are used, but no external device or Windows DPI test has
  been performed during proposal.

### Baseline vs N4.2 structural slice

| Concern | Original 0.9.69 | N4.2 change |
|---|---|---|
| Inspector form labels | Affine-scaled (0.72-0.92), distinct from controls | Unscaled 12-13pt labels at shared native coordinates |
| Child editor positioning | Historical literals, second scale pass | Pure `InspectorContentLayout` rectangles used by both paint and resized |
| Cue / Cell grouping | Historical vertical gaps | Compact Cue, Cell, Notes hierarchy with controlled vertical rhythm |
| Notes at 1180x820 | Bottom-anchored to historical action coordinates | Textbox >=70px after both MIDI actions; grows on taller windows |
| Audio gain and cell toggle | Existing JUCE controls/validations | Same components and callbacks, only bounds changed |
| MIDI/OSC/Control actions | Existing Edit and Import controls | Both actions retained, same target IDs/callbacks; no extra placeholder |
| Mixer, Master and Cue Grid | Original geometry, native 318px Master | **Unchanged**, deliberate |
| Dark themes | Industrial Orange and Legacy | Same tokens, no decorative gradients/motion |
| Project v10 | Native Show JSON v10 | Unchanged; no migration |

## 3. Functionality inventory and parity

| Existing operator function | Existing component | N4.2 bounds and status |
|---|---|---|
| Cue name | `rowName` native TextEditor | `i.cueNameField`, preserves focus and commit |
| Initial BPM | `bpm` native TextEditor | `i.bpmField`, retains validation/tempo map |
| Loop all clips (Cue) | `rowLoop` | `i.rowLoop` |
| Cell name | `cellName` native TextEditor | `i.cellNameField` |
| Cell Audio gain (dB) | `gainDb` | `i.gainField`, visible only for Audio |
| Include in Row GO | `follow` | `i.followToggle` |
| Cell loop playback | `loop` | `i.loopToggle` |
| Open/create MIDI clip | `midiEditButton` | `i.editAction` |
| Open/create OSC clip / Control Set | `oscEditButton` | `i.editAction` |
| Audio/MIDI import | `importButton` | `i.importAction`, type-specific |
| Editable Cue notes | `cueNotes` multiline TextEditor | `i.notesField`, scrollbars and Esc propagation preserved |
| Inspector hide/show | `inspectorSideTab` | unchanged 20x46 grip, 26px collapsed rail |

Controls intentionally hidden before N4.2 (legacy tabs, rowBehaviour,
manual Apply) remain hidden: there is no silent feature deletion.
The conditional visibility contracts remain in `InspectorController`.

## 4. Falsifiable hypothesis, red team and operator risk

H-N4.2: at all supported min/max sizes the Cue, Cell and Notes groups
have no caption/control overlap, native editable controls stay >=26–33px
high, and Cue Notes remains >=70px. This should improve visual clarity
at 1180x820 and increase notes space at 1647x895, with no lost action.

**Counterexamples** to look for:
1. MIDI has BOTH import and edit actions; note header must remain below
   both buttons even at 820px. Audio only has import, OSC/Control edit
   without audio gain; no hidden button should steal focus.
2. Changing selected row/column during LIVE must not change
   `liveCuelistIndex`, `nextCueIndex`, GO target, mapping or JSON data.
3. Inspector collapsed while changing cue must never reveal fields.
4. Long cue and cell names (including RTL/Unicode), Control Set name,
   disabled state, gain value -60/+12, notes 8192 chars, keyboard
   navigation and IME must still be usable.
5. No global GO/PANIC hitbox movement, no new audio-thread processing,
   no changed Master or native mixer routing, no new UI framework.
6. An off-screen `createComponentSnapshot()` is a rendering artifact,
   NOT Windows 100/125/150% DPI, Narrator or accessibility certification.
7. A tidy Inspector alone does not mean the entire workspace has been
   structurally redesigned. Sidebar, global info hierarchy, cue density
   and Timeline are separate future work.

## 5. Test and measured results

- **Native pure geometry tests**: `tests/WorkspaceLayoutTests.h`,
  viewport sizes 1180x820, 1440x900, 1647x895, 1920x1080,
  3840x1080; checks native fields, section ordering, notes, pinned
  metrics and unchanged Cue Grid geometry. **Execution: ND until CI**.
- **Native component tests**: actual `MainComponent` and named JUCE
  child controls, bounds equals pure layout at 1180/1647/1920;
  persisted Show + LIVE/NEXT unchanged after resize.
  **Execution: ND until CI**.
- **Native offscreen screenshots**: `interface-n42-inspector-audio-1180.png`,
  `-audio-1647.png`, `-audio-1920.png`, `-midi-1180.png`,
  `-midi-1647.png`. Need inspect output artifact after CI.
- **Windows Release/CTest**: ND at this writing, exact SHA required.
- **Windows physical 100/125/150 DPI + Narrator + keyboard/IME**: ND.
- **Operators, median identification time, error rate A/B**: ND,
  target comparison is baseline vs new on matched real Cue scenario.
- **External Audio/MIDI/OSC/LTC timing/hardware**: ND; outside UI slice.

## 6. Acceptance and rollback

Accept for UI preview only after Windows Release/CTest full pass,
inspection of actual JUCE screenshot artifacts and code review verifying
no loss of controls or editing state. Merge remains manual, no automatic
integration. Do not call ready-for-LIVE until a separate real bench.

Rollback = revert this small PR; no persistent model changes, JSON
migration, altered shortcuts or transport state. The eventual workspace
redesign still requires follow-up architectural decisions and
full-session screenshots of many cue lists, 13+ cues and 11 columns.
