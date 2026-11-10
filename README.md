# The CUElist

**Native show-control software for Windows.**

The CUElist brings Audio, MIDI, OSC and generated LTC audio together in a cue-based workspace for theatre, concerts, live events and installations. Designed for show operators, it focuses on predictable playback and clear control rather than digital audio workstation features.

**Platform:** Windows x64 · **Stack:** C++17 / JUCE 8.0.12 · **Project format:** `.cuelist` (JSON v10) · **Version:** [`VERSION.txt`](VERSION.txt)

## Features

- **Cue control:** up to 12 Cuelists, independent VIEW/LIVE navigation, GO, STOP, PANIC, row/cell playback, per-cell Auto Stop and Undo/Redo.
- **Audio:** WAV/MP3 playback, mixer, Master, gain/pan/mute, multichannel output routing and limiter.
- **MIDI & OSC:** MIDI clips and Piano Roll, tempo maps, scheduled OSC events, automation and Control Mappings with MIDI/OSC Learn.
- **LTC:** generation of LTC audio tracks in Audio cells.
- **Workspace:** cue matrix, Timeline, Inspector, Preflight, Diagnostics, backups and crash recovery.

## Build from source

Requires **Windows x64**, Visual Studio 2022 / Build Tools (*Desktop development with C++*), Windows SDK, CMake tools and Git. The build script retrieves the pinned JUCE dependency if needed.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1
```

The script builds the Release application and runs CTest. Executable: `build/TheCuelist_artefacts/Release/The Cuelist.exe`.

## Projects and documentation

Shows are saved as `.cuelist` files using **JSON schema v10**, independently of the application version. Keep the associated `<show-name>_media` directory when moving a project.

- [Complete User Guide](docs/complete-user-guide.md) — operating instructions and configuration.
- [Changelog](CHANGELOG.md) — versions, changes and verification results.
- [R&D Lab](docs/research/README.md) — research, test protocols and remaining validation gates.

## Status and licensing

**In active development; not certified for show-critical operation.** Automated tests do not establish hardware compatibility or end-to-end timing accuracy. Validate the complete setup, including Audio, MIDI, OSC and LTC outputs, before a live production.

Copyright © 2026 **Davide Salvatore and Riccardo D'Ambrosio**. All rights reserved. The original application is proprietary; see [LICENSE](LICENSE) and [third-party notices](NOTICE.md). Distribution is subject to the applicable third-party licensing terms.
