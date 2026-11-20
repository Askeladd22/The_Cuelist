# RQ-SEC-001 — Bounded physical OSC control-edge state (0.9.81)

**Baseline:** `main@dc8d8597944ab42954bf3bcad8a9eb38de524367` (20 November 2026); `VERSION.txt` 0.9.80; project schema JSON v10.
**Method:** PAUL Plan → Apply → Unify; SEED Application/Deep; Trail of Bits C++ review (manual, bounded scope); Matt Pocock red-green debugging; CCteam QA; Unlazy gates. The installed workflows do not themselves run external scanners.
**Phase:** approved scoped PLAN; APPLY and checks must be recorded with evidence; UNIFY stays open until verification.

## Scenario, boundary and evidence

An operator has a held physical MIDI GO and an OSC GO binding while unrelated hosts send unique OSC addresses to explicitly enabled UDP listener ports. OSC decoder bounds a datagram to 1024 bytes and an address to 127 bytes (`src/OscControlInput.h`), but `PhysicalControlEdges::recordAndWasDown` in `src/ControlDispatchPolicy.cpp` previously inserted *every* physical identity into a map, retaining false releases too. Since `MainComponent::dispatchControl` records even unmatched events before mapping resolution (`src/ControlRuntime.cpp`), a flood of unique OSC addresses could retain an unbounded number of keys on the GUI thread.

Threat model: REMOTE, when configured OSC UDP ports are reachable from untrusted network hosts; no authentication is provided. Local physical MIDI must retain GO continuity. This fix limits accumulation; it does not authenticate the UDP sender or prevent input congestion.

## Hypothesis, red team and alternatives

Hypothesis: at most 4096 simultaneously held OSC identities can be tracked, and new identities under saturation are treated as already-held for GO/PANIC safety. Previously registered held controls remain tracked, and release permits recovery. MIDI states remain independent of OSC capacity.

- Reject: indiscriminate clearing/eviction of held controls; it can re-arm a held GO during cue/Control Map transitions.
- Reject: track only currently mapped OSC inputs; a mapping can change while a control is held.
- Chosen: bounded positive-state set for OSC with fail-closed admission; a separate positive-state set for locally configured MIDI controls. Release does not reserve a slot. This may intentionally suppress a new OSC GO during saturation: operational availability is a known residual risk.
- No modification to network sender trust settings or protocol compatibility in this phase.

## Acceptance criteria and gates

- **AC-1**: 100,000 unique OSC presses cannot create more than 4096 simultaneously held OSC identities; no unbounded retention of released identities.
- **AC-2**: a held MIDI or OSC GO is never interpreted as a second rising edge after a flood or Control Map change.
- **AC-3**: new OSC presses at capacity cannot trigger a fresh GO/PANIC; release of a tracked identity frees a slot for a later legitimate press.
- **AC-4**: existing edge, unmatched-release, MIDI Learn / OSC Learn and ordinary mapping tests still pass on Windows Release/CTest.
- **AC-5**: VERSION.txt, CMake version and CHANGELOG align; .cuelist v10 unchanged; no audio callback I/O/allocations; no automatic PR merge.

### Test evidence ledger (fill only after actual execution)

| Gate | Verification | Status |
| --- | --- | --- |
| TDD-RED | Commit with regression test against pre-fix implementation; Windows CTest fails on capacity assertion | NOT RUN |
| TDD-GREEN | Windows Release/CTest on fixed PR SHA | NOT RUN |
| VERSION | CI version/changelog validator on fixed PR SHA | NOT RUN |
| STATIC | Review source, diff, tests and invariants on pinned SHA | NOT RUN |
| HARDWARE | OSC network flood with MIDI desk, physical GO/PANIC, DPI screenshots and LTC/audio bench | ND — NOT RUN |

## Rollback and remaining risks

Revert the single feature commit/PR without modifying stored shows or migrating JSON v10. If OSC saturation is detected in a live rehearsal, restore a trusted/control-isolated network and re-test controller operation before running the show. Operator must not infer delivery from UDP write success. Hardware timing, audio underrun under flood, and external receiver validation remain ND.
