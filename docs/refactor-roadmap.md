# The Cuelist — incremental refactor plan (post-0.9 GUI fixes)

> **Avvertenza documentale — revisione 13 novembre 2026.** Questo file contiene
> un piano storico: espressioni come “current PR”, “the next stage” e
> “CI must still be verified” descrivono il momento in cui furono scritte,
> non lo stato operativo attuale. La fonte della versione resta
> [VERSION.txt](../VERSION.txt); lo storico reale delle modifiche resta
> [CHANGELOG.md](../CHANGELOG.md). Non usare questi riferimenti storici come
> autorizzazione per nuovi merge o refactoring.

## Chiusura della campagna Simplify S1–S8 — 13 novembre 2026

**Perimetro:** le duplicazioni effettivamente verificate nelle superfici
native di menu, Quick Learn, codec JSON Control Binding, FileChooser,
ripristino dei clip MIDI/OSC e finestra di editor sganciata. Non è
una dichiarazione che *l'intero repository* sia libero da debito tecnico.

| Fase | Risultato integrato su `main` | PR verificata |
|---|---|---|
| S1 + S3 | Eliminato avvio OSC non richiamato; unica lista CMake dei sorgenti GUI | [#170](https://github.com/Askeladd22/The_Cuelist/pull/170) |
| S2 | Menu Settings nascosto delega al builder/dispatcher nativo | [#171](https://github.com/Askeladd22/The_Cuelist/pull/171) |
| S4 | Unico teardown dei flag della sessione Quick Learn | [#172](https://github.com/Askeladd22/The_Cuelist/pull/172) |
| S5 | Unico contratto di serializzazione dei 22 campi ControlBinding v10 | [#173](https://github.com/Askeladd22/The_Cuelist/pull/173) |
| S6 | FileChooser JSON condivisi, importatori Default Map e Control Map separati | [#174](https://github.com/Askeladd22/The_Cuelist/pull/174) |
| S7 | Unica ricerca per ID stabile negli Undo MIDI/OSC fra Cuelist | [#175](https://github.com/Askeladd22/The_Cuelist/pull/175) |
| S8 | Unico shell `juce::DocumentWindow` per gli editor MIDI e OSC | [#176](https://github.com/Askeladd22/The_Cuelist/pull/176) |

**Evidenza:** GitHub conferma il merge di tutte le PR #170–#176.
La CI Windows Release/CTest sulla PR #176 è terminata con
`success` ([run 43503240808](https://github.com/Askeladd22/The_Cuelist/actions/runs/43503240808)).
Questo dato non sostituisce un collaudo operativo esterno. La baseline
al momento della verifica era `main@ddc36ef07b20e45365554f43a3e57780a8d7ebca`
con versione applicativa `0.9.55`.

### Regola di arresto e attività distinte

**Questa campagna di semplificazione mirata termina qui**, salvo una
regressione effettiva dimostrata dagli ultimi test o dalla revisione.
Non aprire nuove micro-PR per mera riduzione di righe o per aggiungere
un'altra astrazione: ogni nuova modifica richiede un consumer identificato,
una complessità eliminabile, un'ipotesi falsificabile e un test decisivo.

**Gate ancora aperti (ND, non attestati da CI):** uso Windows nativo a
DPI 100/125/150%, click sulla X di chiusura, Detach/Dock di MIDI e OSC,
cambi VIEW durante una LIVE, banco Audio/MIDI/OSC e ricevitore LTC esterno,
misure di dropout, latenza e sincronismo end-to-end, recupero da errore
e verifiche di licensing prima di qualunque distribuzione. Registrare
risultati per configurazione, unità e soglia; non inferire prestazioni
a valle di UDP/MIDI da test simulati.

**Rollback:** ogni fase è una PR piccola e separata, reversibile per
revert. Nessuna richiede migrazione dello schema progetti JSON v10.
Per il lavoro successivo usare le RQ del laboratorio, anziché proseguire
automaticamente questa sequenza.

---

## Non-negotiable invariants

- No implicit GO, PANIC, transport reset or MIDI/OSC transmission when editing Inspector data.
- Preserve project JSON format v10 and the existing v1–v10 compatibility and backup/recovery behavior.
- Preserve the asynchronous project-load transaction: media is prepared and checked before the current show is replaced.
- Do not allocate, block, read files or introduce GUI locks in the audio callback.
- Keep fixed BPM / Auto Stop controls, cue row and column hit targets, editable Gain dB and docked editors intact.
- Build and run the Windows CTest suite before merging each phase. Physical interface rehearsal remains a separate acceptance gate.
- The portable release/package workflow remains manual.

## Phase 1 — merged into feat/desktop-foundation

**Goal:** isolate responsibilities without changing runtime behavior or data ownership.

- `src/InspectorController.cpp`: refresh Inspector controls, compare committed state to drafts, validate and commit edits. Owns the private Gain dB text parser.
- `src/ProjectLifecycle.cpp`: confirmation before discard, save/open, project activation, preflight/diagnostics, crash recovery and safe handoff to prepared media.
- `src/MainComponent.cpp`: retains visual composition, window sizing, event routing and coordination.
- `CMakeLists.txt`: compile the two new translation units into both the application and unit-test binaries.

**Important limit:** phase 1 separates source boundaries, **not** model ownership. These methods are still `MainComponent` members. This is intentional: moving ownership or changing event ordering before regression tests would make this first step substantially riskier.

## Phase 2 — cue editing boundaries

**Phase 2a (merged into feat/desktop-foundation):** split existing UI-owned editing commands by their responsibility, without changing logic, IDs, event ordering or ownership.

- `src/CueOperations.cpp`: row colour/Auto Stop, add/rename/reorder/copy/duplicate/paste/delete cue, column add/reorder/delete/collapse, tempo/BPM edits and the transient editing dialogs.
- `src/CellOperations.cpp`: cell swap/copy/paste/cut/delete, loop/follows-row toggles, clip timing and timeline seek.
- `src/MainComponent.cpp`: retains transport, mixer/routing, event coordination, device handling and visual layout; moving method definitions into separate translation units keeps all existing caller contracts.
- `CMakeLists.txt`: compile both modules in the application and tests.
- Extend automated tests with row/column identity, typed-cell associations and cached PCM ownership while preserving existing copy/paste and row duplication test coverage.

**Phase 2b (merged into feat/desktop-foundation):** introduce `ShowEditOperations.h/.cpp` in `CuelistCore` with three tested, JUCE-GUI-free model operations:

- `swapCueRows`: bounded cue reordering that preserves stable row IDs.
- `swapCueColumns`: bounded column reordering with corresponding typed cell slots swapped across every row; rejects malformed rectangular layouts.
- `prepareInsertedCueCopy`: transactional cue insertion into a separate output Show, reassigning row, cell and private Control Set binding IDs. The destination is only replaced after full `validateShow` succeeds; failed copies leave output unmodified.
- `CueOperations.cpp` keeps existing operator/transport sequencing but delegates these three model edits; the GUI still owns media-cache association and checks PCM memory limits **before** adopting a newly prepared show.
- New model-only regression tests cover stable IDs, clip/type association, correct Control Set identity cloning, range failures and project nonmutation.

**Phase 2c (merged into feat/desktop-foundation):**

- `prepareDeletedCue` and `prepareDeletedColumn` build a replacement Show and gather the deleted cell IDs without touching the current model, PCM cache, GUI or devices. Neither output changes on failure.
- Cue deletion removes legacy mappings scoped to that cue ID and clears stale locked-Control default-source cue IDs without discarding their settings; column deletion removes references to its ID from legacy mappings, locked Control Column defaults, cue-specific sets, and reusable templates.
- Cleanup includes column gain, **column pan**, MIDI CC, OSC output and explicit column-scoped mappings (previous UI deletion omitted column pan).
- After model validation succeeds, the UI removes only those cell IDs from its PCM cache, installs the prepared model, refreshes mapping state and invokes the existing stopped-transport change flow.
- Deleting the final cue or column leaves a valid blank cue or Audio column just as before; no live audio or file on disk is deleted.
- New model-only tests cover cross-layer mapping cleanup, locked-Control source metadata, stable IDs, last-item fallback and invalid-input atomicity; an additional UI test verifies that remaining decoded PCM survives column deletion.

The next stage is Phase 3: extracting testable workspace layout geometry from painting, while preserving existing screen positions.

No existing audio callback, playback state inheritance, project schema or media-cache ownership changes are made in phase 2a.

## Phase 3 — workspace layout boundaries

**Phase 3a (merged into feat/desktop-foundation):** centralize the screenshot-approved workspace geometry in the deterministic `WorkspaceLayout.h/.cpp` calculator.

- `calculateWorkspaceLayout` owns rectangular bounds for the cue viewport, mixer utilities, timeline, docked clip editor and drag/close/collapse handles, pinned BPM / Auto Stop area, inspector outer panel and both section cards, plus virtual cue grid dimensions.
- `WorkspaceLayout::inspectorTransform` and `scaleInspectorControl` share the identical vertical scaling formula previously duplicated between painting and `resized()`. The Inspector card background and CELL heading remain aligned across window sizes.
- `MainComponent::workspaceLayout()` adapts current window size, cue/column dimensions and active dock height into a geometry input. `paint()` and `resized()` consume the same result, while `MainComponent` continues to own component lifetime, visibility and event routing.
- Regression tests verify 1180×820, 1440×900, 1647×895 and 1920×1080, scrolling/pinned controls, collapsed/expanded clip dock, the shared Inspector transform and actual JUCE component bounds.
- No visual reflow or spacing change in this phase; it is a structural extraction with the exact existing baseline coordinates and control sizes.

**Phase 3b (merged into feat/desktop-foundation — compact dock sizing):**

- Preserve the approved 0.9 cue-grid, Inspector, mixer and Timeline geometry for all dock states that already fit.
- Resolve the *visible* dock height inside `WorkspaceLayout` against the available window height. The requested editor height (up to 400px) remains a preference; minimum 310px grid region, 75px Timeline lane and 47px footer reserve take precedence. At 1180×820 the largest dock is 220px; at 1440×900 it is 300px; at 1920×1080 all 400px fit.
- When a compact window is later enlarged, restore the preferred dock height automatically. The resize handle starts subsequent drags from the *currently visible* height rather than an invisible oversized preference.
- No change to GO/PANIC, media handling, audio timing, MIDI/OSC, project serialization or fixed BPM/Auto Stop placement.
- Add geometry checks across supported display sizes, closed/collapsed/default dock baselines, and an end-to-end JUCE dock drag/resize/regrow test.

**Phase 3c (only if visual testing warrants it):** handle narrower-than-supported window sizes or revisit the Inspector's control density as a separate deliberate GUI design change, not as part of a runtime-safety fix.

## Phase 4 — Control Mapping boundaries

**Phase 4a (merged into feat/desktop-foundation; CI must still be verified):** isolate mapping/device business logic from the Control Panel dialog without changing runtime behavior.

- `src/ControlUI.cpp` retains `ControlPanel`, `openControlPanel` and `editControlSet`: presentation, operator controls and the MIDI Learn interface.
- `src/ControlRuntime.cpp` contains runtime mapping initialization, MIDI input selection, binding edits, presets, Control Set lock/priority/templates, and the original `dispatchControl`. The code still executes on the JUCE UI thread; the audio callback remains unchanged.
- `src/ControlLayerBuilder.h/.cpp` lives in the independent core library and provides deterministic `buildControlLayers(show)` and `activeControlPresetForCue(show, acknowledgedCueId)`. Visible Control Column order resolves equal-priority ties, locked defaults apply across cues, and selected rows never activate hardware mappings.
- Tests cover locked vs cue-local mappings, equal-priority reordering, acknowledged-cue presets, malformed in-memory grids and physical GO press/release debounce. Existing MIDI Learn, Control Set, mapping validation, pickup, MIDI/OSC routing and project v10 tests are preserved.
- No GUI geometry or audio callback changes. Windows build and complete automated tests required before merging.

**Phase 4b (merged into feat/desktop-foundation; CI must still be verified):**

- `src/ControlDispatchPolicy.h/.cpp` in `CuelistCore` owns deterministic physical MIDI button-edge tracking (`PhysicalControlEdges`), current-target normalization for pickup, stable column-ID/type selection, fader deadband and MIDI CC output quantization. These do not invoke audio callbacks or send to hardware.
- `src/MainComponent.h` keeps a `PhysicalControlEdges` member rather than a raw held-input map. Project MIDI input changes, disconnect and reconnect still clear the physical state before new events are accepted.
- `src/ControlRuntime.cpp` delegates only the *same* decisions to the core helpers. GO/PANIC continues to be triggered solely from a new physical edge on the UI thread. MIDI and OSC send functions are unchanged.
- Five new tests cover device/channel/input independence, unmatched releases across cue switches, reverse pickup curves and inversion, output type and ID after column reorder, and CC rounding/deadband.
- Full app/UI regressions and Windows release CI are required before merging; physical controller and MIDI/OSC output acceptance remains separate.

**Phase 4c (current PR; MIDI session reliability):**

- `src/MidiSessionPolicy.h/.cpp` separates *decisions* from MIDI port opening: `planMidiSession` checks only exact device identifiers in an enumerated device snapshot. It distinguishes an unplugged connected input, a selected input that has returned, a disabled configuration and a similarly named but different controller. It never opens ports itself.
- `MainComponent::timerCallback()` uses a single inventory snapshot on each normal refresh and retains the existing same-tick disconnect/reconnect sequence, status text and pressed-input clearing. It never connects to a different discovered device, and never triggers GO or PANIC on reconnection.
- `consumeMidiLearnEvent` preserves one-shot MIDI Learn behavior: an armed event updates the capture and is not dispatched to live outputs. Ordinary processing resumes after capture without changing supported MIDI event types.
- New offline regression tests cover exact-ID device replacement/reappearance, disabled inputs, MIDI Learn isolation (including mapped GO), stale packet queue reset, and post-reconnect physical button re-arming. No physical interfaces are needed for CI.
- No changes to audio callback, mapping priorities, output queues or project JSON v10. The operator must still rehearse device unplug/replug and MIDI/OSC routing on actual hardware.

**Later:** if live hardware tests reveal a need, inject dedicated MIDI/OSC output sinks for deterministic end-to-end routing tests in a separate PR. Do not casually alter MIDI/OSC queues, GO acknowledgment timing or audio callback behavior.

Hardware acceptance is mandatory before release.

## Release gate

Green CI establishes that source compilation and automated regression tests pass. It does not establish that actual connected audio interfaces and MIDI/OSC peripherals behave correctly during a live show.
