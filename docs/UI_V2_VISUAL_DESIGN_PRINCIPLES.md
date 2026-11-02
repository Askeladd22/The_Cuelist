# The CUElist — Visual design principles for native V2

Updated 2026-11-02. These principles are a concrete design and review contract, not a new HTML mockup.

## Goal

The UI must look like a deliberate professional show-control workstation, **not a generic AI-generated dashboard**. Legibility, predictable operations and consistent rhythm are more important than making every area visually prominent.

## Decisions

1. **One surface per functional region.** Do not put a rounded card inside a rounded card inside a raised modal-looking panel. Use the existing Midnight Slate base and a hairline to separate real sections. Raised surfaces are reserved for interacting or selected elements, not for every label.
2. **Colour carries meaning.** Teal may indicate active output/GO/playing, amber a pending or cautionary cue and MIDI-related context, purple OSC and red stop/error. Ordinary section headings are neutral. Avoid using teal simply to decorate each heading or panel edge.
3. **Typography shows hierarchy.** Use embedded Hind for large operational titles and Exo 2 for compact labels, respecting the application's 10pt minimum. One strong selected cue identity, small descriptive grouping labels and readable field labels; minimize repeated uppercase labels.
4. **Control surfaces are honest.** A button-like surface has a working action. Read-only meters and status do not look clickable. Empty Audio/MIDI/OSC cells do not show phantom transports.
5. **Keep geometry stable during a live show.** Cue Grid row 62px, fixed header 72px, fixed BPM/Auto Stop band 184px and left Master Audio lane 318px remain unchanged unless a later explicitly approved stage requires migration. Use the same rectangles for painting, pointer targets and tests.
6. **Keep necessary complexity visible.** Do not replace real features with a decorative placeholder. Maintain inline edits, specific field validation, MIDI/OSC Learn, routing, Inspector, Timeline, docks, menus, Undo/Redo, device state, peak meters, safety actions and the full project schema.
7. **Prefer tested restraint over more styling.** Each change must show what it removes and why. Inspect at 1180/1280/1440/1920 and 32:9 widths, compact heights, and 100/125/150% Windows DPI. Add meaningful native JUCE regression tests; the Windows build alone does not certify visual quality.

## N4 — Inspector application

- Render a single flat Inspector backdrop with a quiet left boundary.
- Keep `CUE NN` as the main point of reference, with only the selected column type coloured.
- Use **CUE PARAMETERS**, **CELL PARAMETERS** and **CUE NOTES** as subtle semantic section labels, separated by thin rules instead of nested rounded cards.
- Preserve the exact existing JUCE controls and geometry: cue name, initial BPM, tempo map, cue loop, cell name, gain, follows-row, loop, edit/import actions and operational notes.
- Preserve `WorkspaceLayout::inspectorTransform` and `scaleInspectorControl` so narrow and short windows retain tested field alignment. A later N4.2 may improve compact layout, but must come with screenshot and keyboard regression checks.

## Outstanding stages after N4

**N5:** Audio/MIDI/OSC/Control mixer strip consistency; **N6:** Timeline and docked editors; **N7:** detailed MIDI/OSC/Control Mapping workflows; **N8:** dialogs, popups, Guide, keyboard/focus accessibility; **N9:** Windows/DPI/ultrawide screenshots plus full hardware and project round-trip verification.

All future design decisions should meet the above contract, and neither a green test suite nor a mocked preview should be described as proof that the operator experience is visually finalized.
