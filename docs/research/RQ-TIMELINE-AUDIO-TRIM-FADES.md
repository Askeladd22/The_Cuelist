# RQ-TIMELINE-AUDIO-TRIM-FADE (v0.9.79)

Base verified main@e9a73179b25f19529db7812f61d67b1bd992c409;
official VERSION.txt 0.9.78, project JSON v10.

## Scenario and contracts
Operator manipulates normal WAV/MP3 Audio clips directly in real JUCE Timeline.
Lower edges resize; top corner dots adjust fades; body drag still moves.
Do not edit original WAV, change LIVE GO ownership, or create DAW/video support.
Music Snap quantizes trim/move, fades are continuous in seconds. Audio region
is [source offset, source offset + duration); left trimming moves timeline start
and source offset together while keeping right end. Right trim never extends
past loaded media. Fade-in and fade-out are linear in PCM source time.
Loops replay the trimmed region and ramps. Very narrow clips require zoom.

## Sources and architectural check
- src/Timeline.cpp and Timeline declaration in src/MainComponent.h: native
  track painting/mouseDown/mouseDrag/hover; MIDI/OSC right-edge grip exists.
- src/CellOperations.cpp, src/MidiUI.cpp: prior only MIDI/OSC duration editing,
  PCM duration was always the full file.
- src/AudioEngine.cpp: Voice renders preloaded PCM with linear interpolation,
  gain, column and Master/Direct sends, sample-peak telemetry.
- src/ShowModel.cpp: optional cell metadata within schema v10; older shows
  must default to full media and no fades.
- src/ShowHistory.cpp: Undo actions can panic for real structural changes;
  pure Audio trim/fade restore must preserve LIVE.

## Hypothesis / acceptance gates
H1: snapshot-only preview with a single mouseUp model commit prevents frame-by-
frame render-state rebuilding, and a source-relative Audio region plus pre-bus
gain ramp implements both fades without touching the file.

- [ ] Drag lower left edge -> source offset and cue start advance; end fixed.
- [ ] Drag lower right edge -> duration changes; no extension beyond source.
- [ ] Native resize cursor over lower edges and crosshair over top dots.
- [ ] Fade-in/fade-out handles render envelope; start/end and media unaffected.
- [ ] Full audio playback tests, loop, seek, undo/redo, old v10 defaults.
- [ ] Snap ON/OFF, zoom and short clips, MIDI/OSC/editor unchanged.
- [ ] Windows Release/CTest; native 100/125/150% DPI screenshots inspected.
- [ ] Rehearsal on real audio interface: no xrun / GO/STOP/PANIC regression.
- [ ] No changes to Click WAV synthesis, media files or schema version.

## Red team / operational safety
Trimming can uncover a source waveform discontinuity; the requested fade may
be used to smooth it, but no automatic waveform rewrite occurs. Overlapping
fade-in/out can prevent unity gain on short segments. Editing a sounding
Audio cell legitimately alters its range/envelope; unrelated voices are
kept alive, but do not edit show-critical LIVE clips during performance.
A file replaced externally with a shorter version could invalidate existing
trim fields; the renderer clamps to available source frames (not OOB reads).
UI drag preview is visual only until release; no unbounded allocations/locks
in the audio callback. Rollback by reverting the feature branch PR and
retaining project/media backup; previous builds may ignore optional trim/fade
properties and play full files, so preserve working .cuelist backups.
