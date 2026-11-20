# RQ-QA-008 — MSVC C4456 in Timeline OSC automation paint (v0.9.88)

**2026-11-20. Baseline:** main@1b7c1e18481a22224ed71a26a4f85e3191984327, VERSION.txt 0.9.87. **Target** 0.9.88. Project .cuelist schema JSON v10 must not change.

## Scenario and evidence
Windows Release and CTest regularly log MSVC **warning C4456** in `src/Timeline.cpp:409` (`start` shadows a previous local). Inspection of the real function shows two different timing spaces: outer `start` denotes the clip's absolute offset on the Timeline; inner `start`/ `end` denote a local OSC automation span relative to that clip. These locals are used only to compute from/to stripe coordinates. MSVC flags a real ambiguity for maintainers; there is **no observed playback error** in the issue and no claim that this warning proves a transport failure.

**Source and comparison:** original `src/Timeline.cpp:365-441` at pinned baseline, `tests/OscAutomationV9Tests.h` for editor regressions, `tests/TimelineV3Tests.h` for genuine GUI Timeline snapshots, and repeated [Windows CI 44746264655](https://github.com/Askeladd22/The_Cuelist/actions/runs/44746264655) logs showing C4456. These are repository/CI evidence, not hardware measurements.

## Hypothesis, alternatives, red-team
**H1** (measurable): replacing the inner `start/end` identifiers with `automationStart/automationEnd`, without changing expressions, eliminates C4456 on Timeline.cpp while preserving the pixels of OSC automation rendering, clip absolute start and JSON v10 data.

**Chosen:** single local rename/comment, no new function, no timer or audio-engine change. The edited expression uses the original coordinates and relative time mode. **Simplify Codebase:** no duplicate drawing helper or state. **Rejected:** broad refactor of the full Timeline or OSC editor, suppression of C4456 for the whole translation unit, reinterpreting cue start values, or altering OSC UDP scheduling to silence a compiler warning.

**Counterexamples:** no actual application behavior regression may have existed; a warning-free build cannot prove output timing. The native test can detect that a real automation stripe is drawn and that rendering leaves serialized data unchanged, but cannot certify pixel-perfect fidelity across desktop DPI or actual UDP receiver arrivals. A manual pre/post screenshot diff with a fixed OSC test fixture is an optional extra gate, not represented as already executed.

## PAUL / SEED test and verification gates
- **A1:** MSVC Release log on original SHA shows `warning C4456` at `src/Timeline.cpp:409` (observed), while new exact SHA does not (pending).
- **A2:** native GUI snapshots actually differ when an OSC automation is removed in *both* seconds and beats modes; they are not mocks of calculated x coordinates (pending).
- **A3:** original OSC clip `startSeconds=1.25` and project JSON `encodeShow` remain unchanged after both renders; all existing OSC/Timeline/MIDI/Audio tests PASS (pending).
- **A4:** code-level diff modifies only `src/Timeline.cpp`, the single targeted test, version metadata and RQ report. Windows Release/CTest, CodeQL, version guard must be checked on exact PR SHA; **no automatic merge**.
- **A5:** Original master 0.0 dB visual anomaly remains tracked independently in [issue #217](https://github.com/Askeladd22/The_Cuelist/issues/217); do not claim this Timeline PR solves it.

| Gate | Status / verification |
| --- | --- |
| Baseline warning | VERIFIED: original Windows MSVC CI emitted C4456 at Timeline.cpp:409 |
| Test-first RED | NOT APPLICABLE for runtime behavior: original warning is observed baseline; no fabricated failing unit test |
| New native OSC test | NOT RUN; compare snapshots with and without ramp in 2 timing modes |
| Warning removal on Windows | NOT RUN; inspect job log on exact SHA |
| VERSION/CMake/CHANGELOG | NOT RUN |
| CodeQL alerts | ND; scan and triage separately |
| External MIDI/OSC/audio/LTC, DPI and transport timing | ND — hardware not run |
| Rollback | Revert isolated PR; no JSON conversion |

**Trail of Bits C++ manual review:** scoped to Timeline OSC paint loop for shadowed locals, conversion double→float coordinates, validity of existing timing access, ownership of `cell.osc`, absence of new allocations or callback work, and unchanged OSC routing. This is a manual methodology, **not** the upstream scanner. **Matt Pocock:** genuine prior compiler warning as RED signal, verify after compile; the new native test is a regression, not a runtime bug reproduction. **CCteam QA/Unlazy:** record exact CTest numbers and all missing gates. **SEED:** quality gate requires Windows/CodeQL evidence. `main` is not protected by branch rules; don't auto-merge.

## Risks and rollback
Operator risk **low** for name-only patch, potentially positive diagnostic hygiene: future compiler warnings remain visible amid less noise. Reject any change that moves automation pixels, changes beat conversion, modifies show state, or breaks GO/MIDI/OSC output. Revert PR if tests fail, no migration.
