# RQ-QA-008 — MSVC C4456 in Timeline OSC automation paint (v0.9.88)

**2026-11-20. Baseline:** main@1b7c1e18481a22224ed71a26a4f85e3191984327, VERSION.txt 0.9.87. **Target** 0.9.88. Project .cuelist schema JSON v10 must not change.

## Scenario and evidence
Windows Release and CTest regularly log MSVC **warning C4456** in `src/Timeline.cpp:409` (`start` shadows a previous local). Inspection of the real function shows two different timing spaces: outer `start` denotes the clip's absolute offset on the Timeline; inner `start`/ `end` denote a local OSC automation span relative to that clip. These locals are used only to compute from/to stripe coordinates. MSVC flags a real ambiguity for maintainers; there is **no observed playback error** in the issue and no claim that this warning proves a transport failure.

**Source and comparison:** original `src/Timeline.cpp:365-441` at pinned baseline, `tests/OscAutomationV9Tests.h` for editor regressions, `tests/TimelineV3Tests.h` for genuine GUI Timeline snapshots, and repeated [Windows CI 44746264655](https://github.com/Askeladd22/The_Cuelist/actions/runs/44746264655) logs showing C4456. Microsoft documents the [C4456 scope-shadow warning](https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4456?view=msvc-170) and recommends distinct local names. These are official compiler guidance and repository/CI evidence, not hardware measurements.

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
| New native OSC test | **PASS**: [Windows 44754748213](https://github.com/Askeladd22/The_Cuelist/actions/runs/44754748213), code SHA 806d9ac7: `OSC Timeline paints automation strips in seconds and beats without clip mutation` PASS, 421177 assertions, 0 failed tests |
| Warning removal on Windows | **PASS**: same completed run, zero `C4456` occurrences and zero compiler warning-code lines in collected job logs; no broad warning suppression |
| VERSION/CMake/CHANGELOG | **PASS**: [guard 44754748268](https://github.com/Askeladd22/The_Cuelist/actions/runs/44754748268) on source SHA; a documentation-only follow-up commit must reverify its own guard |
| CodeQL C++ | [run 44754748578](https://github.com/Askeladd22/The_Cuelist/actions/runs/44754748578) on source SHA was IN PROGRESS when recorded; final doc SHA must be verified separately |
| CodeQL alerts | ND; scan is not equivalent to alert triage; connector does not expose the Code Scanning alert collection |
| External MIDI/OSC/audio/LTC, DPI and transport timing | ND — hardware not run |
| Rollback | Revert isolated PR; no JSON conversion |

**Trail of Bits C++ manual review:** scoped to Timeline OSC paint loop for shadowed locals, conversion double→float coordinates, validity of existing timing access, ownership of `cell.osc`, absence of new allocations or callback work, and unchanged OSC routing. This is a manual methodology, **not** the upstream scanner. **Matt Pocock:** genuine prior compiler warning as RED signal, verify after compile; the new native test is a regression, not a runtime bug reproduction. **CCteam QA/Unlazy:** record exact CTest numbers and all missing gates. **SEED:** quality gate requires Windows/CodeQL evidence. `main` is not protected by branch rules; don't auto-merge.

## Recorded Windows measurement (on fixed source)

[Run 44754748213](https://github.com/Askeladd22/The_Cuelist/actions/runs/44754748213) completed **success** for code SHA `806d9ac71a1d8f91b6cb6b032aa595f521cfb37b`: C++ MSVC Release compile, **421,177 assertions, zero failures**, 100% CTest PASS, and native screenshot artifact successfully uploaded. The exact new OSC snapshot test PASS in both seconds and beats. A scan of that completed build log finds **zero occurrences of C4456** and **zero compiler warning-code lines**. This is an observed compiler hygiene improvement, not an output-synchronism measurement.

**Remaining gates:** CodeQL on the source SHA and on any later documentation-only head; real show hardware and native physical DPI ND. If the QA notes commit changes only RQ/CHANGELOG, do not misattribute a source test to the later SHA: link to the verified source commit and obtain at least the final version guard.

## Risks and rollback
Operator risk **low** for name-only patch, potentially positive diagnostic hygiene: future compiler warnings remain visible amid less noise. Reject any change that moves automation pixels, changes beat conversion, modifies show state, or breaks GO/MIDI/OSC output. Revert PR if tests fail, no migration.
