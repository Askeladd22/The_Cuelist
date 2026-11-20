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
| TDD RED | New deterministic oversized-address test on the original router through Windows Release/CTest | NOT RUN |
| TDD GREEN | Windows Release and full CTest on corrected commit | NOT RUN |
| VERSION | scripts/check_versioning.py and GitHub version guard on exact PR SHA | NOT RUN |
| REVIEW | Review changed paths, packet size calculation, worker thread and old tests | NOT RUN |
| HARDWARE | Real OSC receiver, live control-mapping sequence, UDP loss/disconnect, MIDI desk and audio/LTC bench | ND |

## Rollback and residual risks

Revert this single PR; no project migration or hardware routing changes. Intentional UDP output cancellation by PANIC is not a transmission failure. Existing Diagnostics counters measure local send/write results, **not** remote delivery; downstream latency and receivers remain ND. Release gates cannot declare LIVE readiness from CI alone.
