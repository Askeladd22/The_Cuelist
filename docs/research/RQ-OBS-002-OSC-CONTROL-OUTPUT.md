# RQ-OBS-002 — OSC Control Mapping output diagnostics and packet bounds

**Date:** 2026-11-20. **Base:** main@c9860e6ab33c3fe2c7281f8ecef3e082f12b96d5, application VERSION.txt 0.9.81; target 0.9.82. **Format:** JSON .cuelist v10 unchanged.

**PAUL:** PLAN → APPLY → UNIFY. **SEED:** Application/Deep, a small independent hardening slice. **Review lenses:** manual Trail of Bits C++ review, Matt Pocock reproducible RED/GREEN loop, CCteam QA, Unlazy evidence gate, Simplify Codebase scope control (no unnecessary new abstractions). These skill adaptations do not run their original external scanners.

## Scenario and observable failure

An operator uses a MIDI Control Map binding whose output is OSC. `ControlOutputRouter::sendOscFloat` currently validates the host, port, slash prefix and finite float but accepts addresses that yield OSC float messages over 1024 bytes. The worker `ControlOutputRouter::run` then silently discards them. On an unsuccessful `DatagramSocket::write`, it also does not increment `dropped`. `MainComponent::showDiagnostics` reports only the Timeline engine's OSC sent/dropped values, not the separate `controlOutput` router, although the I/O activity LED counts combined sends.

**Consequence:** a Control Map output can report queue admission even when it will be discarded, and the operator may see no corresponding send failure in Diagnostics.

**Threat model:** LOCAL_UNPRIVILEGED project/control configuration (malformed or oversized OSC address) and unreliable remote UDP destination, not a claimed remote code execution vulnerability. User-controlled project settings can exercise this path. Other queues and output engine remain out of scope.

## Scope, hypothesis and alternatives

**Hypothesis:** validating the computed OSC float packet bound **before** queue acceptance, and accounting for worker send failures, will provide actionable local diagnostics with no change to valid packet bytes, thread ownership, audio callback or JSON v10.

- **Chosen:** use a precise UTF-8 byte bound for a single-float OSC address under the fixed 1024-byte packet cap; preserve existing rejected-request semantics (not counted as queued drops); count a failed/oversized worker send as a drop. Include independent Control Mapping OSC counters and an overall local total in Diagnostics.
- **Rejected:** duplicate full packet encoding on the UI thread and again on the worker (additional work/allocations with no value).
- **Rejected:** a new transport abstraction/logger or an OSC authentication overhaul (out of scope). Sender authentication belongs to separate security RQ.
- **Red team:** multibyte path whose UTF-8 length exceeds byte budget despite shorter character count; invalid host/port/address and NaN; high output rate/queue saturation; failing UDP destination; PANIC/clear; valid loopback transmission; don't equate UDP write with remote receipt.

## Acceptance criteria and evidence gates

- **AC-1:** `sendOscFloat` rejects an OSC float address that would make a packet > 1024 bytes, including multibyte UTF-8; a rejected request does not claim enqueue success or change queued-drop count.
- **AC-2:** the OSC worker increments a local failure counter for an attempted packet not dispatched successfully (encoding/size or UDP write fail); accepted and successfully written control messages increment the sent counter.
- **AC-3:** Diagnostics show Control Map OSC and Timeline OSC counters separately and local totals (all accounting from actual existing counters; no imaginary receiver ACKs).
- **AC-4:** existing mapping, GO/STOP/PANIC, MIDI/OSC Learn, Timeline OSC and schema .cuelist v10 behavior remain unchanged. Entire Windows Release/CTest suite succeeds on exact head SHA.
- **AC-5:** VERSION.txt, CMakeLists.txt and CHANGELOG.md align; branch/PR only; no automatic merge.

| Gate | Method | Result |
| --- | --- | --- |
| TDD RED | Windows Release/CTest on 18acbd9b: 420920 assertions, exactly one failed test at oversized packet acceptance. [Run 44711085424](https://github.com/Askeladd22/The_Cuelist/actions/runs/44711085424) | FAIL AS EXPECTED |
| TDD GREEN | Windows Release/CTest on ee5a7ff7: 421017 assertions, zero failed tests; both oversize rejection and 1024-byte loopback tests PASS. [Run 44711908286](https://github.com/Askeladd22/The_Cuelist/actions/runs/44711908286) | PASS on tested code SHA |
| VERSION | Version/changelog validator PASS for ee5a7ff7, Windows build same commit, [run 44711908605](https://github.com/Askeladd22/The_Cuelist/actions/runs/44711908605). Final documentation-only commit requires separate CI verification. | PASS on tested code SHA; final SHA PENDING |
| REVIEW | Selected source, test and PR diff manually reviewed. No CodeQL/ASan/scanner; manual Trail of Bits-inspired C++ checklist below | SCOPED PASS; automated coverage ND |
| HARDWARE | Real OSC receiver, live mapped controller, UDP delivery/failure and audio/MIDI/LTC bench; native Diagnostics UI screenshot at 100/125/150% | ND / NOT RUN |

## Manual C++ review and QA report (selected surfaces only)

**Inspected at code SHA ee5a7ff7:** `src/ControlOutputRouter.cpp/.h`, `src/ControlRuntime.cpp`, `src/ProjectLifecycle.cpp`, `src/OscClip.cpp`, `tests/ControlMappingV10Tests.h` and the PR diff. Threat model LOCAL_UNPRIVILEGED configuration / unreliable network delivery. Original Trail of Bits automation **not executed**; coverage is manual and selected, not full-repository certification.

| Check | Evidence and outcome |
| --- | --- |
| Bounds / sizeof / signedness | Fixed OSC float payload has padded UTF-8 address + 8 bytes; address <=1015 bytes yields <=1024 encoded bytes. Worker also checks packet size before the `int` socket-length cast. PASS by review and both boundary tests. |
| String termination / input validation | `address.toRawUTF8()` NUL-terminated UTF-8 length is used; slash, finite float, host and port preconditions retained. Oversized ASCII and multibyte paths rejected without enqueue. PASS by tests. |
| Resource ownership / initialization | Worker starts after member initialization; protected deque remains capacity 512; constructor/destructor and socket remain worker-owned. Unchanged; manually reviewed. |
| Locks / concurrency / callback | Queue lock used only for short enqueue/dequeue; packet conversion/UDP I/O performed after lock release, never in audio callback. Unchanged; manual review only. |
| Return values / error handling | Failed or oversized worker send increments `dropped`, successful local UDP write increments `sent`; API-invalid requests remain rejected without queued-drop count. Worker failure branch reviewed, not fault-injection tested. |
| Macro/API surface / iterator safety | No macros, public format or iterator ownership changes; no new module. Existing Control Mapping dispatch remains on UI thread. |
| Negative controls / counterexample | An already dequeued Control Mapping OSC packet might escape a concurrent PANIC `clear()`; deterministic reproduction is **ND** and tracked in issue #211, not silently declared fixed. |

### CCteam QA verification matrix

| ID | Scenario | Actual | Status |
| --- | --- | --- | --- |
| OBS-01 | Oversized ASCII OSC address accepted before fix | Fails as expected on red SHA, PASS rejection on green SHA | PASS (RED/GREEN) |
| OBS-02 | UTF-8 multibyte address over 1024 bytes | Rejected on green SHA | PASS |
| OBS-03 | Maximum valid 1024-byte float packet | Byte-exact UDP loopback and local sent counter | PASS (loopback only) |
| OBS-04 | Invalid host, port, address, infinity | Existing regressions run in CTest | PASS |
| OBS-05 | UDP write failure and missing receiver | Error accounting statically reviewed; remote loss unmeasured | ND |
| OBS-06 | Native Diagnostics layout and 100/125/150% DPI | GUI visual inspection not performed | ND |
| OBS-07 | GO/STOP/PANIC and .cuelist JSON v10 | Full CTest on ee5a7ff7 passed; physical hardware not used | PASS simulated / hardware ND |
| OBS-08 | CodeQL, ASan, memory/CPU profiling, real OSC receiver and LTC | No external scanner/hardware measurement | NOT RUN |

**Compiler note:** Windows Release emits MSVC C4456 at `src/Timeline.cpp:409` (unmodified source), independently tracked as debt. No claim of production readiness follows from CI.

## PAUL UNIFY (source commit), follow-up gate

PLAN accepted → APPLY complete at ee5a7ff7 → RED/GREEN evidence recorded. The code commit has a Windows Release/CTest PASS on the exact source SHA. Once this report is committed, the final PR head changes *only documentation* and must pass its own version guard and Windows CI before merge. There is no automatic merge. Rollback remains reverting this PR; external delivery, PANIC in-flight cancellation, DPI and hardware remain open.

## Rollback and residual risks

Revert this single PR; no project migration or hardware routing changes. Intentional UDP output cancellation by PANIC is not a transmission failure. Existing Diagnostics counters measure local send/write results, **not** remote delivery; downstream latency and receivers remain ND. Release gates cannot declare LIVE readiness from CI alone.
