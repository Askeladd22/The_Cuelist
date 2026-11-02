# The CUElist — Native V2: remaining redesign roadmap (audit 2026-11-02)

This plan is an **audit of actual native JUCE sources and current Windows CI**, not a new HTML mockup. Protect all show-control features and the JSON v10 schema. Each increment must update VERSION.txt and CHANGELOG.md, run full Windows CMake/CTest and receive visual review before release sign-off. No portable package requested.

## Verified state

- **main beta.27:** global Midnight Slate header with 72px height; clock, CPU/RAM and four MIDI/OSC activity indicators; dedicated always-visible MIDI/OSC Learn under the logo; responsive Cuelist tabs and operator menus. The beta.26 Windows CI passed.
- **Cue Grid N3.1–N3.6:** shared 62px row geometry and 72px pinned headings; two-tier cue name/transport, column headers, selection/drag, inline rename, visual GO/Play/Stop/menu hover; fixed BPM/Auto Stop with no CELLS, inline BPM validation and preserved CONT exceptions.
- **Master Audio:** `ColumnUtilities` directly owns the pinned 318px Master panel, separate from `scrollableStrips`, with real fader, stereo peak, pan, mute, limiter, output routing and Quick Learn targets.
- **beta.27 regression:** Windows application compiled, but CTest failed 1 of 406851 assertions in N3.6 scroll fixture. Test edited the model without rebuilding the viewport because it reused the constructor's 1440x900 window dimensions.
- **beta.28 (this PR):** fixes that fixture, adds a loaded-cell hover state and real Play/Stop affordances for Audio/MIDI/OSC; keeps empty/unloaded cells inert. Windows CI is a required gate.

## Remaining work, priority order

### N4 — Inspector consistency (high)

Review the row and cell cards in `src/MainComponent.cpp`, `src/InspectorController.cpp` and `src/WorkspaceLayout.cpp`. They still rely on historical coordinate scaling and dense form layouts. Harmonize hierarchy, Exo 2/Hind spacing, native editable fields, explicit selected-row/cell context, collapsed/expanded edge tab, keyboard focus/validation and narrow-window behavior. Preserve entire-row and per-cell parameters, gain dB, cues notes, callbacks, Undo and live apply.

### N5 — Per-column mixer/strip finish (high)

Audit `src/ColumnUtilities.cpp`, `src/GainMeterFader.cpp`, audio/mixer tests, every Audio/MIDI/OSC/Control strip. Align margins, meter/fader scales, labels, peak clipping, hover states, pan pots, mute, output routing, overflow and X-scroll. Preserve and explicitly test the fixed **318px Master Audio** independently of other strips, for compact heights and 32:9.

### N6 — Timeline and dock integration (high)

Review `src/Timeline.cpp` and `src/WorkspaceLayout.cpp` to unify timeline control density, header/ruler, BPM/grid/snap (which have different semantics), clips, scroll, seek, zoom, collision and resize behavior. Preserve both collapsed/minimum and expanded layouts. Keep the fixed Cue Grid, pinned metrics, active docked MIDI/OSC editor and Inspector non-overlapping.

### N7 — MIDI/OSC editors & Control Mapping (medium/high)

Harmonize `src/PianoRoll.cpp`, `src/MidiUI.cpp`, `src/OscEditor.cpp`, `src/OscUI.cpp`, `src/ControlUI.cpp`: docking/undocking, toolbar and clip geometry, draggable clip length, native scrollbars, disabled/error states, learn overlays and panel spacing. Preserve OSC multi-message editing, MIDI resolution/timing, Undo/Redo, mapping presets and device discovery. No replacement with placeholders.

### N8 — Dialogs, menus, accessibility & typography (medium)

Review modal prompts, OS file picker, device/routing windows, context menus, Guide/About, feedback, keyboard operation, contrast, touch targets and Unicode glyph fallback. Preserve current protective confirmations, error handling and shortcuts. Use embedded Hind/Exo 2, explicit Noto symbols only where appropriate. No feature removal disguised as visual cleanup.

### N9 — Final verification & sign-off (release gate)

Use real Windows snapshots and hands-on interaction at 100%, 125%, 150% DPI and narrow/ultrawide windows (including 1280x768, 1440x900, 1920x1080, 3840x1080). Verify row/column scroll and repaint, all menus and focus paths, Live Audio/MIDI/OSC devices, Master bus/routing/mapping, BPM and Auto Stop exceptions, crash recovery, project save/reopen and full CI. Automated tests cannot substitute for the visual/hardware check. Document all intentional deviations and do not merge a failing CI.

## Non-negotiable invariants

- Keep GO/Panic and per-row/per-cell transport functions, shortcut mapping, Undo/Redo, multi-Cuelist rename/reorder/overflow, drag/drop, import and save.
- Fixed BPM and Auto Stop on the right; the cue-level toggle is the only on-row stop control; cell exceptions remain via Inspector/context menu/CONT indicators.
- Master Audio remains pinned to the Cuelist lane of width 318px and outside horizontally scrolling strips.
- Clock/CPU/RAM/I/O/audio monitor remains real telemetry; global MIDI/OSC Learn commands remain one-click under logo.
- Do not rewrite core engine, silently change project schema or build a portable bundle.
