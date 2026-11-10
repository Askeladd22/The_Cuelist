# RQ-002 — Accesso diretto alle celle Auto Stop dalla fascia cue

> **Nota storica — superata nella 0.9.40:** la soluzione 0.9.38 apriva un menu a tendina. Il nuovo design concordato seleziona direttamente le celle sulla griglia dopo un clic su **+**, senza menu: vedere [RQ-003](RQ-003-AUTOSTOP-INLINE-PICKER.md). La documentazione sotto descrive il prototipo precedente.

**Data:** 2026-11-10  
**Baseline verificata:** `main@57f9413e5790315fdc25aa283cd4dc023e6834f0`, `VERSION.txt=0.9.37`.  
**Candidato:** `0.9.38`, branch `feat/autostop-cell-picker-plus-0938`.  
**Schema progetto:** JSON v10, invariato.  
**Evidenza di prova:** da completare per candidato; non certificato per show live.

## Scenario operatore

Una cue combina più celle (Audio, MIDI, OSC e Control). Con Auto Stop di cue ON, l'operatore deve scegliere quali celle si fermano al prossimo GO. Dopo la rimozione del controllo CELLS in N3.4B, il menu contestuale destro e l'Inspector permettono ancora di cambiare la scelta, ma la fascia Auto Stop non offre un ingresso visibile e diretto. La richiesta è ripristinare **+ sotto il toggle**, senza eliminare l'accesso da menu destro.

## Confronto con l'implementazione preesistente e fonti

- Codice repository: `src/CueViews.cpp` e `src/MainComponent.h` della baseline, toggle cue in `CueMetricsOverlay`.
- Modello/runtime: `MainComponent::setAutoStop` e `setCellAutoStop` in `src/CueOperations.cpp`, `stopOnNext` persistito in `ShowModel.h`.
- Menu contestuali della baseline: `CueGrid::showCellMenu` e `MainComponent::showControlCellMenu` chiamano lo stesso setter.
- Decisione storica, non più vincolante: `docs/UI_V2_NATIVE_PHASE_N3_4B_AUTOSTOP.md` descrive la rimozione completa di CELLS.
- API JUCE ufficiale: `juce::PopupMenu`, voci con spunta e callback d'azione (https://docs.juce.com/master/classjuce_1_1PopupMenu.html).

Il + **non** ripristina l'espansione CELLS e non modifica la logica di trasporto: apre un selettore nativo di celle con spunte, una scelta per apertura. Il menu destro resta operativo. Il lavoro non introduce nuove dipendenze.

## Ipotesi falsificabile

Il + riduce il numero di passaggi necessari per individuare la selezione delle celle Auto Stop rispetto al menu contestuale, senza aumentare clic involontari sul toggle o cambiare il funzionamento della LIVE. Soglia proposta per il pilot: almeno 8/10 operatori trovano l'azione senza indicazioni in meno di 10 s e zero attivazioni accidentali GO/Auto Stop in 30 tentativi; registrare tempi e fallimenti, non considerarli misurati prima della prova.

## Red team e rischio operatore

- **Riga sbagliata con scroll Y:** hitbox calcolata con `CueGridGeometry`, stesso mapping del toggle.
- **BPM/Auto Stop interferiti:** rettangoli distinti; nessun overlay persistente sopra la matrice.
- **Callback asincrona dopo cambio VIEW/riordino:** l'azione controlla VIEW, cue ID, column ID e cell ID prima di chiamare `setCellAutoStop`. Nuovi progetti con ID identici rimangono un controesempio da collaudare manualmente durante una sostituzione mentre il menu è aperto.
- **Spunte inattese:** ogni cella nasce inclusa; il menu mostra lo stato reale di `stopOnNext` e aggiorna le spunte alla riapertura. Il marker CONT. resta visibile sulle celle escluse.
- **Operatori con mouse non preciso:** superficie + da 28×20 px; verificare 125/150% DPI reali e uso con interfacce touch, senza considerare conclusivo il solo pixel test.
- **Moltissime colonne:** verificare leggibilità, overflow e menu con nomi lunghi/duplicati e differente scala Windows.
- **Undo e LIVE:** selezione spuntata usa la stessa operazione editoriale esistente; verificare Undo/Redo con LIVE in riproduzione.

## Piano QA e gate

1. Test unit/integration JUCE: hitbox senza overlap, nessuna mutazione in zona vuota, spunte Audio/MIDI/OSC/Control, toggle singolo indipendente, righe confinanti, aggiornamento dopo riapertura, ID stabili dopo reorder, encoding/decoding JSON v10.
2. Windows Release build + CTest su branch e confronto con baseline; annotare SHA, job e numero asserzioni.
3. Screenshot nativi 1180×820, 1280×768, 1440×900, 1920×1080 e 32:9 ai DPI 100/125/150% (quando disponibili). Verificare tutte le righe, menu aperto vicino all'ultima cue visibile, scrolling e colori di stato.
4. Test con operatori (misure della soglia sopra), includendo qualcuno che non conosce il menu destro. Misurare errori e tempo su almeno due cue con colonne miste.
5. Banco esterno: Auto Stop Audio/MIDI/OSC e LTC mentre viene configurato il set, misurando output effettivo; CI verde non certifica il live.

**Risultati attuali:** implementazione e test aggiunti al branch; compilazione, CI, DPI, UX e hardware: **ND fino a risultato verificato**.  
**Raccomandazione:** mantenere separato da `main` finché CTest Windows e una prova grafica non sono verdi.  
**Rollback:** revert della PR (senza cambiare schema), preservando i flag salvati `stopOnNext`.
