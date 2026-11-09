# RQ-001 — Validazione VIEW/LIVE, GO e focus (protocollo)

**Data:** 2026-11-09
**Baseline di sola lettura:** \`main@21e75aabbaa551c2342340c63a822d94bbc07f2a\`, versione 0.9.31.
**Branch di verifica:** \`test/rq001-view-live-fault-repro\`.
**Formato progetto:** JSON v10, invariato.
**Stato:** test di regressione proposti sul codice nativo JUCE; risultati Windows, operatori e hardware **ND** finché non viene allegato un report eseguito.

## Ipotesi, non risultati

- **H-T02:** visualizzare una Cuelist non LIVE e chiedere di importare un file in una cella Audio vuota non deve disarmare o interrompere la LIVE preesistente.
- **H-T01:** l'avvio esplicito di una singola cella della Cuelist LIVE non deve modificare la prossima cue GO armata, né riarmarla accidentalmente al cambio VIEW.
- **H-T03:** l'operatore identifica la lista LIVE, anche quando la relativa tab è nascosta nell'overflow e VIEW è un'altra Cuelist.

La semantica di **GO esplicito su una riga** è differente dalla visualizzazione e dal singolo Cell Play: il GO esplicito della VIEW è documentato come trasferimento intenzionale del trasporto.

## Evidenza di codice disponibile

- \`src/CueOperations.cpp\`, \`activateCuelist\`: cambia VIEW senza chiamare PANIC, preserva \`liveCuelist\` e \`liveNextCue\`.
- \`src/MainComponent.cpp\`, \`triggerCell\`: al momento applica \`stopAll()\` e trasferisce la LIVE **prima** di discriminare il caso della cella vuota / import.
- \`src/MainComponent.cpp\`, \`triggerCellAt\`: salva/ripristina \`nextCue\`, mentre \`select()\` ha anche modificato \`liveNextCue\`.
- \`src/MainComponent.cpp\`, \`refreshCuelistTabs\`: l'overflow mantiene la tab VIEW attiva visibile e può occultare la LIVE.
- \`tests/CuelistGuiMockupTests.h\`: mantiene i test preesistenti della separazione VIEW/LIVE; le nuove prove T02 e T01 sono inserite accanto a essi.

I percorsi sopra sono verificati staticamente. Non sono una misurazione di comportamento su Windows o una certificazione.

## Prove automatiche — ordine T02, T01

### T02 — cella Audio vuota in VIEW

**Fixture:** MainComponent nativo, LIVE A con un clip PCM lungo in playback, seconda Cuelist B appena creata con cella Audio vuota.

1. Avviare cue in A e verificare \`getCellPlayback(...).playing\`.
2. Creare/selezionare VIEW B senza cambiare LIVE.
3. Invocare \`triggerCell()\` sulla cella Audio vuota di B (stesso callback del doppio clic Audio).
4. Processare un ulteriore blocco audio simulato.
5. Asserire che LIVE sia ancora A e che il clip originario sia in playback.

**Atteso:** nessun PANIC per il solo intento d'import. Il selettore file asincrono non deve influire sul trasporto.

**Rischio infrastrutturale:** il test attraversa \`juce::FileChooser::launchAsync\`; su runner Windows headless verificare che il selettore non provochi blocco o failure indipendente dal trasporto. In quel caso il risultato rimane **inconcludente**, non classificato come difetto provato.

### T01 — Cell Play e cursore GO

**Fixture:** A con quattro cue e clip PCM disponibile nella cella della cue 4.

1. Armare la cue 2 in A.
2. Eseguire \`triggerCellAt(3,0)\`, simulando Cell Play della cue 4.
3. Asserire che il cursore della prossima GO rimanga sulla cue 2.
4. Aprire VIEW B.
5. Eseguire \`goNext()\` e processare il blocco di ack.
6. Verificare che \`acknowledgedCueId\` sia quello della cue 2.

**Atteso:** Cell Play distinto da selezione/GO di riga; nessun cambiamento accidentale del cursore. Confermare questa policy con gli operatori prima di fissarla come contratto definitivo.

## T03 — task UX su interfaccia nativa

**Fixture:** nove Cuelist con nomi non intercambiabili: \`Pre-show\`, \`Act I\`, \`Act II\`, \`Act III\`, \`Intermission\`, \`Finale\`, \`Backup\`, \`Soundcheck\`, \`Emergency\`. Finestra 1180×820 Windows, scalatura 100% e 150%, almeno una prova con operatore che non conosce The CUElist.

1. Avviare A, poi aprire una VIEW distante nell'overflow.
2. Chiedere «Quale lista riceverà Space?» prima di ogni pressione.
3. Chiedere di selezionare una cue nella VIEW senza cambiare LIVE.
4. Chiedere di riconoscere il nome LIVE senza usare il tooltip; registrare se cerca nel menu overflow.
5. Eseguire dopo conferma la stessa prova a 1920×1080 per controllare l'effetto della visibilità delle tab.

Registrare task success, identificazioni errate, tempo dalla domanda alla risposta, primo elemento guardato, esitazioni e modalità di recupero. L'osservazione di un tecnico non sostituisce quella di operatori con livelli diversi di esperienza.

## Misure e criteri

| Metrica | Unità | Criterio preliminare |
|---|---|---|
| T02 LIVE preservata | booleano | Sempre vera per l'import non concluso |
| T01 next cue ID | identity int | Uguale al precedente GO armato |
| T01 ack della GO dalla VIEW | cue ID | Corrisponde al GO armato |
| T03 identificazione LIVE | corretto/errato | Target 95%, da rivedere dopo pilota |
| T03 tempo di riconoscimento | millisecondi | Mediana e p90; **nessun valore misurato** |
| T03 GO involontario | conteggio | Ogni evento indagato singolarmente |

**ND:** latenza reale di dispositivi, dati operatori, prestazioni CI, test Windows, screenshot nativi e hardware.

## Red team

- **Contro T01:** l'operatore può aspettarsi che cliccare la cue (non il tasto Cell Play) riarmi GO. Tenere distinti selezione semplice, Cell Play, Row GO e trigger mappati.
- **Contro T02:** attivare una cue della VIEW deve trasferire la LIVE intenzionalmente. L'importazione di un file, però, non è un'attivazione.
- **Contro T03:** un label LIVE sempre visibile potrebbe aggiungere rumore; provare la riconoscibilità attuale prima di aggiungere nuovi componenti.
- **Fallback QA:** se il file picker fallisce su runner headless, costruire un test isolato senza simulare falsa evidenza di playback interrotto.

## Gate di avanzamento

- [ ] CTest Windows eseguito sul commit della PR; annotare test rossi e cause.
- [ ] Riprova T02 mediante doppio clic nativo su Windows con file chooser cancellato.
- [ ] Riprova T01 con playback e cue multiple, inclusa presenza di MIDI/OSC.
- [ ] Screenshot Windows ai DPI richiesti per T03.
- [ ] Pilota UX con partecipanti reali e tab invisibile.
- [ ] Verifica nessuna regressione a GO, PANIC, editing inline, Control Map, Undo/Redo, JSON v10.
- [ ] Gate hardware separato; CI verde **non** equivale a ready for live.

**Strategia di rollback:** scartare il branch/PR di esperimento senza toccare \`main\`. Eventuali fix richiedono un ulteriore gate e un piano di regressione. Nessun merge automatico.
