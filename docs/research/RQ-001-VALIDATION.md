# RQ-001 — Validazione VIEW/LIVE, GO e focus (protocollo)

**Data:** 2026-11-09
**Baseline di sola lettura:** `main@21e75aabbaa551c2342340c63a822d94bbc07f2a`, versione 0.9.31.
**Branch di verifica:** `test/rq001-view-live-fault-repro`.
**Formato progetto:** JSON v10, invariato.
**Stato aggiornato:** l'utente ha fornito il log Windows/CTest della 0.9.32; la prova automatizzata è stata eseguita, con fallimenti dettagliati sotto. Prove operatore, screenshot DPI e hardware **ND**.

## Ipotesi, non risultati

- **H-T02:** visualizzare una Cuelist non LIVE e chiedere di importare un file in una cella Audio vuota non deve disarmare o interrompere la LIVE preesistente.
- **H-T01:** l'avvio esplicito di una singola cella della Cuelist LIVE non deve modificare la prossima cue GO armata, né riarmarla accidentalmente al cambio VIEW.
- **H-T03:** l'operatore identifica la lista LIVE, anche quando la relativa tab è nascosta nell'overflow e VIEW è un'altra Cuelist.

La semantica di **GO esplicito su una riga** è differente dalla visualizzazione e dal singolo Cell Play: il GO esplicito della VIEW è documentato come trasferimento intenzionale del trasporto.

## Evidenza di codice disponibile

- `src/CueOperations.cpp`, `activateCuelist`: cambia VIEW senza chiamare PANIC, preserva `liveCuelist` e `liveNextCue`.
- `src/MainComponent.cpp`, `triggerCell`: al momento applica `stopAll()` e trasferisce la LIVE **prima** di discriminare il caso della cella vuota / import.
- `src/MainComponent.cpp`, `triggerCellAt`: salva/ripristina `nextCue`, mentre `select()` ha anche modificato `liveNextCue`.
- `src/MainComponent.cpp`, `refreshCuelistTabs`: l'overflow mantiene la tab VIEW attiva visibile e può occultare la LIVE.
- `tests/CuelistGuiMockupTests.h`: mantiene i test preesistenti della separazione VIEW/LIVE; le nuove prove T02 e T01 sono inserite accanto a essi.

I percorsi sopra sono verificati staticamente. Non sono una misurazione di comportamento su Windows o una certificazione.

## Prove automatiche — ordine T02, T01

### T02 — cella Audio vuota in VIEW

**Fixture:** MainComponent nativo, LIVE A con un clip PCM lungo in playback, seconda Cuelist B appena creata con cella Audio vuota.

1. Avviare cue in A e verificare `getCellPlayback(...).playing`.
2. Creare/selezionare VIEW B senza cambiare LIVE.
3. Invocare `triggerCell()` sulla cella Audio vuota di B (stesso callback del doppio clic Audio).
4. Processare un ulteriore blocco audio simulato.
5. Asserire che LIVE sia ancora A e che il clip originario sia in playback.

**Atteso:** nessun PANIC per il solo intento d'import. Il selettore file asincrono non deve influire sul trasporto.

**Rischio infrastrutturale:** il test attraversa `juce::FileChooser::launchAsync`; su runner Windows headless verificare che il selettore non provochi blocco o failure indipendente dal trasporto. In quel caso il risultato rimane **inconcludente**, non classificato come difetto provato.

### T01 — Cell Play e cursore GO

**Fixture:** A con quattro cue e clip PCM disponibile nella cella della cue 4.

1. Armare la cue 2 in A.
2. Eseguire `triggerCellAt(3,0)`, simulando Cell Play della cue 4.
3. Asserire che il cursore della prossima GO rimanga sulla cue 2.
4. Aprire VIEW B.
5. Eseguire `goNext()` e processare il blocco di ack.
6. Verificare che `acknowledgedCueId` sia quello della cue 2.

**Atteso:** Cell Play distinto da selezione/GO di riga; nessun cambiamento accidentale del cursore. Confermare questa policy con gli operatori prima di fissarla come contratto definitivo.

## T03 — task UX su interfaccia nativa

**Fixture:** nove Cuelist con nomi non intercambiabili: `Pre-show`, `Act I`, `Act II`, `Act III`, `Intermission`, `Finale`, `Backup`, `Soundcheck`, `Emergency`. Finestra 1180×820 Windows, scalatura 100% e 150%, almeno una prova con operatore che non conosce The CUElist.

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

**Strategia di rollback:** scartare il branch/PR di esperimento senza toccare `main`. Eventuali fix richiedono un ulteriore gate e un piano di regressione. Nessun merge automatico.


## Evidenza Windows fornita dall'utente — 0.9.32 (2026-11-09)

**Provenienza:** log di compilazione/CTest su Windows fornito dall'utente, non esecuzione da parte dell'assistente. Log originale conservato nella conversazione, non allegato alla PR.

- Configurazione CMake e MSBuild: completata, con `The Cuelist.exe` e `CuelistTests.exe` prodotti.
- CTest: 1 target eseguito (`CuelistTests`), risultato complessivo fallito. All'interno del target: **419.987 asserzioni, 5 test falliti**.
- **T02 fallito:** `T02: opening an empty VIEW cell took ownership of LIVE`. Questo conferma il trasferimento di stato improprio. Il test si interrompe al primo `check` non soddisfatto: non prova autonomamente che l'audio abbia cessato di suonare su un dispositivo.
- **T01 fallito:** `T01: direct Cell Play unexpectedly changed the LIVE GO cursor`. Confermato il disallineamento interno; il test si interrompe prima dell'assert sull'ack del successivo GO.
- **Altri tre fallimenti:** spostamento tab con freccia sinistra, pulsante aggiunta Cuelist, rinomina inline. Questi test seguono T02 nella suite. La causa è **ND**: l'interferenza del dialogo asincrono è un'ipotesi da falsificare, non una conclusione.
- La CI della versione 0.9.31 (PR precedente, SHA `224cce506d8bd6496530daee685c9ead53686259`) risultava positiva nel job Windows consultato; questo rafforza, ma non prova, la correlazione temporale tra T02 e i tre fallimenti.

**Controllo del controesempio:** il riferimento ufficiale JUCE indica che [`FileChooser::launchAsync`](https://docs.juce.com/develop/classjuce_1_1FileChooser.html) apre un dialogo modale e ritorna immediatamente. Spostare T02 alla fine della suite è un esperimento per isolare i test UI seguenti, non una modifica del comportamento dell'applicazione.

## Esperimento di correzione — 0.9.33, PR separata

- In `triggerCell()`, gestire i percorsi di edit/import prima del trasferimento LIVE.
- Su Cell Play con un clip presente nella VIEW, il trasferimento resta esplicito e pubblica il RenderState della nuova LIVE prima del comando.
- In `triggerCellAt()`, preservare `nextCue` e `liveNextCue` durante il Cell Play diretto.
- Eseguire T02 come **ultimo test** della suite per separare gli effetti del `FileChooser` dai test di frecce, pulsante + e rinomina.
- Nuovo test automatico: Cell Play Audio caricato in VIEW trasferisce LIVE e produce playback del clip corretto.

**Gate 0.9.33:** nuovi Windows CI/CTest ND fino alla conclusione; verificare i 5 test precedentemente falliti, il test di trasferimento esplicito e la generazione screenshot. Se falliscono ancora i tre test UI, investigare indipendentemente prima di qualunque merge.

**Rischi operativi:** nessuna misura end-to-end con MIDI/OSC/LTC esterni; nessuna prova di recupero hardware; nessuna evidenza ancora per T03. **Rollback**: mantenere il fix in branch non integrato se i gate non passano.


## Aggiornamento RQ-001 / T03 — 2026-11-09

**Baseline:** `main@895346a32272efd8fb7db338075187d401df38ac`, versione 0.9.33. `VERSION.txt` e CMake coerenti.

### Risultato 0.9.33 acquisito

L'utente ha fornito un log Windows di 0.9.33 con compilazione di `The Cuelist.exe` e `CuelistTests.exe`, CTest concluso con **419.999 asserzioni e zero test falliti**, durata complessiva **5,44 s**. T01, T02 e il test di trasferimento LIVE esplicito risultano PASS. Anche i tre test UI precedentemente rossi passano. È un collaudo locale riportato nel log; il banco hardware esterno non è stato eseguito. Non attribuire il risultato a una prova hardware o a un test dell'operatore.

### T03 predisposto su branch separato

- Regressione nativa a 1180×820 e 1920×1080 con 9 Cuelist: verifica **LIVE nascosta sotto overflow**, VIEW visibile, GO globale destinato comunque alla LIVE corretta, controprova su layout largo.
- Snapshot da veri componenti JUCE in `test-artifacts`, pubblicati nel workflow esistente come `interface-rq001-t03-live-hidden-1180.png` e `interface-rq001-t03-live-visible-1920.png` solo se il job completa il percorso di screenshot.
- Protocollo di misura registrato in `RQ-001-T03-PILOT.md` e CSV vuoto in `RQ-001-T03-OBSERVATIONS.csv`.

**Distinzione critica:** screenshot off-screen (raster 1×) **non** sono screenshot del desktop Windows a DPI 100% e 150%. La quota di risposte corrette, i tempi degli operatori e il comportamento al vero scaling Windows sono **ND**. Per il resto della RQ il prerequisito è misurare prima di modificare l'indicatore LIVE.

**Gate residuo:** PR T03 Windows CI/CTest e immagini, ispezione visiva, scaling reale, pilota con operatori, hardware esterno separato. Nessuna modifica all'applicazione, all'engine o allo schema .cuelist v10 in questa iterazione.
