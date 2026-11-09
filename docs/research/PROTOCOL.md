# The CUElist — protocollo tecnico di ricerca e validazione

**Ambito:** affidabilità e usabilità di un'app JUCE di show control, non game design.

## 1. Definire il problema, non la feature

Ogni richiesta diventa un record **RQ-###**: scenario concreto, conseguenze per l'operatore, ambiente, stato attuale al commit SHA, invariante da preservare, ipotesi falsificabile, evidenza necessaria. Se mancano dati, dichiararlo.

Esempio: «Durante una Cuelist LIVE l'operatore può ispezionare una VIEW diversa senza GO involontario, senza perdita dei riferimenti attivi e con stato sempre identificabile?». La domanda può avere test automatici, test di usabilità e collaudo hardware indipendenti.

## 2. Fonti e priorità

1. **Codice/test/tag SHA** per ciò che The CUElist esegue realmente.
2. **Documentazione ufficiale del prodotto e specifiche normative** per comportamento dei concorrenti e protocolli.
3. **Esperienze di operatori** (forum, interviste, issue, recensioni) per problemi ricorrenti, non per pretendere misura statistica.
4. **Ragionamento di design** come ipotesi, non come riscontro.

Registrare data, versione, URL, autore, sezione e limiti di ciascuna fonte. Una doc di QLab 5 non dimostra comportamento di una versione diversa o equivalenza con Windows. Non fingere di aver provato un prodotto solo leggendo il suo manuale.

## 3. Comparazione controllata e red team

Per un'azione di show control confrontare tempi/numero di passi, errori recuperabili, visibilità LIVE/VIEW, keyboard access, reversibilità, coerenza, carico cognitivo, sicurezza e latenza di feedback. **Hard gate** su perdita di funzioni, GO accidentali, silenzi non previsti, callback audio con allocazioni/blocchi, regressioni schema v10 e routing. Non assegnare voti di mercato basandosi soltanto su screenshot.

Il red team deve cercare: race tra UI e audio thread, device swap, perdita OSC senza acknowledge, MIDI note stuck, LTC receiver che perde lock, recupero parziale, media corrotti, pressione simultanea di GO/STOP/PANIC, font/glyph mancanti e finestra piccola/DPI elevato.

## 4. Dall'ipotesi al test

- **G0 Scoping**: caso e invariante scritti.
- **G1 Evidence**: fonti attendibili e alternative confrontate.
- **G2 Design**: task flow e fallimenti previsti, revisione controargomentata.
- **G3 Automated**: unit/integrazione, project roundtrip, CMake/CTest e screenshot nativi quando pertinente.
- **G4 Bench**: misure di performance/tempi e robustezza in condizioni documentate.
- **G5 Hardware/operator**: prova con dispositivi veri, run lunga e compiti osservati.
- **G6 Decision**: proposta APPROVE / ITERATE / REJECT / DEFER con rischi residui e link al PR.

Non occorre G5 per *ogni* modifica testuale; è obbligatorio prima di dichiarare collaudato il comportamento critico. Una PR può essere accettata prima del G5 **solo** senza affermazioni di certificazione/live readiness e se il rischio è esplicitato.

## 5. Discipline di PR

- Mai modificare main direttamente per fare ricerca; branch per tema.
- Nessuna UI decorativa al posto di un controllo reale; nessuna perdita di feature.
- Preservare audio/MIDI/OSC, Control Map, VIEW/LIVE e semantica esistente salvo modifica approvata.
- Ogni PR incrementa VERSION.txt, mantiene CMake major.minor.patch allineato e aggiunge CHANGELOG.md. **La versione dell'app NON è lo schema .cuelist.**
- Per risultati quantitativi allegare comando, hash, hardware, output grezzi, percentili e outlier. Non riportare numeri inesistenti.
- Nessun badge «live safe», «zero latency» o «certified» senza prova indipendente e criteri dichiarati.
