# GATES — N5.4 Audio column silence annotation and dB/dBFS clarity

Baseline verified main@442e4bfcc4750804b65e858ce2ed647c6d50b02e,
official VERSION.txt=0.9.74, schema .cuelist JSON v10.
Tracking issue #202 and draft branch fix/n54-column-silence-annotation-v0975.
Version proposed 0.9.75. No automatic merge.

## Closure evidence for preceding N5.3 (not inherited checks)

- Windows Release/full CTest on merged N5.3 PR #201 head
  218ac20f36d716bdaec24d96c76e9698107ff935:
  [run 43604661755](https://github.com/Askeladd22/The_Cuelist/actions/runs/43604661755)
  PASS; 420842 assertions, 0 failed tests.
- Version/changelog guard: [run 43604661760](https://github.com/Askeladd22/The_Cuelist/actions/runs/43604661760) PASS.
- Actual JUCE screenshot artifact ID 14117208839 examined and compared
  to baseline 0.9.73 at 1180x820 and 1647x895. The Master panel region
  had 0 changed pixels; Audio strip changed by 221 pixels at both
  sizes. Earlier candidate placed -infinity too close to MUTE; final
  patch lifted the caption 11px and passed the N5.3 collision test.
- Residual human observation: that lift visually distances -infinity
  from the final tick. This is the purpose of N5.4, not evidence
  that N5.3's tests failed. No hardware/DPI/operator data acquired.

## N5.4 design and red-team gates

Hypothesis H-N5.4: a small informational connector, painted only
inside the actual JUCE Audio column scale, identifies the bottom
tick as the -infinity end-stop without moving the text, the tick,
the gain cap, the L/R peak rails, PAN or MUTE. The connector sits
outside the shorter intermediate (-48) tick and reaches exactly
the lifted caption, without creating a fake signal meter.
Column gain tooltip must distinguish gain in dB (fader position)
from L/R signal peaks in dBFS (meters). The Master uses its legacy
default paint and its existing tooltip.

- [x] Gate A: confirmed main/version, read N5.3 source and exact
  native before/after screenshots; issue #202 authored and baseline pinned.
- [ ] Gate B: implement a minimal >=1px connector in columnReadableScale
  only; no mode/model/coordinate mapping changes.
- [ ] Gate C: restore explicit gain-vs-meter units in column tooltip,
  including wheel, Shift fine adjustment, reset, without altering
  the Master tooltip.
- [ ] Gate D: native tests validate actual column tooltip and Master
  default state; existing N5.3 endpoint/calibration/mute tests remain
  unchanged and pass on Windows CTest.
- [ ] Gate E: native snapshots from exact SHA visually reviewed at
  1180x820, 1647x895, 1920x1080. Verify connector does NOT join the
  -48 intermediate tick and that Master is pixel-identical to 0.9.74.
- [ ] Gate F: VERSION.txt, CMake project version and CHANGELOG 0.9.75,
  version/changelog guard success.
- [ ] Gate G: Windows Release/full CTest, screenshot artifact upload,
  0 failures on exact final commit SHA.
- [ ] Gate H: Matt Pocock standards/spec, GStack diff, Taste/Emil
  UI contrast and worst-case audit; no changes to transport,
  scroll/hitboxes, pan/mute, audio callbacks, routing or JSON v10.
- [ ] Gate I: native Windows DPI 100/125/150, Narrator/IME,
  operator A/B gain-vs-peak explanation, external audio/MIDI/OSC/LTC
  and latency hardware: ND/not run, not LIVE sign-off.
- [ ] Gate J: modal Esc/PANIC issue #191 remains independently open.

## Worst-case and rollback

At +6 and 0 ceiling, the final silence reference must remain
identifiable when gain is -infinity, +0, -48 or close to clipping.
At 1180x820 MUTE and PAN must remain readable; at 1647x895
the connector must not become a decorative line confusing with
an actual audio peak. In collapsed strip mode the label/connector
must not appear (existing compact rendering remains unchanged).
No synthetic screenshot substitutes for a Windows desktop DPI/IME
or external hardware test. A merge is NEVER automatic.
Rollback: revert the small v0.9.75 PR, no Show schema migrations.

Do not execute inherited CHECK scripts or untrusted shell hooks.
