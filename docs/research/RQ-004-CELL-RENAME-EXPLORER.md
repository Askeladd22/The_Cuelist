# RQ-004 — Cell Rename e Find in Explorer

**Scenario:** l'operatore vuole rinominare la singola cella dalla Cue Grid con **Ctrl+R** o dal menu destro. Vuole individuare in Esplora file il file di una cella **Audio**. Il nome è un'etichetta del progetto; non deve cambiare percorso, nome del file, Control Map o LIVE.

**Baseline verificata:** `main@4b8cc9f27153129fa130531d89edfa4ca7a0bd5d`; `VERSION.txt=0.9.42`. Candidato 0.9.43 sul branch `feat/cell-inline-rename-explorer-0943`. Persistenza JSON **v10** invariata.

## Confronto e fonti

- `src/CueOperations.cpp`: editor JUCE inline già usato per cue, colonna e Cuelist. Esteso con `InlineRenameKind::cell`, identità `cell.id` e selezione dalla VIEW. Enter salva, Esc annulla. Shortcut su `MainComponent::keyPressed`, ignorato dentro un TextEditor.
- `src/CueViews.cpp`: `CueGrid::createCellContextMenu` espone **Rename cell** per Audio/MIDI/OSC/Control e **Find in Explorer** soltanto per Audio. `src/ControlUI.cpp` conserva l'editor Control Mapping e aggiunge Rename nel menu Control.
- `src/ShowModel.h` e `src/ShowModel.cpp`: `cell.name` e `cell.file` sono campi separati già serializzati. Il rename non altera il file, la sorgente PCM o il routing.
- JUCE ufficiale: [File::revealToUser](https://docs.juce.com/master/classjuce_1_1File.html) apre la posizione del file in Explorer su Windows senza eseguirlo; [PopupMenu](https://docs.juce.com/master/classjuce_1_1PopupMenu.html) consente opzioni disabilitate. Sono API di libreria, non prove di esecuzione.
- Approccio scartato: rinominare fisicamente il file sorgente. Romperebbe riferimenti, backup e condivisione del media fra cue.

## Ipotesi misurabile

Con 10 cue e 4 colonne miste, almeno 8 operatori su 10 rinominano una cella in meno di 10 s senza toccare i file. Per Audio con file valido, almeno 8/10 trovano la sorgente in Explorer entro 10 s. **Misure: ND**; servono prove con operatori e file veri, non solo CTest.

## Red team e sicurezza

- **VIEW/LIVE:** rename da menu non seleziona una cue differente o muove NEXT; shortcut modifica soltanto la cella selezionata. I callback asincroni verificano VIEW, ID riga, colonna e cella prima di agire. Le operazioni normali di GO/STOP/PANIC rimangono intatte.
- **Focus:** Ctrl+R non ruba tasti agli editor di testo; Enter/Esc sono gestiti dall'editor inline condiviso. Lo scorrimento porta in vista una cella selezionata fuori viewport.
- **Identity e Undo:** modifica il campo `cell.name` e usa la history esistente; un nome vuoto ripristina il fallback grafico. Nomi custom sono ora visibili anche in celle vuote e Control senza sostituire la semantica delle mappe.
- **Explorer:** menu solo Audio. Voce disabilitata con path vuoto, relativo irrisolto o file mancante. La funzione ricontrolla `existsAsFile()` prima di chiamare `revealToUser()`. Non aprire il media con `startAsProcess()`.
- **Controesempi da misurare:** sorgenti su SMB/dischi rimovibili, permessi Windows, nomi lunghi, colonne scrollate/collassate, shortcut con editor in focus, cambio progetto mentre un popup è aperto. `revealToUser` non restituisce successo/fallimento: verifica Explorer manualmente.
- **Rollback:** revert della PR; schema v10 immutato e nomi preesistenti conservati.

## Test e gate

| Verifica | Stato |
| --- | --- |
| Test JUCE: Ctrl+R, Editor inline, Enter/Esc, persistenza v10, tipi Audio/MIDI/OSC/Control, file invariati, menu contestuale e file assenti/relativi | Aggiunti, esito da CI |
| Windows Release + CTest | ND, workflow da verificare |
| Screenshot app nativa: 100%, 125%, 150% DPI; viewport e colonne strette | ND |
| Explorer Windows con percorso normale, disco esterno e rete SMB | ND |
| Prove operatori e hardware Audio/MIDI/OSC/LTC, nessuna interruzione LIVE | ND |

**Raccomandazione:** PR piccola, non unire senza Windows CI verde e screenshot nativo. Nessuna certificazione di uso live da una compilazione riuscita.
