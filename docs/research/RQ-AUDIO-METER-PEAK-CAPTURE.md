# RQ-AUDIO-METER — Peak telemetry during brief Click WAV transients

Base `main@e464112dc8b89460f5e00a8e837bc80ab99e6237`; official app version 0.9.77; .cuelist v10.

## Operator scenario and measurable hypothesis
Operator plays a normal WAV click or another short transient. Audio is audible
but strip/Master peak bars occasionally remain dark because the UI reads only
the last completed callback (nominal GUI 25 Hz). H1: preserve the **highest**
L/R sample peak across audio blocks between UI reads, then atomically consume
that peak once, without changing actual audio or timer frequency.

## Verified source observations (pre-change)
- `RenderState::render()`: post-strip stereo PCM sum and max(abs(sample))
  over every sample of every normal (<=8192-frame) block.
- `ColumnPeakTelemetry::left/right.store`: overwrites each callback.
- `AudioEngine::processBlock`: separate Master pre-limiter last-block peak.
- `ColumnUtilities::refreshMeters`: polls peaks from UI nominally at 25 Hz.
- `GainMeterFader::setStereoPeaks`: dBFS conversion, decay/hold **after** polling.
  No GUI hold can recreate a transient already overwritten in telemetry.
- 8192+ frame oversize path approximates multi-voice sums; not changed in this fix.

## Acceptance and red team
- [ ] Red-capable Windows regression: one PCM pulse followed by silent callbacks
  **without a UI poll**: old implementation loses it.
- [ ] Green: strip/Master return max L/R over interval regardless of callback count,
  then return 0 on a second poll without new samples.
- [ ] Keep instantaneous `getColumnStereoPeak` / `masterPeakLeft/Right`
  behavior and test consumers; avoid destructive double-reader semantics.
- [ ] No allocation, locks, PCM DSP, waveform, device routing, or timer changes
  in callback; confirm 48 kHz/512 and other rates/buffer sizes.
- [ ] Thread interleaving: audio write concurrent with GUI consume must
  preserve peak in either current or next window, never silently discard it.
- [ ] Preserve column IDs on RenderState republish and no stale peaks after
  audio device disconnect/reconnect; verify on Windows.
- [ ] Windows Release, full CTest, UI/DPI screenshot, CPU/RT performance check,
  and physical multichannel audio test. CI pass != live-ready.

## Risk, constraints and rollback
Root cause of the user's specific visual symptom remains an informed code
diagnosis until instrumented Windows runtime confirms it. Sample-peak meters
are not event counters: multiple pulses within one interval display one max.
No alteration to Click WAV synthesis, schema, LIVE/VIEW or MIDI/OSC.
Rollback: revert this focused PR; PCM remains unchanged and user WAVs persist.
