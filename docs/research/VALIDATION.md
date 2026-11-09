# Validazione The CUElist: automatica, banco e operatore

## I tre livelli di prova

**A — CI:** CMake Release Windows / CTest / regression e screenshot nativi. Dimostra che i contratti testati funzionano in ambiente controllato, **non** che dispositivi esterni abbiano ricevuto il messaggio.

**B — Banco tecnico:** PC Windows rappresentativo con interfacce reali, MIDI I/O, receiver OSC e decoder LTC indipendente. Acquisire timestamp esterni, log, percentili e contatori; evitare connessioni verso attuatori/scenografie reali durante fault injection. Usare show di prova e uscite isolate.

**C — Operatore:** prove su task prestabiliti con tecnici/utenti, incluse sessioni di pressione e recovery. Registrare passi, successi, errori, tempi e capacità di spiegare lo stato LIVE. Un tester non equivale a validazione statistica.

## Comandi reali (Windows, dalla root)

~~~powershell
python scripts/check_versioning.py
python scripts/test_versioning.py
powershell -ExecutionPolicy Bypass -File scripts/build.ps1
powershell -ExecutionPolicy Bypass -File scripts/hardware-check.ps1 -OscLoopback
~~~

Il primo blocco non sostituisce il Windows CTest; il secondo produce un inventario hardware e un test OSC **solo loopback** e non verifica delivery verso il destinatario reale. Il comando build scarica JUCE pinned, quando mancante, e richiede Visual Studio C++.

Per LTC seguire [docs/ltc-hardware-acceptance.md](../ltc-hardware-acceptance.md); per test playback e MIDI vedere [docs/testing/live-playback-midi-output.md](../testing/live-playback-midi-output.md); per gate completo vedere [docs/0.9-release-checklist.md](../0.9-release-checklist.md).

## Matrice minima di condizioni

- Windows 100%, 125%, 150% DPI; finestre 1180×820, 1280×768, 1440×900, 1920×1080, ultrawide 3840×1080 e 5120×1440 quando disponibile.
- Output audio: stereo e multicanale; Direct PRE/POST, Master, limiter, canali sovrapposti, mute, click/drop.
- MIDI: sender/receiver, CC, Note On/Off, hot-unplug/replug, cambio esatto device, assenza port.
- OSC: loopback, IP/porta errati, processo remoto chiuso, pacchetti non validi, comandi multipli, jitter rete; non considerare successo una send() senza verifica ricezione.
- LTC: 24, 25, 29.97 DF e NDF, 30 fps, almeno una transizione minuto e dieci minuti; decoder esterno per lock/continuità.
- Progetto: show schema v10, vecchi show compatibili, import media mancanti, recovery, save/backup, Undo e VIEW/LIVE.
- GO/STOP/PANIC e device events sotto carico; uso prolungato con log CPU/RAM, allocazioni, buffer underrun, overflow e rate di late dispatch.

## Metriche da preregistrare

- **Tempi reali:** scheduling→ricezione dispositivo, median/p95/p99/worst, outlier e clock di riferimento/accuratezza del misuratore; distinguere audio callback, worker dispatch e receiver esterno.
- **Stabilità:** drop/overflow, memory growth, interruzioni audio, note appese, errori di file/device, recovery completata.
- **Usabilità:** completion rate per task, tempo da click o keypress a successo, GO involontari, correzioni, comprensione LIVE/VIEW, stato noto dopo perdita dispositivo.
- **Integrità:** salvataggio e riapertura senza perdita, schema immutato, no silent mutation del live project, rollback su import fallito.

Stabilire soglie di successo prima di eseguire esperimenti; non inventare millisecondi «garantiti» né trasformare test offline in prove di latenza fisica.

## Rapporto e tracciabilità

Ogni prova usa un template con ID, commit hash, macchina, periferiche, driver, firmware, sample rate/buffer, schema show, comando o scenario, output grezzi, risultato, limiti, riproducibilità. Aggiornare [DECISIONS.md](DECISIONS.md), mantenendo aperto il gate hardware finché realmente non superato.
