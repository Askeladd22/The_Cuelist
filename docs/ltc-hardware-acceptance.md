# LTC Audio — independent hardware acceptance checklist

This document applies to **generated LTC Audio**, not a Cuelist-wide timecode transport. Complete the checks with a real audio interface and an **independent LTC decoder or compatible receiver** before using LTC to control show equipment. Cuelist CI cannot substitute for that test.

## Safe bench setup

1. Use a test show with one empty Audio cell and a dedicated physical output pair. Set the column to **Direct Out**, disable **Master Send**, and avoid the Master pair and any audience speakers or headphones.
2. Connect only the intended audio interface **line output** to the external LTC decoder's supported input, using the correct electrical level and cabling. Start with output level low; calibrate to the receiver's recommended input range without clipping.
3. Keep clip gain, row gain, column pan/phase and output gain fixed; avoid DSP, enhancements, automatic gain control, mono summing with other content, loop mode, time stretching and sample-rate changes during playback.
4. In Audio Devices, verify the active output hardware channels and sample rate. The generator WAV is always **mono PCM 48 kHz / 16-bit**. Document whether the device plays at 48 kHz or resamples (for example 44.1 kHz).

## Test matrix

Generate a new track for each configuration. Start the track in the cell using ordinary play or GO, and record the first frame, frame counts, continuity, drop flag and final decoded frame from the external receiver.

| Rate | Starting timecode | Critical expected transition |
| --- | --- | --- |
| 24 fps | 01:00:59:22 | 01:00:59:23 → 01:01:00:00 |
| 25 fps | 01:00:59:23 | 01:00:59:24 → 01:01:00:00 |
| 29.97 drop-frame | 01:00:59;28 | 01:00:59;29 → 01:01:00;02 |
| 29.97 non-drop | 01:00:59:28 | 01:00:59:29 → 01:01:00:00 |
| 30 fps | 01:00:59:28 | 01:00:59:29 → 01:01:00:00 |

At **29.97 drop-frame**, also verify that the tenth-minute boundary (e.g. 01:09:59;29 → 01:10:00;00) does **not** skip labels. Frame labels, not source audio frames, are skipped; the exact frame clock is 30000/1001 for both 29.97 modes.

## Audio-engine acceptance

- Verify playback on the intended Direct Out channels with no signal on the Master output. Do not trust meters alone: check the physical patch.
- Verify start, Stop and Panic. During live playback, test a row GO with another Audio cue and selective Auto Stop; document whether it stops LTC as configured. Avoid looped LTC for show synchronization unless specifically tested.
- Test at 48 kHz and, if required by the venue, the actual device rate and block sizes. Check decoder lock and frame continuity through a complete selected duration; short test files do not prove 10-minute stability.
- Save and reopen the `.cuelist` project and verify the generated WAV still resolves, plays and routes properly. Copy the generated media when moving the project.
- Record device model, driver, firmware, OS output format, channels, gain setting, cable, receiver, frame rate, test duration, observed drift/dropouts, and pass/fail.
- Do not certify a setup if the decoder loses lock, reports wrong flags/frame numbers, skips or duplicates unexpected frames, or if the signal reaches monitors.

## Automated versus physical evidence

CI checks generated PCM BMC data, time address, sync, drop-frame flags, and Direct Out rendering. It **does not** run external physical receivers, verify cable levels, end-to-end clock stability, or promise sample-accurate synchronized show-control timing. Each venue/receiver configuration requires an independent acceptance record.
