# The CUElist — inventario verificato

**Snapshot:** `main@ae671d774cdcc08158a68e0af70398e08c655746`, 9 novembre 2026. **Stato: sola lettura di codice/documentazione; nessuna build Windows o test hardware eseguito per questo report.**

- **Versione:** `VERSION.txt` = 0.9.30; CMake `project(TheCuelist VERSION 0.9.30)`, JUCE 8.0.12, C++17, Windows x64.
- **Formato:** progetti `.cuelist` JSON schema **v10**, indipendente dalla versione software.
- **Audio/MIDI/OSC:** engine, router separati, Control Mappings, editor MIDI/OSC e generazione LTC audio; il prodotto non si presenta come DAW e non dispone di playback video nativo.
- **Workflow operatore:** [CHANGELOG](../../CHANGELOG.md) segnala VIEW vs LIVE separate in 0.9.17, Undo/Redo show editor in 0.9.19, nuove policy MIDI in 0.9.24, redesign UI/editor fino a 0.9.30.
- **Temi:** Graphite/Copper e Industrial Orange convivono; la ricerca UX non può assumere che l'ultimo tema sia l'unico utilizzato.
- **Test:** [tests/](../../tests/) contiene test Audio, MIDI, OSC, transport, Control, LTC, rendering e modelli. [.github/workflows/windows-ci.yml](../../.github/workflows/windows-ci.yml) usa Windows 2022, configura/build e CTest con screenshot.
- **Gate aperti:** [0.9 release checklist](../0.9-release-checklist.md), [LTC hardware acceptance](../ltc-hardware-acceptance.md) e [stability/timing](../stability-and-timing.md) richiedono collaudi fisici, DPI e verifica di sicurezza del flusso LIVE.

## Fatti vs assunti da verificare

| Tema | Supportato dai file | Da misurare/verificare |
|---|---|---|
| VIEW indipendente da LIVE | codice/changelog versione 0.9.17+ | comprensione visiva, errore GO involontario, stato under stress |
| Routing e policy di reconnect | documentazione, codice, test | strumenti MIDI/OSC reali, disconnect a caldo, stuck notes |
| Timing Audio/MIDI/OSC | trasporto/contatori e test | timestamp esterni, p95/p99, jitter worst case, drop |
| LTC generato | test e checklist accettazione | decoder esterno, 24/25/29.97 DF/NDF/30 fps, continuità |
| UX e grafica nativa | JUCE e test geometry | screenshot visivi su Win 100/125/150% DPI, ultrawide e tastiera |
| Progetti e recover | codice e note QA | smoke di show reale, salvataggi, media assenti, recovery crash |
| Versione/packaging | regole repository | licenze, ZIP portable, installazione su macchina pulita |

## Debito documentale rilevato

`README.md` ha un riferimento fisso a 0.9.28 anche mentre `VERSION.txt` è 0.9.30; nella PR bootstrap l'etichetta viene resa dinamica. Alcune roadmap descrivono vecchie beta e non devono prevalere sulla main recente. L'identità reale delle feature è nel codice e nei test al commit dichiarato.

**Nessuna nuova funzionalità proposta è già pronta alla distribuzione.**
