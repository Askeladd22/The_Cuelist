# The Cuelist — Complete User Guide

**Current prerelease:** `versione in [VERSION.txt](../VERSION.txt)` · **Windows:** x64 · **Project schema:** v10 · **JUCE:** 8.0.12 · **License:** proprietary

This English guide covers the software currently implemented in this repository. It replaces the legacy 0.8-alpha instructions. For an executive overview, see the [README](../README.md).

> **Important:** The Cuelist is beta software, not certified for show-critical use. Always rehearse with the actual audio interface, controllers and OSC receivers, run Preflight, check backups and maintain a tested emergency procedure. A green CI build is not proof of hardware reliability or end-to-end timing accuracy.

**Themes and appearance:** open **View → Appearance** or **Settings → Appearance** and choose **Legacy — Graphite / Copper** or **Industrial Orange**. Selection is applied immediately and remembered on the next application launch. Appearance belongs to local application settings, not to a Show file: existing cues, clip media and custom cue colour IDs never change.

**Industrial Orange colours:** structural graphite `#101619 / #182227 / #202C32 / #29373D / #303B40 / #35434A`; brand orange `#C66B42`. GO/PLAY is `#55B875`, PANIC `#E05A46`, WARNING `#E5A52E`, INFO `#55AFC5`. Audio is muted steel blue `#8FA8B2` (different from Info); MIDI is `#B79A72`, OSC `#A18BB9` and Control `#7186A6`. Clip bodies in Timeline show their type colour; selection uses a separate orange outline. Individually coloured cues have stronger body fills with independent selection rails and automatically contrasting labels. The seven saved cue colours are never rewritten. **Legacy** restores the preceding Graphite/Copper UI colours (v0.9.15).

**MIDI editor window actions:** three compact icon buttons sit at the right of the Piano Roll toolbar: Collapse (up/down chevron), Detach (pop-out window) and Close (X). They share the same size and spacing. The floating editor's Detach button becomes Reattach, using an inward arrow.

**Timeline Collapse:** click the arrow next to TIMELINE in its header to reclaim workspace space. When collapsed, the arrow stays accessible as a reopen grip at the bottom of the workspace; the View → Timeline menu toggles the same state and persists it across app launches.

**OSC editor gestures:** right-drag across messages or ramps to select several elements; a short right-click still opens the context menu. Double-click the interior of an OSC ramp to insert a control point (Snap applies); drag any point to change its value or time. The obsolete Ramp start/end-value dialog is removed. Track headers remain the sole place to edit the OSC address, multi-argument template, target argument index and parameter range. The target index is required to preserve correct multi-argument messages.

**Header Undo/Redo:** two curved-arrow controls appear immediately after Guide when the window is wide enough; on compact windows use Edit/More or Ctrl+Z, Ctrl+Y and Ctrl+Shift+Z. Their availability follows the shared edit history.

## 1. Concepts and project structure

A **Show** contains shared output columns, device and mixer configuration, Control Mapping settings, and one to twelve **Cuelists**. Every Cuelist has its own ordered cue rows, clip contents and technical notes. Adding cues/columns, renaming cues/columns or reordering existing cues/columns does not intentionally interrupt LIVE playback. The next GO follows the stable cue ID even if its row is moved. Selecting or changing a MIDI Output likewise does not issue PANIC: already playing audio and transport clocks continue; pending MIDI messages from the old route are invalidated, and the selected device receives subsequent messages. If a MIDI Output is unplugged, its events are skipped; the exact configured port may reconnect without automatically substituting another. Deleting a currently live cue/column remains a structural safety-boundary operation. Editing cue-local Timeline Markers or tempo changes does not interrupt LIVE playback; an active MIDI/OSC clip continues at its current absolute-time position while upcoming events are rescheduled.

Changing the viewed Cuelist does not stop the LIVE transport. A green dot on its tab identifies the LIVE sequence; GO continues there while other tabs are viewed. Explicit GO on a cue in another Cuelist intentionally takes over the live transport.

Each cue is a row; each column represents a kind of material:
- **Audio:** mono/stereo WAV or MP3 files with per-cell, cue and strip gain and playback settings.
- **MIDI:** note clips played via a selected hardware/software MIDI output, synchronized to the cue tempo map.
- **OSC:** timed UDP messages and numeric automation curves sent to a configured OSC destination.
- **Control:** MIDI input mappings associated with a cue or promoted to a column default; these are control rules, not media playback clips.

An individual **cell** can be played independently. **Row GO** triggers the participating cells together. Multiple rows may overlap until stopped manually or by an Auto Stop rule. BPM and Auto Stop remain in a pinned area of the cue grid.

## 2. Starting and configuring The Cuelist

### 2.1 Running an existing build

Extract the entire Windows x64 package. Open `The Cuelist.exe` or `Avvia.cmd`; the launcher does not compile or install software. A GitHub source-code archive does not contain a ready-to-run EXE.

A properly packaged build does not require Visual Studio, Git or CMake to run. Keep the included `LICENSE`, `NOTICE.md` and JUCE license notices with the package. The command-line option `--offline` starts the interface without opening the audio device and is intended for diagnostics and smoke tests.

### 2.2 First rehearsal

1. Open **Audio devices** in the left sidebar and choose the intended audio interface and output configuration.
2. Open an existing `.cuelist` or create a new show. Verify the media referenced by each cue.
3. Configure audio routing, MIDI ports and OSC host/port under the individual Column Utilities.
4. Enable the intended MIDI input under **Settings → MIDI Devices**; verify MIDI Learn after the physical device is connected.
5. Select **Preferences → Pre-flight**, read the warnings and resolve missing files, routes and devices.
6. Test cell Play/Stop, **GO**, **Panic**, loop and Auto Stop with the connected equipment before relying on the show sequence.
7. Save the show and make a separate external backup.

## 3. Workspace overview

### 3.1 Left sidebar

The sidebar provides **New**, **Open**, **Save**, the editable show title, **Settings**, **Control Mappings** and a project overview. Near the bottom, **On Deck** identifies the next cue; **GO / SPACE** and **PANIC / ESC** remain accessible at all times.

- **New** starts a fresh show and warns if current changes would be discarded. The compact **Unsaved changes** dialog keeps **Cancel** as the safe choice; **Discard and continue** must be selected explicitly to proceed without saving. Closing the dialog or pressing Escape also cancels the operation. To save instead, select Cancel and then Save.
- **Open** loads an existing project, subject to preparation and validation before activation.
- **Save** writes the project and collects referenced media.
- **Preferences** contains Pre-flight, Diagnostics, Fader Range and Rename active Cuelist.
- **Control Mappings** opens the shared native editor with Default Map selected. Clicking an assigned Control Column cell opens that same editor preselected on its saved or cue-local Map.

The shortcut hints formerly shown at the bottom of the sidebar have been removed; the keyboard shortcuts still work.

### 3.2 Top bar, Help, and operational telemetry

The top row contains named **Cuelist tabs**, the **+** action, **Undo/Redo**, a 24-hour clock, Windows CPU/RAM usage and a compact MIDI RX/TX / OSC RX/TX / audio-availability monitor.

The **Help** button in the top toolbar opens a menu with **Guide** and **About The Cuelist**. The Guide entry opens the content of the local `docs/complete-user-guide.md` **inside The Cuelist**, in a separate non-modal, resizable, scrollable window with headings and a built-in search. No external browser or Markdown editor is launched. The reader uses the bundled Markdown file, which is copied beside the compiled EXE and into portable packages, and reloads it each time the Guide is opened. About opens a compact, grouped native dialog with the version derived from `VERSION.txt`, the two credited co-authors **Davide Salvatore** and **Riccardo D'Ambrosio**, and concise copyright/licence information. It links to LICENSE and NOTICE.md without repeating the co-authors in multiple sections.

- **Undo/Redo** use one bounded edit history across supported Show changes (cue/column structure, cell and row properties, Master/mixer adjustments and MIDI/OSC gestures). Changing VIEW tabs and GO/Stop/PANIC transport commands are not history actions. Undo of a change to the LIVE sequence stops playback for safety; undo limited to a different VIEW retains LIVE.
- **CPU/RAM** report system-level usage, not per-plugin or per-clip estimates. The two usage values and their horizontal meters have separate aligned positions; unsupported telemetry displays N/A.
- **MIDI RX/TX** indicate local traffic; **OSC TX** indicates local transmission. OSC RX remains inactive because a native OSC input endpoint is not implemented.
- **Audio Available** reports local device availability, not an assertion that speakers or downstream equipment are functioning.

### 3.3 Cue grid, Inspector, mixer and Timeline

Within a Cuelist, you can **drag a cue's title up or down** to change
its position, or use **Ctrl+Shift+Up / Ctrl+Shift+Down** to move the selected
cue by one row. The existing up/down arrows beside each cue remain available.
As with the existing row arrows, moving a cue is a structural edit that
**stops active playback** before changing cue order; the cue's cells, media
references and notes move with it.

The central cue grid has row controls on the left and columns for Audio/MIDI/OSC/Control. Its Cuelist title and all Audio/MIDI/OSC/Control column headings stay **fixed during vertical scrolling**. Column headings track their cells during horizontal scrolling. The **BPM** and **Auto Stop** strip remains fixed at the right, including during horizontal scrolling. During vertical scrolling, row values, toggles and action targets remain **below** their fixed headers: controls hidden behind the header are neither visible nor clickable. Row Play/Pause, Stop and the action menu are physically separated, as are the larger Play/Stop targets inside each cell. Column Utilities and the Master Audio strip occupy the lower working area; a Timeline with zoom/grid/snap controls is underneath.

Small icon buttons across the workspace use a restrained shared visual style: the glyph carries the semantic colour while the border stays subtle. Hover, pressed, toggled, disabled and keyboard-focus states remain distinguishable. This affects the appearance, **not** the Play/Stop/GO/PANIC actions, selection, saved shows or clicking areas. Destructive actions still use red/pink symbols. A visible focus outline indicates keyboard focus; the critical GO and PANIC actions retain their distinct treatment.

The **Inspector** on the right edits the selected cue and cell separately. Use its narrow edge tab (one framed surface with a directional chevron) to collapse the Inspector toward the right, increasing the width of the workspace; click the tab again to reopen. The same area contains a cue-specific multiline **Notes** field.

Columns can be collapsed individually or together with **Collapse all** in the cue grid header. Collapsing is a layout choice; it does not stop running media or change audio routing. The pinned BPM / Auto Stop area has a continuous visual boundary with the scrollable grid and the mixer below, including at row separators. The Cue List's **vertical scrollbar sits at its left edge**, rather than between the last cell and BPM / Auto Stop. It appears only when there are more rows than can fit on screen. The pinned row header follows that scrollbar's visible inset, keeping Play, Stop and row names aligned with the scrolling content. When no vertical scrollbar is necessary, the grid expands all the way to the BPM boundary instead of retaining an empty gutter.

### MIDI / OSC quick Learn, project functions and cue overrides

**M Learn** and **O Learn** in the upper-right toolbar enter a persistent mapping
session. The Inspector shows the function list, with a **Map to edit** dropdown
above **Cancel Learn**. Choose Default Map, a cue's Control Column override, or
an independent saved reusable map. Select an Inspector function (or click its
highlighted mixer/transport control), then send the next MIDI/OSC message.
The message is consumed without triggering live GO/PANIC; the assignment is
saved into **only the selected map**. Learn stays active for the next mapping.
Cancel Learn, or press the active Learn button again, to return to normal use.

The controls highlighted during Learn include GO, PANIC, Master Gain and Pan,
master Limiter On/Off, and each audio column's Gain, Pan, Mute,
Phase/Polarity and L/R Send. Amber means MIDI; violet means OSC. The active
destination pulses. Click a function name **twice** to rename the logical
function in place; its MIDI/OSC source and output destination are unchanged.
Named MIDI/OSC output functions keep project-specific labels and stable
identities across controller replacements. Unconfigured output columns
remain silent until routed from Control Mappings.

**Device Settings are separate from mapping.** Open Settings → MIDI Devices
to enable or disable each MIDI input and output. Several input devices can
be enabled simultaneously; a disabled output cannot be silently selected
as a substitute. Settings → OSC displays the local IP and lets you
configure up to 16 distinct listening UDP ports separated by commas.
An empty setting explicitly disables OSC input. Quick Learn never enables
an OSC listener behind your back; configure the ports first.
Settings → Audio Devices opens the existing JUCE selector (and stops playback
as before). The Clock submenu switches between the 24-hour and 12-hour
display formats.

**Default Map is permanent and low-priority**, not a normal interchangeable
cue map. Control Column maps override only their explicitly assigned
functions once that cue has been activated by GO; every other function
inherits its Default Map binding. Changing the selected Inspector row alone
does not change the live mapping. When the next cue has no override,
Default Map automatically resumes.

**Control Mappings** opens with Default Map selected and provides the archive
of **independent named Control Maps**, without creating or requiring Control
Columns. **New Map** creates an empty map, **Duplicate** copies the selected
mapping with new binding IDs, and **Rename Map** changes its label without
changing its stable ID. **Delete unused Map** requires that no cue in any
Cuelist references the map; reassignment or removal must happen first.
Select a saved map to modify its mapping entries or learn its MIDI/OSC sources.
A map's name and rules live in the shared archive: editing a map updates
**all cues explicitly assigned to that same map**. Changing a single cue's
*assignment* leaves all other cue assignments intact. Existing legacy
cue-local overrides still work independently as before. Export/Import
Default Map remains distinct and excludes project-specific names.

**Control Column cells:** double-clicking an unassigned cell creates an
**empty cue-local override**, visually labeled **CONTROL MAP / Empty map**.
This is different from the unassigned state (+ Control Map) even when
there are zero bindings. The cell's appearance remains stable across cue
selection and after a show roundtrip, including collapsed Control Columns.
Right-click an unassigned Control Column cell to assign a saved map
**without first creating a local override**. Click a populated cell to edit
the local or shared map, save a new reusable copy, assign a saved map
(including to multiple cues) or remove its assignment. Removing it
restores inheritance from Default Map; replacing one cue's assignment
never modifies another cue. Medium/High/Maximum
priority still resolves overlapping Control Column maps.

**Operational precautions:** physical controller Learn events cannot
activate live playback while being captured. MIDI note releases remain
tracked. Incoming OSC accepts supported numeric and boolean OSC 1.0
messages on the explicitly enabled UDP ports; it does not authenticate
senders or acknowledge packet delivery. Test every hardware path and
firewall setting before a performance. Historical locked Control Columns
remain supported through migration, not as a new operator lock.

### I/O Activity and quick project access

The native **M Learn** and **O Learn** actions now belong to the **I/O Activity** panel next to MIDI RX/TX and OSC RX/TX. Their Quick Learn behavior, device handling and protocol colors are unchanged.

The left sidebar offers **Recent Projects** below **Workspace**. Click a previously opened or saved show to load it with the existing safe background loader. The current show's unsaved changes still require explicit discard confirmation. **Open...** launches the ordinary project chooser; **All** lists remaining history. The recent project paths are held in the application's local preferences and do not alter the project format.

The pinned BPM / Auto Stop lanes retain their original editing/toggle hitboxes but use balanced title/value alignment, a quieter cue-state accent and a continuous separator.

## 4. Managing multiple Cuelists

### 4.1 Creating, selecting and removing

- Click **+** next to the Cuelist tabs to create a new list. A newly created list starts with an independent blank cue.
- The application uses the first available `Cuelist N` name, including numbers freed by deletion, rather than skipping numbers.
- Click a tab to view/edit another list without interrupting the LIVE transport. The Cue List, Timeline and Inspector follow the VIEW tab; the green dot marks the LIVE tab. Global GO stays on LIVE. Explicitly starting a row in VIEW takes over the transport and stops the previous live sequence.
- Tabs continue side by side while there is space; **drag a tab onto another tab to reorder entire Cuelists** (the destination is highlighted). Moving a tab does not start or stop cues, change the active Cuelist, or alter its data. Click the **< / >** arrows beside the active Cuelist title to move it one place at a time, or press **Ctrl+Shift+Left / Ctrl+Shift+Right** when a text editor is not focused. The arrows disable at the first/last position. The overflow menu also provides access to hidden Cuelists' rename, delete and **Move left / Move right** actions when the bar is too narrow. Save the project to retain the new tab order.
- Use the **×** beside a visible tab to delete a Cuelist. A confirmation dialog explains that its cues are removed from the show. Source audio files are not deleted from disk.
- The last remaining Cuelist cannot be deleted.

Deleting a Cuelist can be undone while its bounded edit history remains available; source audio files are never deleted. Save a backup before reorganizing a live show, especially when the LIVE transport is involved.

### 4.2 Renaming — synchronized in both directions

**Double-click either the Cuelist tab or its name in the pinned cue-grid header** to edit the name directly on that label, without opening a dialog. The alternative **Preferences → Rename active Cuelist** uses the same inline editor. Double-clicking a cue title or a column name also edits directly where that name appears; the Workspace project name already behaves this way.

The text is selected for immediate replacement: **Enter** confirms, **Escape** cancels without altering the show, and clicking elsewhere confirms a valid name. An invalid Cuelist name stays in the inline field for correction. Names still use the existing uniqueness/length constraints. If a Cuelist is hidden in the tab overflow, its inline editor appears beside the overflow control: renaming alone never activates another Cuelist or interrupts playback. The cue's initial BPM remains editable inline. The old **Cue tempo map** dialog has been removed: tempo changes are edited only through Timeline Markers.

The tab and the grid header are two views of the **same saved Cuelist name**. Renaming either updates both immediately, and the name persists when you save the `.cuelist` file. Names must be unique, nonempty and no longer than 64 characters.

The grid heading displays the current name only; it does not need an additional `CUE LIST /` prefix.

### 4.3 Shared and independent settings

Cuelists share column definitions, routing and the overall show settings. They do **not** share their cue ordering, cell contents or per-cue technical notes. Adding, moving or removing a column updates the corresponding cell structure across all lists; removing a Cuelist never removes a shared column.

## 5. Playing and editing cues

| Action | Behavior |
| --- | --- |
| **GO / Space** | Launches the armed next cue; it does not wrap to the beginning at the end of a sequence. |
| **Row GO** | Starts cells included in that row's GO. |
| **Cell Play** | Starts or resumes the individual cell according to its playback behavior. |
| **Cell Stop** | Stops and resets that cell. |
| **Row Stop** | Stops the cue's cells, including cells excluded from Row GO. |
| **Row Play/Pause** | Pauses or resumes row playback without restarting the cue where supported. |
| **PANIC / Esc** | Stops active audio, MIDI and OSC playback. |
| **Ctrl+C / Ctrl+V** | Copies/pastes compatible selected cells; row operations are also available through cue context menus. |
| **Ctrl+X / Backspace** | Cuts or removes the selected cell where allowed, without intercepting normal text editing. |
| **Ctrl+R** | Renames the selected cell inline. Enter saves, Escape cancels. Does not rename the underlying media file. |

Space does not launch a cue while typing into a text input. Escape is intended to remain available for Panic even from the Inspector note editor; check the behavior with your connected devices.

Click a cue to make it the next armed cue. Use the row menu to **Copy**, **Paste below**, **Duplicate** or **Delete** a whole cue. The cell context menu covers cell-specific copy, paste, cut, delete, loop and inclusion in Row GO. **Rename cell** edits the cell label inline for Audio, MIDI, OSC and Control; the same operation is available with Ctrl+R on the selected cell. The name is saved with the show, not applied to the source file or the Control Map. For Audio cells only, **Find in Explorer** reveals the associated existing source file in Windows Explorer. It is disabled when no valid local file is available. Duplicated rows and cells receive appropriate new identities; imported audio can share decoded PCM in memory when safe.

Use the **▲ / ▼** controls to reorder cue rows. These glyphs, the **◀ / ▶** column controls, **▶ / ⏸ / ⏹** transport icons and **⋯** row actions use **Noto Sans Symbols 2** explicitly where the embedded typeface provides them. A vector symbol is used when the exact glyph is unavailable; no operating-system emoji substitution occurs. The Delete cross remains vector geometry. See the [Unicode symbol audit](unicode-symbol-audit.md). Changes that affect the structure or timing may stop transport to preserve deterministic cue state.

### 5.1 Playback behavior and looping

The clip playback behavior determines how a second trigger acts: **Retrigger**, **Pause/Restart**, **Pause/Resume** or **Continue** as configured. **Loop playback** is independent of that behavior. The Inspector separately exposes the cue's **Loop all clips**, the cell's **Loop playback** and **Include in row GO** settings.

### 5.2 Auto Stop

Each row has a pinned **Auto Stop** toggle, normally enabled initially. Select the separate **CELLS +** control beneath the switch to reveal the cell-level Auto Stop options. It changes to **CELLS −** while expanded. The switch and disclosure have distinct clickable regions and remain below the fixed header when you scroll.

A previously active cell is stopped by a later cue's GO only when both its row-level Auto Stop and its cell-level Auto Stop are enabled. Disabling row Auto Stop allows its clips to continue. A direct Stop, Row Stop or Panic still stops them. The next GO can also stop protected clips independently of whether the new cue itself has media.

## 6. Inspector: cue settings, cell settings and notes

The right-side Inspector distinguishes **Cue properties** from **Cell properties**.

**Cue:** name, initial BPM and cue-wide loop selection; tempo changes and generic markers are edited directly in the Timeline. **Cell:** name, compatible options for Audio/MIDI/OSC/Control, gain in dB for audio, inclusion in GO, playback loop, clip import or editor launch.

The **Notes / Cue NN** multiline editor holds technical stage directions, reminders and other operational notes. They belong to the selected cue and are included in the project save; each Cuelist maintains its own notes. Moving to another cue or collapsing the Inspector commits valid pending changes. An invalid Inspector value can prevent the panel from closing until corrected.

Text input not yet applied to the model is not independently guaranteed to exist in a periodic recovery checkpoint. Save explicitly after editing cue notes and titles.

### Control Map editing: one shared interface

The **Control Mappings** panel is the only native editor for Default Map, saved reusable Control Maps and cue-local Control Column overrides. Click an assigned Control Column cell to open the same editor with its Map preselected. The `...` menu on a cue-local override provides effective mapping preview, saving the local overrides as a reusable Map, and applying an existing saved Map. These actions retain the existing ownership and assignment semantics; opening the editor never changes assignments. Use the Control Column cell's right-click menu to assign or remove Maps.

When editing a cue-local override, function destinations are inherited from project functions and remain read-only. Use MIDI/OSC Learn to capture the replacement physical input. Default and saved Maps retain their regular function editing controls.

## 7. Audio, mixer and routing

### 7.1 Audio cells

Audio cells support WAV and MP3, imported with the cell's import action. One mono or stereo file may be associated with a cell. The show decodes audio to PCM for playback and enforces memory limits; media import and saving large collections can still take time.

Individual cell gain is expressed in **dB**. A peak meter reports **dBFS**: 0 dB gain is unity, while 0 dBFS is the digital ceiling. The two units are not interchangeable. A muted fader reads **−∞ dB**; its mathematical minus/infinity symbols are rendered directly with embedded Noto Sans Math, while the unit remains in Exo 2. The meter's L/R labels and the gain readout occupy separate vertical lanes.

### 7.2 Column Utilities

Audio column strips provide gain, stereo meters, pan, Mute, phase inversion and routing. MIDI and OSC strips expose the appropriate output controls rather than audio faders. Collapsed Audio columns retain compact meter/gain and Mute controls. **Mute** uses the embedded **Noto Sans Symbols 2** directly: **🔊** means audio/output enabled; **🔇** means muted. Both are centred by their visible glyph outlines inside the compact button, rather than by the font's advance width. The same two-state icon is used for Master, Audio columns and MIDI/OSC data-output mute, while phase inversion retains its own symbol.

An Audio column can send signal to the **Master** on its left/right paths and/or to a physical stereo **Direct Out** pair. Direct can be **PRE** or **POST** relative to the strip fader; post-strip Master Send is independent. Mute, pan and other per-cell/per-cue processing follow their implemented signal path.

### 7.3 Master Audio

The **Master Audio** strip provides Main Out routing, stereo gain/meter, pan, mute and an optional safety peak limiter. Its meter is pre-limiter. The limiter indicates current gain reduction and does not affect independent Direct Out paths.

**Routing warning:** sending a Direct Out to the same physical channels as Master Main Out can cause post-limiter summation and clipping. Preflight warns about the conflict, but an operator must verify the actual routing.

Master gain/mute/pan act on PCM audio, not MIDI notes or OSC commands. The output stage clamps device PCM samples, but that is not a substitute for physical gain staging and rehearsal.

### 7.4 Generate LTC Track (audio source only)

Right-click an **empty Audio cell** and select **Generate LTC Track**. Enter duration (0.1–600 seconds), start timecode (HH:MM:SS:FF; use HH:MM:SS;FF for 29.97 drop-frame), and frame rate (24, 25, 29.97 drop-frame, 29.97 non-drop or 30 fps). The operation writes a real **mono 48 kHz / 16-bit PCM WAV** in `Documents/The Cuelist/Generated LTC`, then imports it as an ordinary Audio cell. Occupied cells cannot be overwritten through this command. Generated media must be preserved alongside the project when transferring shows.

For a timecode receiver, dedicate a separate **Direct Out** hardware pair, disable that column's **Master Send**, and route only a chosen line output into the receiver's LTC input. Use unity source/row levels, start with the physical interface output gain low, disable OS/device audio enhancement, and avoid mute, fader automation, pan changes, phase reversal, looping, crossfades or shared mixes on the timecode path. A PRE Direct Out bypasses the column fader but **not** clip/row gain, mute, pan or phase. The same mono signal is present on both sides of the selected Direct Out pair; connect only the intended line output.

**Safety:** LTC is a harsh encoded waveform, not listening material. Never route it to audience loudspeakers, headphones or an unverified Master output. Verify all patching and gain staging before playback. A WAV generated at 48 kHz may be resampled by the device when running at another sample rate; external decoder tolerance must be verified on that actual device.

This feature is **audio LTC generation only**. It does not synchronize the Cuelist's transport from external timecode, provide global LTC/MTC following or guarantee lock with other devices. Before show use, follow [the LTC hardware acceptance checklist](ltc-hardware-acceptance.md) with an independent decoder.

## 8. MIDI clips and Piano Roll

Add a **MIDI** column and choose the correct external output in its Column Utilities. Import an existing MIDI file or create a note clip in the built-in **Piano Roll**. The clip participates in cue GO, stopping, looping and timeline operations as supported.

The importer supports standard MIDI type 0/1 files using PPQ, subject to size and note-count limits. File tempo and non-note events such as controllers, program changes, SysEx and sustain may not be imported; the cue's tempo map controls playback of imported notes. SMPTE and type-2 MIDI files are unsupported.

MIDI ports are tracked by their device identifiers. A missing configured output is **not** silently replaced by another interface. Reconnecting the original exact-ID port invalidates outdated queued messages. Physical output latency remains dependent on hardware, driver and OS scheduling.

Use **Undo/Redo** for the supported Show, MIDI and OSC editing operations. All use the same chronological history; the buttons are disabled when no transaction is available. GO, Stop, PANIC and switching VIEW tabs are excluded.

## 9. OSC messages and automation

Add an **OSC** column and enter the destination **host/IP address and UDP port** in Column Utilities. Create an OSC clip, then edit timed events and automation tracks using the OSC editor.

- Event messages contain an OSC address, event time and ordered typed arguments.
- Arguments use `i:` (int32), `f:` (float32), `s:` (string), and `b:` (boolean), for example `i:7; f:0.5; s:scene; b:true`.
- **Double-click a track header** to edit the common OSC address, typed argument list, 1-based automated numeric argument, and parameter minimum/maximum. Every segment on that track uses these settings; incompatible changes are rejected without modifying the clip. **Double-click a ramp** to adjust only its start/end values. Intermediate points remain untouched. Ctrl-click to select several events or ramps, drag a multi-selection together, or use Duplicate/Delete; Ctrl-drag empty timeline space draws a selection marquee.
- Events and automation share the editor's horizontal timeline; vertical scrolling follows the same track alignment.
- Clips may use Seconds or Beats; conversion uses the selected cue's tempo map.
- MIDI/OSC events are timestamped against the audio callback's common monotonic origin, but actual delivery can be delayed by device workers and the operating system.

**UDP has no built-in delivery acknowledgement.** A transmitted packet count or illuminated OSC TX indicator does not prove that the receiver applied a command. Use receiver-side monitoring during rehearsal.

## 10. Control Mapping and Control Columns

### 10.1 Learn and physical devices

Select the intended map using the dropdown in Inspector Learn. For each
function, select the target and send the next MIDI or OSC message; the
protocol stays armed for successive assignments. Mixer and transport
hotspots select the same stable logical functions. Double-click a function
name in the Inspector to rename it without losing its hardware assignment.

Enable MIDI input/output and configure OSC listeners **only under Settings**.
A MIDI input can be absent while its target remains highlighted for a future
capture. OSC input must have at least one enabled UDP listening port.

### 10.2 Default, reusable and cue-local maps

**Default Map** is the unremovable global fallback. A saved reusable map
stores a snapshot of function input assignments until explicitly loaded
into a Control Column cell. At confirmed GO, that cell's assignments override
only the corresponding function IDs; all other inputs fall back to Default
Map. Changing an inactive cue or modifying a reusable map does not silently
change the live scene.

Control Mappings opens on Default Map and manages the saved map archive:
create, duplicate, rename, and delete when unused. A saved map exists
independently of any Control Column and can be referenced in multiple cues.
Updating the shared map updates its assignments; changing a cue's selected
map only affects that cue. In Control Columns, double-click an empty cell
to create a **visibly assigned but empty** local map, or click a mapped
cell for edit/assign/replace/remove actions. Clearing an assignment
restores Default Map; its saved map remains in the archive.

Controller preset export/import transfers hardware assignments and output
routing without replacing project-specific names. Existing historical show
mappings and older locked-column metadata remain supported by migration.

Test connected and disconnected hardware, a cue transition, controls with
Pickup and Jump, CC/button releases and local network OSC reception
before using a new mapping in a live show.

## 11. Tempo maps and Timeline

The selected cue owns a dedicated Marker lane above its audio, MIDI and OSC tracks. The lane remains visible and editable when the Timeline is resized to a compact height (120 px or taller); media tracks hide if the panel is too small. Double-click an empty point in this lane to add a **Generic Marker** (a stage reference that never changes tempo), or double-click an existing marker to rename it inline. Drag a marker to move it. Both creation and movement follow the current magnetic Snap division; turn Snap off for a free, absolute-time position.

Right-click a marker to **Rename**, **Convert to Tempo Marker**, **Edit BPM**, **Convert to Generic Marker**, or **Delete**, according to its current type. A Tempo Marker changes the BPM instantly at its own timestamp and holds that value until the next Tempo Marker. The cue's existing inline BPM sets the initial speed; no separate Tempo Map button or Inspector dialog is used. Generic Markers do not change timing.

Marker timestamps are stored in **absolute cue seconds** (not in bars or beats), so an earlier BPM edit cannot displace a later marker. The audio, MIDI and OSC scheduling engine still uses its existing beat-to-second conversion; it rebuilds the internal Tempo Map from these absolute-time markers. Older v10 and pre-v10 tempo maps are imported with their original beat changes converted to the corresponding absolute timestamps. Saved projects remain JSON schema v10.

**LIVE continuity:** creating, dragging, renaming, converting, editing BPM or deleting Timeline Markers must never call STOP/PANIC or restart a running cue. Editing the initial cue BPM has the same rule, including Undo/Redo. The audio cursor and cue clock continue. During a live tempo edit, the MIDI/OSC event schedule is updated from the unchanged absolute-time playback position; MIDI notes that remain active across the change are not re-attacked. Events that move to times already in the past are not replayed. This is not a guarantee of end-to-end timing or external hardware latency; verify those separately.

The Timeline retains zoom, grid visibility, Snap, horizontal scrolling and independent control of clip dragging. The BPM grid reacts to each Tempo Marker without changing audio-file playback speed. The existing pinned BPM and Auto Stop controls remain visible in the cue matrix.

## 12. Files, backups and recovery

### 12.1 Saving and transferring a show

The project document is `<name>.cuelist`, a versioned JSON file with **schema v10**. The application version `versione in [VERSION.txt](../VERSION.txt)` is a separate identifier. Audio assets are collected into an adjacent `<name>_media` folder; **transfer the project file and this folder together**. MIDI/OSC clip data and Cuelist metadata are saved within the project document.

Overwriting may retain a `.cuelist.bak` backup and older media generations. Check disk usage. Previously created schema v1–v9 and compatible v10 projects can be opened by the supported migration logic; the optional multi-Cuelist and cue-note fields do not require a schema-number change. Newer feature data may not be understood by older application binaries.

### 12.2 Project load safety

Opening a project parses and validates the model and prepares its media on a background worker. The active show is not replaced until the new project passes validation, media decoding and PCM budget checks. Newer load requests supersede stale ones. Viewing another Cuelist inside an already-open project does not stop the LIVE transport; explicit cue GO in another tab intentionally changes the live source.

### 12.3 Crash recovery

A bounded background checkpoint writer periodically saves applied unsaved model changes (approximately every 20 seconds) using an atomic file replacement. After an abnormal exit, the application can offer **Restore**, **Discard checkpoint** or **Later**. Recovery does not automatically start playback and does not overwrite the saved show.

A checkpoint is not a substitute for `Save` and off-device copies. It may not reflect keystrokes that were still in an uncommitted Inspector field, and it references media files rather than making additional copies.

## 13. Preflight, Diagnostics and physical validation

Use **Preferences → Pre-flight** to inspect model validity, missing or empty audio, configured device availability, potential routing collisions and OSC endpoint configuration. An OSC destination being configured is not proof that any remote server is online.

Use **Preferences → Diagnostics** for a grouped report covering **Audio**, **MIDI**, **OSC** and **Recovery**, with left-aligned metrics, right-aligned values and distinct colours for connected, disconnected, late and failure states. Counters are a snapshot of the current process, not a continuously updating hardware readout. These measurements describe The Cuelist's workers; external cable, device and receiver latency is not measured.

**Recommended acceptance rehearsal:** run simultaneous audio/MIDI/OSC cues; exercise GO, Row Stop, Panic, selective Auto Stop, Direct/MAIN outputs, Master limiter, Cuelist switching, note saving, MIDI controller connection changes, and OSC receiver failure/restart. Perform a prolonged load test and record real device behavior.

See [Stability and timing](stability-and-timing.md) and the [0.9 release acceptance checklist](0.9-release-checklist.md) for further engineering context.

## 14. Build, test and optional packaging

Install Visual Studio 2022 or Build Tools with **Desktop development with C++**, a Windows SDK and CMake. The build script downloads the pinned JUCE **8.0.12** dependency when `vendor/JUCE` is missing (Git required only for that acquisition).

```powershell
# Release application, test targets and CTest
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1

# Optional: test-only target, or compile then launch
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1 -TestOnly
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1 -Run

# Explicit package creation and extraction verification (not a regular build)
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/package.ps1 -SkipBuild
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/check-package.ps1 -SmokeLaunch

# Read-only hardware inventory, optionally followed by localhost OSC test
powershell -ExecutionPolicy Bypass -File scripts/hardware-check.ps1
powershell -ExecutionPolicy Bypass -File scripts/hardware-check.ps1 -OscLoopback
```

The executable is written to `build/TheCuelist_artefacts/Release/The Cuelist.exe`. Standard pull-request CI builds and runs CTest; creating a portable ZIP is an explicitly requested separate operation. Packaged archives include a SHA-256 checksum and the required license notices.

### Source layout

| Path | Responsibility |
| --- | --- |
| `src/ShowModel.*` | Show/Cuelist data, validation, migrations, JSON and media paths |
| `src/AudioEngine.*` | PCM playback, transport, DSP, output routing and telemetry |
| `src/MainComponent.*`, `src/CueViews.cpp` | Main workspace, tabs, cue grid, sidebar, Inspector coordination |
| `src/Timeline.cpp`, `src/ColumnUtilities.cpp` | Timing views and media/output utility strips |
| `src/MidiClip.cpp`, `src/MidiUI.cpp`, `src/PianoRoll.cpp` | MIDI clips and note editor |
| `src/OscClip.cpp`, `src/OscUI.cpp`, `src/OscEditor.cpp` | OSC clips, automation and editor |
| `src/ControlMapping.h`, `src/ControlRuntime.cpp` | MIDI input mappings, layers, controller dispatch |
| `tests/`, `scripts/` | Automated tests, Windows build, hardware probe and packaging |

## 15. Known limitations and legal terms

This prerelease does not provide native video playback, global LTC/MTC chase or external timecode sync, motorized MIDI controller feedback or guaranteed sample-accurate external MIDI/OSC delivery. It can generate LTC audio tracks, but independent receiver compatibility and hardware output timing require physical tests. The application still has UI-thread work in some import/save operations and needs hardware stress testing before a public production release. Prefer independent backups and a tested fallback.

The Cuelist is **proprietary software; all rights reserved, not open source**. The [LICENSE](../LICENSE) is an English-only conventional copyright reservation, **not a complete commercial end-user licence agreement**. Individual licences already lawfully granted continue under their own terms; any new paid distribution requires separately prepared licence and sales conditions. See [NOTICE.md](../NOTICE.md) for independent JUCE and other third-party obligations. Do not distribute a proprietary binary without securing all applicable third-party rights.
