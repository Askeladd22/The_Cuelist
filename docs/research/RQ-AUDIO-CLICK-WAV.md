# RQ-AUDIO-CLICK-WAV — acceptance gates (0.9.77)

Verified base: main@a88b6fc639d831304134f607ec9132d5f7708600;
VERSION.txt 0.9.76; schema JSON v10. Feature branch:
fix/click-track-render-to-wav-v0977. No automatic merge.

## Operator scenario and hypothesis

Operator chooses Generate Click Track WAV in an empty Audio cell, possibly
while unrelated LIVE audio is running and while browsing another Cuelist.
Hypothesis H1: pre-rendering 48 kHz mono PCM WAV outside the audio callback
and committing only after normal loadClip and memory validation eliminates
the new procedural click voice path and preserves unaffected transport.
The reported crash is not reproduced locally; root cause remains ND.

## Gates / evidence

- [x] Inspect main SHA, ClickTrack/AudioEngine, MainComponent/dialog, v10
  model and ShowHistory, Audio import, LTC file writer, test harness and docs.
- [ ] 48 kHz 16-bit mono WAV: duration, onset/accent at 120 BPM, no overwrite,
  invalid configuration rollback. Windows CTest must execute these tests.
- [ ] New Click WAV occupies requested cell with file path, ordinary PCM map,
  Explorer reveal, v10 save/load, no new procedural click field.
- [ ] Create in dormant VIEW while unrelated LIVE plays: playback, next GO,
  viewed/active Cuelist and selection remain unchanged.
- [ ] Failed disk permission and 512 MB PCM budget: file cleanup and no
  overwrite/model change; dedicated fault-injection verification needed.
- [ ] Stress 0.1, 10, 600 s, 1–4 subdivisions, all timbres, tempo/meter changes;
  verify sampled onset drift within one 48 kHz sample, 600 s memory and UI
  responsiveness on Windows.
- [ ] CMake configure/build Release and Debug, full Windows CTest/ASAN where
  supported, original click crash reproduction, native UI test and audible
  multichannel routing test. No output or hardware test has been run here.
- [ ] Review all error cleanup and legacy v10 read/playback; compare
  documentation, schema, version and changelog; no merge until verified.

## Risk / red team / rollback

A long WAV is ~58 MB on disk and ~115 MB float mono in playback memory;
the existing per-file 128 MiB and per-show 512 MB limits still apply.
Writing on the message thread can temporarily stall the UI for long tracks;
bench 600 s and move to cancellable worker if operator deadlines demand it.
Generated media in Documents must be copied along with the show; a missing
WAV is now a missing-media preflight issue instead of a procedural source.
Tempo edits do not retime WAV, unlike old procedural click cells; explicitly
communicated in the dialog and user guide. Legacy v10 playback retained.
To roll back, close/reopen the prior version or revert this branch/PR; files
already generated are standard WAV and remain valid audio media. Keep project
backup and WAVs. NOT READY FOR LIVE until Windows/hardware gates pass.
