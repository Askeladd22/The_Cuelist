# RQ-SEC-003 — PANIC and already-dequeued Control Map OSC packets

Date: 2026-11-20. Baseline: `main@aff8b4881e6bd4edcf309fe001c40f4fe45b94c1`; VERSION.txt 0.9.82; feature 0.9.83. Project schema JSON v10.

## Scenario and verified code observations
The UI's `MainComponent::stopAll()` runs `engine.panic(); controlOutput.clear();`. In `ControlOutputRouter::run()`, the worker removes a packet from a queue under `queueLock`, releases the lock, encodes an OSC float and then calls `DatagramSocket::write`. `ControlOutputRouter::clear()` currently only clears the queue; it does not mark an extracted packet obsolete.

**Hypothesis:** a packet already extracted but not yet submitted to the OS may escape PANIC and trigger a remote device. This timing window follows from inspection; reproduce it in a deterministic UDP loopback test before claiming it confirmed. Threat model: controller hardware or OSC destination active during PANIC. Impact potentially HIGH for operators controlling show cues.

Official JUCE docs: [WaitableEvent](https://docs.juce.com/master/classjuce_1_1WaitableEvent.html) supports synchronization in the test; [Thread](https://docs.juce.com/master/classjuce_1_1Thread.html) documents thread termination and notification. These are library references, not evidence of a running hardware test.

## PLAN — design options and rejection criteria
- **Reject** blocking `clear()` until socket writes complete (PANIC can hang on unpredictable network I/O).
- **Reject** holding the queue lock during `DatagramSocket::write` (could block the message/UI thread).
- **Proposed** monotonic generation marker, assigned under `queueLock` at queue admission. `clear()` increments marker and clears queue. Worker checks generation after encoding, directly before the write. Add a narrow optional pre-write test probe used only by a Windows regression harness; no callback used for production. After a PANIC, new generation packets still send normally.
- **Counterexample/residual risk:** if the OS UDP write has already started, it cannot be recalled; there remains a small check→write race. Do not claim "no OSC after PANIC" absolutely.
- **Simplify Codebase:** keep existing worker and deque, no duplicate OSC transport, state machine, audio callback or architecture rewrite.

## Acceptance criteria and operator gates
- AC-1: a real worker packet paused after dequeue, then invalidated by `clear()` before its UDP write, must not arrive at a loopback receiver.
- AC-2: a new packet after `clear()` arrives on UDP loopback with the correct address; no persistent OSC mute.
- AC-3: `clear()` does not block on UDP I/O, and accepted valid OSC packets retain their encoded data and counters.
- AC-4: Windows Release/CTest on exact fixed SHA passes existing GO/STOP/PANIC, Control Maps, Audio/MIDI/OSC/LTC and JSON v10 regressions, plus deterministic test; VERSION.txt / CMake / CHANGELOG aligned.
- AC-5: No automatic merge. Native screen DPI, real-controller and external receiver/jitter tests are separately ND until performed.

## RED-team / QA
Arrange UDP loopback on a port allocated by the OS. The new test hook pauses the actual `ControlOutputRouter::run()` after dequeue + encoding but before the irreversible write, using `juce::WaitableEvent`. Act: call `clear()` from the caller thread while worker is paused, then resume. Assert: stale packet not received, new packet received after clear. The first CI run should fail on the original production behavior. The hook is a deterministic test seam, not a replacement UDP mock. Negative controls: real output from `ControlOutputRouter`, packet size and previous routing tests. Hardware bench must separately test late packets and timing under load.

| Gate | Evidence | Status |
| --- | --- | --- |
| TDD RED | [Windows CI run 44715359091](https://github.com/Askeladd22/The_Cuelist/actions/runs/44715359091), commit d59293945ee13ce43871e7e24aa819c7bb19b602: 420932 assertions, one failed test `A Control Map OSC packet was transmitted after PANIC clear()` | FAIL AS EXPECTED — reproduced |
| TDD GREEN | [Windows CI run 44715912712](https://github.com/Askeladd22/The_Cuelist/actions/runs/44715912712), fixed code commit b48865d05fbc5f24df4b499aa5c933a47c9b3b18: 421024 assertions, 0 failures; exact target test PASS, 100% CTest PASS | PASS on code SHA |
| VERSION | Both runs executed the version guard and checked version 0.9.83 against CHANGELOG/CMake; final documentation-only commit must be rechecked on its SHA | PASS on code SHA; final SHA pending |
| STATIC | Manual Trail of Bits-inspired C++ review of selected source/test diff. No static analyzer, CodeQL, AddressSanitizer, ThreadSanitizer, or original Trail of Bits automation | REVIEWED, automated coverage ND |
| NATIVE CI | Windows screenshot artifact upload step succeeded. Actual visual inspection of images and physical DPI 100/125/150 has not been performed | Artifact step PASS, UX/DPI ND |
| HARDWARE | External hardware OSC/MIDI/audio/LTC, latency and jitter timestamp bench, actual PANIC on show equipment | ND — NOT RUN |

## Trail of Bits C++ manual review (scoped, not a scanner)

Compared changes in `src/ControlOutputRouter.h/.cpp`, `tests/ControlMappingV10Tests.h` with `MainComponent::stopAll()`, existing `OscOutputRouter::run()` and `OscStream::epoch`. Threat model: an operator or mapped OSC source causes output while PANIC interrupts an active show. All statements below concern the inspected code at commit b48865d0; no complete repository coverage is claimed.

| Area | Finding |
| --- | --- |
| Buffer-write and sizeof | No packet buffer or serialization changed. Worker retains existing 1024-byte bound and prior OSC encoding tests. |
| Integer/signedness | Added 64-bit monotonic `epoch` to Packet and atomic `cancelEpoch`. Wraparound is theoretically possible only after 2^64 invalidations; not meaningfully tested. |
| Ownership / lifetime | Packet owns its `juce::String` values; the worker owns its socket. Optional injected `beforeWriteProbe` is test-only, empty in normal production; test's wait events outlive router destructor. |
| String termination | No new C strings, parsing, or UTF-8 conversions in the patched worker; prior OSC validation remains intact. |
| Error paths | A pre-write canceled packet intentionally skips UDP write and does not increment the UDP send-failure counter. Previous queuing and local sent/dropped counters remain. |
| Calling preconditions | `clear()` is called by `MainComponent::stopAll()`; ordinary constructors provide no test callback. `sendOscFloat` and `clear` serialize epoch assignment/invalidation under the existing queue lock. |
| Concurrency and RAII | The worker checks its packet epoch with an acquire load after encoding. `clear()` increments epoch under the mutex with release order and returns without waiting on network I/O. No lock added around UDP writes or in audio callback. |
| Iterator safety / init / macros | Queue operations remain under `juce::CriticalSection`; no iterators stored across unlocks and no new macros or dynamic lifetime ownership. |
| Counterexample | A UDP write already started before the epoch check or a check→write race cannot be recalled. Therefore **no absolute guarantee** of zero physical output after PANIC. Real downstream tests remain ND. |
| Architecture comparison | The Timeline's separate `OscOutputRouter` already uses `OscStream::epoch` to invalidate stale stream messages. The Control Mapping worker adopts the same principle without introducing a shared abstraction across unlike threading paths. |

**Reviewer verdict:** confirmed defect in the pre-write controlled case; tested fix PASS on code SHA. **LIVE readiness remains BLOCKED on hardware gates**. No guarantee of transport synchronization is inferred.

## CCteam QA / Unlazy acceptance

| ID | Check | Evidence | Status |
| --- | --- | --- | --- |
| AC-1 | Old pre-write dequeued packet suppressed after PANIC clear | Deterministic WaitableEvent+real UDP loopback, RED→GREEN on exact commits | PASS |
| AC-2 | New mapped OSC output works after PANIC clear | Same green regression checks a new address on loopback | PASS |
| AC-3 | Nonblocking `clear()`, preserved counters and packet bytes | No network I/O under UI lock; existing loopback/invalid OSC regression CTest PASS; no independent latency benchmark | PASS simulated / timing ND |
| AC-4 | Full Windows Release/CTest, schema compatibility and functionality | 421024 assertions, 0 failures on b48865d0; no serializer/audio callback edits | PASS automated / hardware ND |
| AC-5 | Versioning and PR isolation | VERSION.txt/CMake/CHANGELOG 0.9.83; draft PR #213, not merged by this implementation | PASS on code SHA, final docs SHA to check |
| UAT-1 | External OSC target and MIDI controller in a real rehearsal | Not performed | NOT RUN |
| UAT-2 | Screenshot visual review 100/125/150% and operator feedback | CI screenshot artifact uploaded, not visually assessed | ND |
| UAT-3 | ASan/CodeQL/ThreadSanitizer and stress-jitter distribution | Not performed | NOT RUN |

## PAUL UNIFY — qualification and further gate

The test demonstrates a real **pre-write** race on original code; fixed code passes under controlled scheduling. Simplify Codebase survey: the optional one-callback test seam is justified by a deterministic regression; no new output router or redundant transport introduced. SEED Application/Deep gate: engineering slice complete at code level; hardware-level LIVE gate remains WARN/BLOCKED. The following PR-head is expected to modify **only** this research note and CHANGELOG.md; rerun version guard and Windows Release/CTest on that final SHA before treating it as the current QA baseline. Explicitly no automatic merge or rollback migration.

## Rollback
Revert this isolated PR; no saved-show conversion or migration. If new cancellation logic harms operator sequencing, do not merge; test the prior main version 0.9.82, and isolate devices in rehearsals. CI green never certifies physical-output synchronization.

**Methods:** PAUL Plan→Apply→Unify, SEED Application/Deep, Trail of Bits manual C++ review, Matt Pocock deterministic red-green, Simplify Codebase focused survey, CCteam QA and Unlazy gates. No original external skill automation executed.
