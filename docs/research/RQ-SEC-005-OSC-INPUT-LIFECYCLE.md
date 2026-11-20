# RQ-SEC-005 — Cooperative OSC input thread shutdown (v0.9.85)

**Baseline:** `main@60671f8cc0c818ee5186e1d5742ca5ddbecc03bb`, VERSION.txt 0.9.84, .cuelist schema JSON v10. **Target:** 0.9.85. **Scope:** `src/OscControlInput.h`, focused `tests/ControlColumnTests.h`, version metadata only.

## Scenario / risk for an operator

A show operator may disable OSC control input or change UDP ports during a rehearsal or LIVE session. `OscControlInput::stopListening()` previously called `signalThreadShouldExit(); stopThread(1000); sockets.clear();`. JUCE documents that an expired timed `stopThread` can forcibly kill a worker, potentially while locks or native I/O are held. The return was ignored. With up to sixteen sockets, worker `run()` polls `waitUntilReady(true,10)` and nonblocking `read`; no deterministic timeout or forced kill has been observed in this project so far. **This is preventive lifecycle hardening, not a reproduced crash fix.**

**Official sources:**
- JUCE Thread::stopThread(int), stopThread() and startThread() return contracts: https://docs.juce.com/master/classjuce_1_1Thread.html
- JUCE DatagramSocket::shutdown: https://docs.juce.com/master/classjuce_1_1DatagramSocket.html — closes underlying socket and aborts blocking operations without destroying its C++ owner while concurrent reader may still use it
- Repository source `src/OscControlInput.h:73-164` and Settings apply `src/ControlRuntime.cpp:29-80`, inspected at pinned main baseline.

## PAUL / SEED scope and hypothesis

**Hypothesis (measurable):** signalling termination, shutting down each native UDP socket *before* joining the receiver and destroying objects *after* joining removes the JUCE forcibly-killed-thread path. Windows CI should complete the 16-port stop within 3000ms under normal runner load, then receive a fresh OSC datagram after restart. A rejected configuration must preserve the working listener; a true post-bind failure must still use the existing restoration branch, although that rare path is not deterministically fault-injected here. Failure to launch the receiver thread must not be reported as success.

**Chosen:** keep existing single `OscControlInput` worker and `std::vector<std::unique_ptr<DatagramSocket>>`; call socket `shutdown()`, then `stopThread(-1)` (cooperative indefinite join), then free sockets and clear read/write cursors. Check `startThread()` result and revert temporary binding state if it fails. **No additional mutex or new thread model**. Simpler than a second event loop and consistent with JUCE shutdown API.

**Rejected:** retrying `stopThread(1000)` or ignoring a false result; both may kill the worker and then release resources the worker held. **Rejected:** destroying sockets before join. **Rejected:** intentionally blocking a test callback for >1000ms to force a thread kill in CI. **Rejected:** replacing the realtime audio/OSC Timeline paths or creating a new network subsystem.

**Counterexample / red team:** `stopThread(-1)` can wait indefinitely if native socket shutdown or thread code fails to return; there is no hard upper bound. This is an availability tradeoff requiring physical Windows and network-fault measurement; no claim of bounded worst-case stop latency from a CI threshold.

## AC and test gates

- **AC-1:** After a live 16-port listen, stop shuts down the sockets and joins the worker before freeing resources, with no timed force-kill in the implementation.
- **AC-2:** A stopped listener reports no active ports; a later start receives the correctly decoded OSC value on a real 127.0.0.1 UDP socket.
- **AC-3:** Invalid reconfiguration with duplicate ports is rejected without discarding the working listener. A failed thread start cannot claim success by code inspection; a real OS bind failure and restoration require separate fault injection.
- **AC-4:** Windows Release/CTest, old OSC Learn and mapping tests, CodeQL, VERSION/CMake/CHANGELOG guard pass on the exact PR head; JSON v10 unchanged; no audio callback changes.
- **AC-5:** stop duration measured on the Windows CI runner for 16-port normal operation, with a CI safety threshold of 3000ms, but hardware p95/p99 and blocked native I/O behavior remain ND.
- **AC-6:** draft small PR, no automatic merge; rollback by reverting the PR, no .cuelist migration.

| Gate | Method | Status |
| --- | --- | --- |
| Source review | Manual Trail of Bits-inspired lifecycle/concurrency review of OSC input, UI caller and test fixture | INSPECTED |
| TDD RED | Deliberately force a >1000ms native wait to reproduce JUCE kill | NOT RUN — unsafe test scenario, not required to characterize the documented API risk |
| Functional regression | Real UDP loopback sixteen-port listener, stop/rebind, rejection of duplicate ports, decoded readback | NOT RUN |
| Test premise check | Runs 44728131774 and 44728365029 failed because a second Windows UDP bind succeeded despite a held port; invalid assumption in test | TEST DESIGN CORRECTED; rerun needed |
| Windows Release/CTest | GitHub workflow at exact PR SHA | NOT RUN |
| CodeQL | C++ security-extended analysis at exact PR SHA | NOT RUN |
| Version guard | GitHub version/changelog check at exact PR SHA | NOT RUN |
| Hardware | Real MIDI/OSC desk, long runs with stop/quit while UDP floods, p95/p99/max stop delays, audio/LTC | ND |
| Rollback | Revert single PR, without project conversion | AVAILABLE |

### Counterexample from real Windows CI

The original regression expected `DatagramSocket::bindToPort` to fail when another UDP socket held the same port. The Windows runner proved that assumption false: [run 44728131774](https://github.com/Askeladd22/The_Cuelist/actions/runs/44728131774) and [run 44728365029](https://github.com/Askeladd22/The_Cuelist/actions/runs/44728365029) failed specifically with `OSC reconfiguration succeeded despite a port already occupied`. JUCE [setEnablePortReuse](https://docs.juce.com/master/classjuce_1_1DatagramSocket.html) and Microsoft [Winsock port reuse](https://learn.microsoft.com/windows/win32/winsock/using-so-reuseaddr-and-so-exclusiveaddruse) document platform-dependent binding semantics. The test now asserts rejection of a duplicate port list, which is unambiguously invalid by existing application rules. This PR **does not change the port-sharing behavior**. Explicit exclusivity/security policy must be considered separately; real post-bind failures require fault injection, not simulated port ownership.

### QA, test trace and review limitations

Matt Pocock feedback loop uses a concrete loopback and a repeatable 16-port stop; cannot claim RED/GREEN for an unobserved fault. CCteam QA distinguishes simulated/network-loopback from an external show control receiver. Trail of Bits C++ review here is **manual selected-file coverage**: worker-owned sockets, lifetime join, input byte limits 1024, no new callback allocations/locks, no new integer conversions; no upstream analyzer script executed. Simplify Codebase: reuse original worker and remove the forced-kill lifecycle edge; no invented layer. Unlazy: all gates are typed PASS/FAIL/ND and report exact SHA.

**Operator recommendation:** use isolated network, rehearse changing OSC port configuration, keep independent physical/MIDI PANIC, don't regard CI green as LIVE-safe. The branch must not merge automatically.
