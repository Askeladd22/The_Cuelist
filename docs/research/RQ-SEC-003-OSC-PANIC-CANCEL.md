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
| TDD RED | Windows Release/CTest on probe-only commit (expected stale packet received) | NOT RUN |
| TDD GREEN | Windows Release/CTest on fixed commit, exact SHA | NOT RUN |
| VERSION | Version/changelog validator, exact SHA | NOT RUN |
| STATIC | Manual C++ ownership/concurrency review, no automated Trail of Bits scanner | PARTIAL |
| HARDWARE | On-air mimic with hardware MIDI/OSC and external timestamps, multi-DPI screenshots | ND |

## Rollback
Revert this isolated PR; no saved-show conversion or migration. If new cancellation logic harms operator sequencing, do not merge; test the prior main version 0.9.82, and isolate devices in rehearsals. CI green never certifies physical-output synchronization.

**Methods:** PAUL Plan→Apply→Unify, SEED Application/Deep, Trail of Bits manual C++ review, Matt Pocock deterministic red-green, Simplify Codebase focused survey, CCteam QA and Unlazy gates. No original external skill automation executed.
