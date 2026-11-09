# RQ-001 / T03-B — Indicazione esplicita VIEW/LIVE (esperimento JUCE)

**Data:** 2026-11-09  
**Baseline ufficiale letta prima delle modifiche:** `main@68d56b356285b5c25bc0e2dde4ff6ee97dbfed90`, versione `VERSION.txt = 0.9.35` e `project(TheCuelist VERSION 0.9.35)`.  
**Branch:** `research/rq001-t03b-visible-live-identity` (versione di esperimento 0.9.36, non pubblicata né autorizzata al merge).  
**Schema file progetto:** `.cuelist` JSON v10, non modificato.

## Scenario e evidenza di partenza

Nei due screenshot **nativi JUCE 0.9.35 forniti dall'utente** (`interface-rq001-t03-live-visible-1920.png`, `interface-rq001-t03-live-hidden-1180.png`) lo scenario è identico: VIEW `Emergency`, LIVE `Pre-show`, GO globale armato sulla cue 02. A 1180×820 la tab LIVE è nell'overflow; ON DECK mostra `LIVE / 02` senza il nome, mentre la cue 01 della VIEW è etichettata `NEXT CUE`. A 1920×1080 la tab Pre-show è visibile, ma il piccolo punto verde non ha una legenda sempre visibile.

**Distinzione:** sono osservazioni di schermate e sorgenti, NON dati sull'errore umano. Il test JUCE T03 e il log dell'utente sulla 0.9.35 hanno superato la regressione funzionale: 420.023 asserzioni / 0 fallimenti, 14,21 s, inclusa la generazione dei due PNG. Non è un test su hardware esterno.

## Ipotesi falsificabile

**H-T03B:** rispetto alla 0.9.35, un operatore identifica più spesso e/o più velocemente la Cuelist e la cue destinatarie del GO globale quando la UI mostra il nome LIVE anche con tab nascosta e contrassegna la prossima cue della VIEW come `VIEW NEXT`.

**Controipotesi:** la distinzione esistente (`ON DECK`, punto verde e messaggi di stato) è già sufficientemente chiara, e le etichette in più consumano spazio o riducono la leggibilità della UI. Una scritta chiara può anche creare falsa sicurezza se il cursore non è ancora stato confermato dal callback: il testo deve descrivere il target *armato*, non l'evento di uscita MIDI/OSC/LTC realmente eseguito.

## Variante B — intervento nativo minimo

1. **ON DECK:** conservare rettangolo 156×51 px, posizione `h-339`, e i pulsanti GO/PANIC invariati. Prima riga `ON DECK / LIVE`; seconda riga il **nome LIVE** (intero nel modello, ellissi visuale se troppo lungo) e `GO 02` oppure `END`. Il nome completo è disponibile nel menu overflow e nel tooltip del GO globale, non affidarsi a un troncamento come unica fonte.
2. **Header della Cuelist:** lasciare immutati titolo, font e hitbox dell'editor; il testo secondario introduce il prefisso `VIEW / ... cues / ... clips`.
3. **Riga cue:** mostrare `VIEW NEXT` al posto di `NEXT CUE` soltanto per la riga armata nella VIEW **quando VIEW ≠ LIVE**. Non cambiare priorità di `PLAYING`, `PAUSED`, `MUTED` e altri stati.
4. **Tab overflow:** prefisso `LIVE / ` sulla Cuelist nascosta proprietaria del trasporto, senza modificare selezione, ID comando, riordinamento o gestione di rinomina/eliminazione.
5. **Tooltip GO:** esplicitare il nome LIVE completo e il numero della cue armata o la fine della lista; aggiornarlo nel timer UI (25 Hz), senza connessione al callback audio.

Modifiche limitate a componenti JUCE/C++ esistenti e ai test/documentazione: nessuna nuova superficie mockup HTML, nessuna migrazione, nessun accesso realtime dal thread audio. Le etichette indicano la *destinazione del comando*, non conferma di successo di uscite esterne.

## Criteri di verifica automatica

- Ricontrollare il test T03 originario (nove Cuelist, GO da VIEW=8 a LIVE=0, tab nascosta a 1180×820 e visibile a 1920×1080).
- Test T03-B: il nome e il numero LIVE rimangono immutati durante navigazione VIEW; overflow espone un tooltip coerente; GO porta il cursore a `END`; rinomina della LIVE aggiorna nome; tornare alla LIVE e selezionare una cue la ri-arma.
- Produrre 2 nuove immagini off-screen da JUCE con fixture identica alla 0.9.35: `interface-rq001-t03b-live-hidden-1180.png` e `interface-rq001-t03b-live-visible-1920.png`. Mantenere le 2 immagini storicamente denominate T03 nell'output per compatibilità con l'attuale workflow screenshot; **non considerarle copie della UI 0.9.35**, perché vengono rigenerate dal branch.
- Controllare visivamente perdita di etichette, ellissi, allineamenti con il bordo del pannello, visibilità in caso `END`, contrasto della UI Industrial Orange e Legacy.
- Verificare CTest Windows su HEAD del branch, eseguibile generato, test di versioning, screenshot di CI. Test Windows a DPI 100/150 reali rimangono separati.

## Pilota comparativo (non ancora eseguito)

Utilizzare un disegno pilota con **due gruppi controbilanciati** (idealmente 12 operatori, 6 baseline/6 variante al primo contatto, poi crossover con scenari equivalenti ma nomi e numeri cue diversi per ridurre effetto memoria). Confrontare *a parità di larghezza e di DPI*: A 0.9.35 vs B 0.9.36; poi confrontare separatamente le due larghezze.

Domanda: «Se premi ora Space, quale Cuelist e quale cue riceveranno GO?». Misurare tempo prima della risposta in ms, risposta corretta (nome e numero), esitazioni, fiducia 1–5, aperture overflow e richieste di aiuto. Annotare esperienza e ordine nel CSV vuoto `RQ-001-T03B-OBSERVATIONS.csv`, distinguendo `variant` (A=0.9.35, B=0.9.36) da `condition` (compact/wide). **No dati inventati o prove con apparecchi di spettacolo collegati.**

**Gate esplorativo, NON prescritto da ISO:** trattenere il redesign se aumenta anche un solo errore di identificazione, oppure se causa clipping non aggirabile; proseguire alla valutazione con un campione più ampio se la differenza sembra positiva. Un pilota di 12 persone non può certificare la sicurezza dal vivo.

## Red team, rischi per operatore, rollback

- Un nome LIVE lungo fino a 64 caratteri può apparire troncato nel pannello da 94 px: verificare visivamente e usare tooltip/menu per il nome integrale; se il nome rimane difficile da riconoscere, riconsiderare dimensione/geometria in una PR distinta.
- `VIEW NEXT` non deve essere confuso con un comando GO; il cursore LIVE continua indipendente. Gli operatori poco esperti potrebbero interpretare `VIEW NEXT` come «prossima cue del pulsante verde»: questo è un controesempio da indagare.
- Un tooltip non è una prova di accessibilità screen reader; l'accessibilità tastiera/assistive technology richiede collaudo Windows separato.
- Non dedurre sincronizzazione Audio/MIDI/OSC/LTC da CI o PNG.
- Reversibilità: nessun dato salvato cambia. **Rollback:** chiudere la PR senza merge o revert mirato del commit UI; verificare VERSION/CMake/changelog e rifare CTest. Nessuna promozione automatica.

## Risultati e gate

| Voce | Evidenza |
|---|---|
| Baseline T03 comportamento GO | PASS — log Windows utente su 0.9.35 |
| Baseline screenshots 1920/1180 | Ricevuti dall'utente e ispezionati |
| Variante B sorgente JUCE/test | Modificato nel feature branch; build **ND** finché CI non conclude |
| Variante B screenshots e ispezione | **ND** |
| Windows DPI reale e pilota operatori | **ND** |
| MIDI/OSC/LTC hardware e latenze esterne | **ND** |

**Raccomandazione:** non integrare la variante B prima dei gate Windows, revisione screenshot, confronto operatore e controllo di eventuali controesempi. La UI può essere ulteriormente iterata sullo stesso branch, oppure abbandonata senza toccare il prodotto.
