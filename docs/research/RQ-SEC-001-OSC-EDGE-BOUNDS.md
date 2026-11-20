# RQ-SEC-001 — Bounded physical OSC control-edge state (0.9.81)

**Baseline:** `main@dc8d8597944ab42954bf3bcad8a9eb38de524367` (20 November 2026); `VERSION.txt` 0.9.80; project schema JSON v10.
**Method:** PAUL Plan → Apply → Unify; SEED Application/Deep; Trail of Bits C++ review (manual, bounded scope); Matt Pocock red-green debugging; CCteam QA; Unlazy gates. The installed workflows do not themselves run external scanners.
**Phase:** approved scoped PLAN; APPLY and checks must be recorded with evidence; UNIFY stays open until verification.

## Scenario, boundary and evidence

An operator has a held physical MIDI GO and an OSC GO binding while unrelated hosts send unique OSC addresses to explicitly enabled UDP listener ports. OSC decoder bounds a datagram to 1024 bytes and an address to 127 bytes (`src/OscControlInput.h`), but `PhysicalControlEdges::recordAndWasDown` in `src/ControlDispatchPolicy.cpp` previously inserted *every* physical identity into a map, retaining false releases too. Since `MainComponent::dispatchControl` records even unmatched events before mapping resolution (`src/ControlRuntime.cpp`), a flood of unique OSC addresses could retain an unbounded number of keys on the GUI thread.

Threat model: REMOTE, when configured OSC UDP ports are reachable from untrusted network hosts; no authentication is provided. Local physical MIDI must retain GO continuity. This fix limits accumulation; it does not authenticate the UDP sender or prevent input congestion.

## Hypothesis, red team and alternatives

Hypothesis: at most 4096 simultaneously held OSC identities can be tracked, and new identities under saturation are treated as already-held for GO/PANIC safety. Previously registered held controls remain tracked, but the overflow latches until the operator explicitly re-applies or disables OSC input in Settings. MIDI states remain independent of OSC capacity and MIDI reconnect cannot clear the OSC latch.

- Reject: indiscriminate clearing/eviction of held controls; it can re-arm a held GO during cue/Control Map transitions.
- Reject: track only currently mapped OSC inputs; a mapping can change while a control is held.
- Chosen: bounded positive-state set for OSC with sticky fail-closed admission; a separate positive-state set for locally configured MIDI controls. Release does not reserve a slot. A rejected still-held OSC source cannot re-arm merely because a different source released. Operator re-arms explicitly through Settings; Diagnostics reports overload. This intentionally blocks new OSC GO under saturation: operational availability is a known residual risk.
- No modification to network sender trust settings or protocol compatibility in this phase.

## Acceptance criteria and gates

- **AC-1**: 100,000 unique OSC presses cannot create more than 4096 simultaneously held OSC identities; no unbounded retention of released identities.
- **AC-2**: a held MIDI or OSC GO is never interpreted as a second rising edge after a flood or Control Map change.
- **AC-3**: new OSC presses at capacity cannot trigger a fresh GO/PANIC; release of a different tracked identity cannot re-arm a rejected still-held source. Recovery requires an explicit successful OSC Settings Apply/Disable.
- **AC-4**: existing edge, unmatched-release, MIDI Learn / OSC Learn and ordinary mapping tests still pass on Windows Release/CTest.
- **AC-5**: VERSION.txt, CMake version and CHANGELOG align; .cuelist v10 unchanged; no audio callback I/O/allocations; no automatic PR merge.

### Test evidence ledger (fill only after actual execution)

| Gate | Verification | Status |
| --- | --- | --- |
| TDD-RED | Windows Release/CTest on dc34e436: regression fails only on capacity assertion, 420904 assertions, one failed test. https://github.com/Askeladd22/The_Cuelist/actions/runs/44707138683 | FAIL AS EXPECTED |
| TDD-GREEN | Windows Release/CTest on source commit 1209af6aae2465e91ecb951ea31bcd80bcca8bbb: 421008 assertions, zero failures. https://github.com/Askeladd22/The_Cuelist/actions/runs/44708146121 | PASS on source SHA; current PR SHA checked separately |
| VERSION | GitHub version/changelog guard passed on commit 754ee04fe2a200d85bb004eaa22e8216ae4e8a9f; build job also passed version guard on 1209af6 | PASS on listed SHAs; latest SHA checked separately |
| STATIC | Manual Trail of Bits C++ review of ControlDispatchPolicy, OSC/MIDI input lifecycle and PR diff; CMake/test/docs consistency checked. No automatic parser, CodeQL, ASan or full-repo audit. MSVC C4456 in unmodified Timeline.cpp:409 remains follow-up. | SCOPED REVIEW ONLY (automated coverage ND) |
| HARDWARE | OSC network flood with MIDI desk, physical GO/PANIC, DPI screenshots and LTC/audio bench | ND — NOT RUN |

## PAUL UNIFY — scoped evidence and residual gates

The changed code and deterministic tests were compared with the approved RQ acceptance criteria. The original 100000-address test failed on the pre-fix code (TDD RED) and passed on the fixed code (TDD GREEN) in real Windows CI. The implementation partitions held MIDI and OSC state, makes OSC overflow sticky, permits explicit operator re-arm, preserves `.cuelist` v10 and does not touch the audio callback. Since later documentation commits exist, **the GitHub checks on the eventual final PR SHA must still be consulted**; avoid promoting this intermediate source SHA to an unconditional LIVE claim. Manual UDP flood through an actual network, MIDI desk, DPI inspection and receiver LTC/audio bench remain ND. Branch and draft PR #210 are reversible and must not merge automatically.

## Rollback and remaining risks

Revert the single feature commit/PR without modifying stored shows or migrating JSON v10. If OSC saturation is detected in a live rehearsal, use independent MIDI/physical emergency controls, isolate the trusted control network, re-apply the OSC input ports in Settings to clear the latch, and re-test controller operation before running the show. A re-arm intentionally clears previously held OSC identities: release all OSC buttons before re-arming. Operator must not infer delivery from UDP write success. Hardware timing, audio underrun under flood, and external receiver validation remain ND.
