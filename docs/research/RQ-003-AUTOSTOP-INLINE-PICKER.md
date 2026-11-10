# RQ-003 — Selezione Auto Stop direttamente dalla Cue Grid

**Data:** 2026-11-10  
**Baseline:** `main@94babfe00f92826f7d8d393c06c756af0ed5b953`, VERSION.txt 0.9.39, JSON schema v10.  
**Implementazione candidata:** 0.9.40, branch `feat/autostop-inline-cell-picking-0940`.  
**Origine del requisito:** feedback operatore e screenshot nativo Windows 1440×931 (fornito in conversazione), non test con più operatori.

## Scenario, problema e scelta UX

Lo switch ON/OFF è troppo a sinistra nell'area Auto Stop. Il pulsante + della 0.9.38 apre un menu che costringe a scegliere per nome le celle: l'operatore desidera invece premere + e fare clic **sulla cella stessa** per includerla o escluderla immediatamente. Ciò deve funzionare per celle vuote, Audio, MIDI, OSC, Control e colonne collassate, senza avviare la riproduzione né aprire editor.

**Soluzione reversibile:** spostare lo switch a destra nel medesimo spazio fisso e trasformare + in un controllo di modalità con evidenza grafica. La modalità si applica a una singola cue identificata dal suo ID stabile e alla Cuelist VIEW attiva. Il clic su una cella alterna `cell.stopOnNext` attraverso il setter già utilizzato da Inspector e menu destro; ogni evento è applicato e salvabile immediatamente. Una cella inclusa è evidenziata verde con `AUTO`, una esclusa è ambra con `CONT.`. Il contatore sotto lo switch riporta il rapporto incluse/totali, non le celle effettivamente in riproduzione.

Il + chiude la modalità senza aprire alcun PopupMenu. Clic su una cue differente termina la modalità **senza eseguire** azioni sulla cella diversa. Cambio VIEW, caricamento progetto, GO e PANIC chiudono la modalità; Esc continua a eseguire PANIC. Il normale menu contestuale destro è conservato quando la modalità non è attiva.

## Fonti verificabili e confronto

- `src/MainComponent.h` e `src/CueViews.cpp`, versione 0.9.39: fascia Auto Stop, precedente menu del +, geometrie e dispatch del mouse.
- `src/CueOperations.cpp`: `setAutoStop`, `setCellAutoStop`, history/Undo, pubblicazione del rendering Live.
- `src/ShowModel.h` e test `tests/AutoStopV7Tests.h`: flag per cella e formato progetto JSON v10.
- `docs/research/RQ-002-AUTOSTOP-PLUS.md`: variante a menu della 0.9.38, ora superata dalla richiesta esplicita.
- Documentazione JUCE rilevante: `juce::Component::mouseDown` / `mouseDoubleClick`, hit test, `SettableTooltipClient`. La modifica usa gli stessi componenti nativi del progetto; nessun componente HTML.

**Alternativa scartata:** svuotare tutte le spunte entrando in modalità, per poi selezionare soltanto le celle scelte. Farebbe cambiare il comportamento Auto Stop senza un clic consapevole su ciascuna cella, con rischio di scene che continuano. Le celle partono dal loro stato reale, di norma tutte incluse, e un clic ne inverte la partecipazione.

## Ipotesi misurabile

A parità di cue con 6–12 colonne miste, la selezione diretta riduce il numero di azioni e il tempo mediano per configurare tre eccezioni rispetto al menu della 0.9.38. Gate di usabilità proposto: almeno 8/10 operatori completano il task senza suggerimenti; nessun GO o trasporto attivato involontariamente in 30 prove. Valori osservati: **ND**.

## Red team — rischi

- **GO/Play/Stop involontari:** il dispatch intercetta i clic nelle celle mentre la modalità è attiva prima del trasporto, della selezione NEXT o dell'apertura di Control Map. I pulsanti GO/PANIC globali non vengono riutilizzati per scegliere celle.
- **Cue diversa:** off-row click consumato senza mutare una cue differente; riordino usa ID stabile.
- **Celle collassate:** sono intenzionalmente selezionabili anche se l'editing ordinario è bloccato; colore verde/ambra indica la scelta.
- **Stato non evidente:** il + attivo ha un contorno marcato e compare contatore incluse/totali, con marcatori per ogni cella; verificare contrasto e percezione nei DPI reali.
- **Cuelist VIEW ≠ LIVE:** non trasferire il trasporto per una selezione; usa il setter esistente con la semantica LIVE implementata.
- **Auto Stop cue OFF:** i flag per cella restano persistiti, ma avranno effetto soltanto quando il master è ON. Il messaggio UI lo chiarisce.
- **Doppio clic e drag:** disabilitati per la selezione; i test devono garantire che non aprano editor o alterino altre entità.
- **Undo/Redo:** ogni clic percorre `checkpointHistory` nel setter esistente; testare sequenze e reversal senza interferire con la LIVE.
- **Operatività:** in caso di PANIC, uscita dal picker senza impedire l'arresto reale.

## Test, risultati e gate residui

| Verifica | Stato al momento della PR |
| --- | --- |
| Verifica statica delle hitbox, continuità del layout e del dispatch | Implementata, da compilare |
| Test JUCE simulati: tasto +, toggle celle, CONTROL, Audio/Play, collassate, scrolled cue, riordino, JSON v10, Esc | Aggiunti/aggiornati, esecuzione ND |
| Build Windows Release + CTest | ND, CI da verificare |
| Screenshot nativi Windows 100%, 125%, 150%, 1180/1280/1440/1920/32:9 | ND |
| Test con operatori, misure di tempo ed errori | ND |
| Banco esterno Audio/MIDI/OSC/LTC, misure del flusso e delle transizioni | ND |

**Raccomandazione:** non integrare in `main` finché Windows CI/CTest e un'ispezione della UI nativa non siano verificati. Non usare una CI verde come certificazione per show dal vivo. **Rollback:** revert della PR; `cell.stopOnNext` e il JSON v10 non cambiano, menu destro e Inspector restano utilizzabili.
