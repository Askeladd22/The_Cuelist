# LIVE playback and MIDI Output change — Windows acceptance checklist

This plan covers requirements 1–3 in the post-implementation specification. It is a **manual hardware acceptance plan**, not a claim that the tests have run. Use real Windows x64 audio/MIDI interfaces and an OSC receiver. Keep backup show files and a working emergency stop.

## Preparation

1. Connect an audio interface and, if possible, two individually addressable MIDI Output devices (A and B). Record their exact identifiers and firmware.
2. Build the current branch in Release and run CTest first. No portable packaging is required.
3. Create a Show with a long (at least 60 s) recognizable WAV/MP3 in row 1, two MIDI columns with timed short notes and an OSC column. Use a second cue with distinct content; leave a third cue blank.
4. Open Diagnostics for output counters. Start an external timer and monitor audio continuously. Record cue acknowledgement, elapsed playhead, MIDI note/CC events captured at A and B, and dropped packet counters. Use at least 48 kHz / 256 frames; repeat on your actual performance interface/buffer.

## LIVE structural edits

| Operation while row 1 is audibly playing | Expected behavior |
| --- | --- |
| Add a cue after the currently armed cue | Audio uninterrupted; current cell seconds monotonic; GO remains armed to the same **cue ID**, not an accidental new index |
| Add an Audio, MIDI, OSC or Control column | No PANIC; no loop restart; current clip remains playing |
| Rename the active cue and a column | No transport reset or audible drop |
| Move a cue above/below the armed cue, using drag and arrow/shortcut | Transport continues; next GO still targets its original cue ID |
| Reorder two columns, including one with active audio | Active audio position preserved and output remains correct |
| Select another Cuelist and return | VIEW changes, LIVE remains in place with unchanged playback and armed GO |

**Destructive cases intentionally retained as safety boundaries:** deleting a currently running row/column or the LIVE Cuelist, or changing an active cue's tempo map. Do not interpret these cases as approved for seamless editing until a transport-state migration policy is designed and tested. No automatic substitute cue/column is chosen.

## MIDI Output acceptance matrix

| Case | Action | Pass criteria |
| --- | --- | --- |
| 1: MIDI unassigned | Start the Show with no MIDI Output in a MIDI column | Audio plays; cue remains acknowledged; MIDI skipped; no modal blocking alert |
| 2: Select during GO | While audio is playing, select device A as MIDI Output | Audio does not restart; MIDI events after selection appear on A at the current cue time |
| 3: A → B hot swap | While scheduled notes are pending, switch from A to B | Already queued A packets do **not** fire after switch; held A notes are released; subsequent MIDI is delivered to B; audio untouched |
| 4: Sudden unplug | Unplug selected device B mid-cue | Audio, OSC and cue clock continue; MIDI for unavailable B is dropped without blocking; no failover to A |
| 5: Return of B | Reconnect the same exact device identifier | Subsequent output reaches B after recovery; no GO retrigger or unexpected catch-up from an earlier time |
| 6: Disabled output | Disable/enable B in Settings while playing | Only MIDI B is muted/recovered; audio unaffected |
| 7: Empty selection | Switch MIDI Output to None during GO | MIDI ends safely on old route; audio and cursor do not reset |

Check long MIDI notes crossing the device switch and loop boundaries, and repeat with MIDI events densely spaced near an audio block boundary. Look for stuck notes, late old-route packets, duplicate Note On, and inadvertent All Notes Off on unrelated ports.

## MIDI queued-event policy

- On changing output, the retired stream's epoch is incremented and the worker stops its held notes on the **old** port. Already scheduled packets using the old epoch are discarded.
- The new RenderState inherits clock, cursor, paused/playing state and row-mode from the matching cell ID and MIDI content fingerprint. MIDI notes are re-synchronized on the **new** port at the current position; unrelated audio is not restarted.
- A disconnected exact output ID is retained in the Show. Messages cannot be silently redirected to another physical device. Once that exact ID appears, it may be reopened and the live snapshot republished without panic.
- A UI/project edit that actually **deletes active playback material** is a separate, not-yet-approved migration problem and retains the pre-existing safety boundary.

## GUI inspection

At 1180, 1440, 1647 and 1920 px wide, on Windows DPI 100/125/150%, verify:
- MIDI Piano Roll **Collapse / Detach / Close** are three recognizable 28×26 px vector buttons, separated by 4 px and anchored to the right at docked sizes.
- Detached MIDI window shows the return-to-dock icon and correct action; close/collapse remain functional when docked.
- The Timeline's header Collapse arrow hides the Timeline; the reopen grip remains visible near the lower workspace; View → Timeline mirrors the button state.
- Neither header overlaps mixer controls, transport buttons, tab headers, status footer or the pinned BPM/Auto Stop area.

## Report

Attach Windows version, audio interface/driver, buffer, sample rate, exact MIDI device identifiers, test Show (with media redacted if needed), failed matrix rows, relevant screenshots and CTest output. **Automated CI cannot substitute for physical unplug/reconnect tests.**
