# CHANGELOG — The Cuelist

Tutte le modifiche future devono avere **versione incrementata in
[`VERSION.txt`](VERSION.txt)** e una nuova sezione in testa a questo file.
La versione del progetto `.cuelist` (**schema JSON v10**) è **indipendente**
da quella dell'applicazione. Modalità di aggiornamento:
[`docs/VERSIONING.md`](docs/VERSIONING.md).

Questo changelog è stato introdotto il **31 ottobre 2026** e ricostruito
retrospettivamente da `VERSION.txt`, dalla cronologia Git e dalle PR
effettivamente integrate. **Non esistono GitHub Releases o tag che
certifichino tutte le milestone riportate**; le etichette storiche
indicano versioni di sviluppo presenti nei commit, non distribuzioni
pubbliche. I particolari di ogni intervento sono nell'appendice.

## [0.9.70] - 2026-11-14

**RQ:** [#193](https://github.com/Askeladd22/The_Cuelist/issues/193) — redesign strutturale R1/N4.2 dell'Inspector nativo (Matt Pocock, GStack, Taste/UI-UX, Emil e Unlazy).

### Inspector — prima modifica strutturale
- In `WorkspaceLayout` introdotta una sola interfaccia `InspectorContentLayout` che fornisce rettangoli condivisi a `MainComponent::paint()` e `resized()`. La disposizione dei campi non dipende più da un'AFFINE transform per il testo e da un secondo scaling verticale degli input.
- Le sezioni **Cue Parameters**, **Cell Parameters**, **Cue Notes** usano campi nativi JUCE a grandezza costante, tipografia secondaria leggibile e intervalli più compatti; l'area Note si espande con l'altezza della finestra senza sopprimere azioni MIDI/OSC/Audio.
- Conservati i veri componenti `rowName`, `bpm`, `rowLoop`, `cellName`, `gainDb`, `follow`, `loop`, `midiEditButton`, `oscEditButton`, `importButton` e `cueNotes` e tutti i callback, la validazione e il focus. Aggiunti nomi JUCE stabili ai controlli già esistenti per test e accessibilità.
- **Invarianti:** nessuna modifica a Cue Grid 62/72/318/152, BPM/Auto Stop 184, Master Audio 318, Mixer, Timeline, GO/STOP/PANIC, VIEW/LIVE, Undo/Redo, MIDI/OSC/LTC o file `.cuelist` JSON v10. Nessun mockup HTML.
- Regressioni pure/native in `tests/WorkspaceLayoutTests.h` per layout 1180×820, 1440×900, 1647×895, 1920×1080 e 3840×1080; verifica di Bounds per gli editor reali, geometria, note e parità Show/LIVE/NEXT. Nuove snapshot off-screen JUCE Inspector Audio/MIDI in `tests/Tests.cpp`.
- Specifica e limiti: `docs/research/RQ-UI-R1-N42-INSPECTOR.md`, gate `GATES.md`.

### QA e rollback
- **Da verificare sulla PR esatta:** Windows Release/CTest completo, screenshot nativi della nuova composizione, guard versione/changelog; nessun risultato attribuito prima del workflow.
- **ND:** reale DPI Windows 100/125/150, IME/Narrator, prove A/B operatori, hardware Audio/MIDI/OSC/LTC e sincronismo downstream. Questa prima modifica all'Inspector NON completa il redesign dell'intero workspace né certifica LIVE.
- **Rollback:** revert della PR; nessuna migrazione di progetti e nessuna modifica audio realtime.

## [0.9.69] - 2026-11-14

**PR:** [#192](https://github.com/Askeladd22/The_Cuelist/pull/192) (draft, senza merge automatico) — UX-04C test asincrono + UX-04D Esc/PANIC nella Label del nome progetto (Matt Pocock + GStack + Unlazy + Taste/Emil).

### QA native JUCE
- Il workflow Windows della PR #190, integrata in `main`, compilava correttamente ma falliva con **420.418 asserzioni e un test non superato**: `UX04A inline name Esc cancelled editing but missed PANIC`. Riferimento: [Windows workflow #43575358390](https://github.com/Askeladd22/The_Cuelist/actions/runs/43575358390).
- Diagnosi dal sorgente JUCE: `TextEditor::escapePressed()` invia `postCommandMessage`, quindi la callback `onEscapeKey` è **asincrona**. Il test invocava `TextEditor::keyPressed(Esc)` e controllava lo stato immediatamente, senza un message loop in esecuzione.
- Modificato il test per verificare direttamente i **quattro contratti `onEscapeKey`** della versione 0.9.68 (nome, BPM, marker Timeline, Quick Learn), inclusi annullamento del testo, invarianti Show/NEXT/LIVE e indicatore interno di PANIC. Questo è un test **di callback**, non una prova del percorso fisico Windows Escape.
- Aggiunta una piccola specializzazione **nativa JUCE** della Label del nome progetto: Esc annulla prima la rinomina e chiama il medesimo `MainComponent::stopAll()` tramite callback. Conferma con Enter o perdita del focus continua a salvare senza PANIC. Un nuovo test della callback `Label::textEditorEscapeKeyPressed` verifica un nome lungo a 1180/1920, Show/NEXT/LIVE, indicatore PANIC e conferma senza stop; non simula un evento OS reale. Nessun cambiamento al trasporto, audio thread, MIDI/OSC/LTC o schema JSON v10. La CI della precedente revisione di questa PR era **PASS (420.488 assertions, 0 failed)**; la CI del nuovo commit deve essere verificata separatamente. Hardware, focus fisico, timing e popup modali restano **ND**.
- Aggiornato `docs/research/RQ-UX04-ESC-PANIC-FOCUS.md` con i limiti della prova, il percorso JUCE della Label e il red team sul commit involontario. Per Esc in `PopupMenu`/dialoghi resta aperta l'[issue #191](https://github.com/Askeladd22/The_Cuelist/issues/191).
- **Rollback:** revert della PR, senza migrazione dei progetti; nessun merge automatico.

## [0.9.68] - 2026-11-14

**PR:** [#190](https://github.com/Askeladd22/The_Cuelist/pull/190) (draft, senza merge automatico) — UX-04A Esc/PANIC nei quattro editor JUCE inline; Taste + UI/UX Review, Matt Pocock, Context Engineering, GStack Review, CCteam, SkillRoster, Unlazy.

### Sicurezza operativa — Esc durante l'editing
- I TextEditor per rinomina inline (Cuelist/cue/colonna/cella), BPM nella matrice e marker Timeline usavano `setEscapeAndReturnKeysConsumed(true)`. Esc annullava la modifica senza arrivare al `MainComponent::keyPressed` che invoca PANIC.
- I tre callback `onEscapeKey` già esistenti adesso conservano l'annullamento non distruttivo delle modifiche e richiamano esplicitamente `MainComponent::stopAll()` dopo la chiusura dell'editor. Il red team GStack ha individuato un quarto percorso: la rinomina delle funzioni in Quick Learn (`QuickControlTargets`), che ora richiama PANIC tramite callback esplicita collegata dal componente padre. `Enter` rimane l'azione di salvataggio.
- Aggiornati tooltip e guida. Nuovo test nativo simula `TextEditor::keyPressed(Esc)` nei quattro contesti e verifica testo non salvato scartato, Show identico, selezione NEXT/LIVE conservata e `acknowledgedCueId` azzerato (indicatore del percorso PANIC).
- Rapporto RQ separa fonte ufficiale JUCE, evidenza del codice e ipotesi da misurare: `docs/research/RQ-UX04-ESC-PANIC-FOCUS.md`.

### Ambito e gate
- Nessuna nuova UI, hook globale, modifica al callback audio, GO, OSC/MIDI/LTC, Undo/Redo, Control Map, Mixer, Timeline di playback o progetto JSON v10.
- **Non risolve gli altri popup JUCE modali**, che possono intercettare Esc: audit separato [issue #191](https://github.com/Askeladd22/The_Cuelist/issues/191). I risultati CI di questa PR, i test fisici Windows/IME/Narrator e il banco di periferiche esterne sono **ND** fino a verifica. CI verde non certifica readiness LIVE.
- **Rollback:** revert della PR, senza migrazione e senza modificare lo schema del progetto.

## [0.9.67] - 2026-11-14

**PR:** [#189](https://github.com/Askeladd22/The_Cuelist/pull/189) (draft, senza merge automatico) / [issue #188](https://github.com/Askeladd22/The_Cuelist/issues/188) — UX-03F (Taste + UI/UX Review + Unlazy).

### Leggibilità della conferma Delete
- I veri screenshot JUCE Windows a 1180×820 mostravano troncato `Press D to delete / repeating 5 does nothing` nella testata bloccata, larga circa 210 unità logiche in quella configurazione. Rischio: avviso operativo incompleto nello stato distruttivo.
- Nella modalità Delete armata, la seconda riga usa la dicitura completa `D DELETE / 5 IGNORED` (con spaziatura coerente) quando entra nel rettangolo disponibile; sotto la larghezza necessaria usa `D DELETE` e infine `D`. La misura avviene con `juce::GlyphArrangement::getStringWidth` e il font JUCE di rendering, senza ridurre il carattere o alterare la testata da 72 px.
- L'intestazione e il promemoria Esc/PANIC e Space/GO restano immutati; il nome lungo non influenza la riga di conferma. La descrizione accessibile e la guida conservano la spiegazione estesa di `5` ripetuto.
- Aggiunti test JUCE per larghezza effettiva 1180/1920 e valori stretti sintetici, descrizione completa, gestione `5` ripetuto, Esc/PANIC e Delete per ID stabile; screenshot nativi aggiuntivi dello stato armato con nome colonna lungo.

### Compatibilità e verifica
- Nessuna modifica ai callback di tastiera, GO/STOP/PANIC, VIEW/LIVE, Control Map, Undo/Redo, Mixer, Timeline, Audio/MIDI/OSC/LTC o schema file `.cuelist` JSON v10.
- CI Windows Release/CTest e revisione dei nuovi screenshot **da eseguire** su questa PR; DPI/Narrator, misure operatori e banco hardware restano ND. Nessuna dichiarazione ready-for-LIVE.
- **Rollback:** revert della piccola PR, senza migrazione dati né modifica degli shortcut.

## [0.9.66] - 2026-11-14

**PR:** UX-03E — protezione del gesto multi-click sulla barra di azioni colonna (Taste + UI/UX Review + Unlazy).

### Corretto
- La modalità azioni native `Shift+F10` intercettava il primo `mouseDown` per chiudere la barra, ma il separato `PinnedColumnHeaders::mouseDoubleClick` poteva inoltrare il secondo clic a `CueGrid::doubleClickColumnHeader`, iniziando un Rename sotto la barra appena chiusa.
- Ora l'evento che chiude la barra consuma tutta la medesima sequenza multi-click, compreso `mouseDoubleClick`. Una successiva sequenza indipendente (nuovo clic numero 1) conserva la normale rinomina con doppio clic.
- Regressione nativa aggiunta per mouseDown/doubleClick in sequenza, callback doppio clic diretto, ripristino del Rename, immutabilità Show e identità VIEW/LIVE.
- Nessun nuovo controllo, PopupMenu, listener, timer o cambiamento geometrico/visuale. GO/STOP/PANIC, Control Map, Undo/Redo, Mixer, Timeline, Audio/MIDI/OSC/LTC e schema JSON v10 non modificati.

### QA e limiti
- Diagnosi da sorgenti JUCE e componenti nativi; rischio dedotto dal codice, non riprodotto su hardware Windows. Test e CI nuovi **da verificare**; nessuna certificazione LIVE.
- Rollback tramite revert della PR, senza migrazioni.

## [0.9.65] - 2026-11-14

**PR:** [#186](https://github.com/Askeladd22/The_Cuelist/pull/186) (draft, senza merge automatico) — UX-03D dopo la PR #185 (Taste + UI/UX Review + Unlazy).

### Corretto
- Il workflow Windows [#43526445383](https://github.com/Askeladd22/The_Cuelist/actions/runs/43526445383) della 0.9.64 ha compilato l'applicazione, ma CTest è fallito con **420.393 assertions, 1 failed test**: "UX03C: column keys preserve identity and require D confirmation".
- Il test presumeva erroneamente che dopo `Collapse` la colonna MIDI restasse selezionata. Il codice preesistente `toggleColumnCollapsed()` seleziona la prima colonna espansa; un nuovo `Shift+F10` riguarda correttamente questa nuova selezione.
- Il test verifica adesso la selezione effettiva e l'etichetta della modalità azioni, poi riseleziona esplicitamente la colonna MIDI collassata prima di eliminarla con **5**, quindi **D**. La guida chiarisce il comportamento all'operatore.
- **Nessun cambiamento al codice applicativo:** VIEW/LIVE, GO/STOP/PANIC, audio, MIDI/OSC/LTC, Mixer, interazione mouse e schema JSON v10 invariati.

### QA e rollback
- Diagnosi: log della CI #185 e sorgenti correnti letti; verifica statica del contratto test/documentazione. **Da eseguire:** Windows Release/CTest della nuova PR, screenshot nativi, DPI/Narrator e banco hardware. Non dichiarare ready-for-LIVE.
- PR draft; non fare merge automatico. Rollback: revert della PR, nessuna migrazione.

## [0.9.64] - 2026-11-14

**PR:** [#185](https://github.com/Askeladd22/The_Cuelist/pull/185) (draft, senza merge automatico).

**Stato:** UX-03C (Taste + UI/UX Review + Unlazy) — sicurezza dei comandi globali in modalità azioni colonna.

### Corretto
- La scorciatoia `Shift+F10` non crea più un `juce::PopupMenu` modale: il suo `MenuWindow::keyPressed` interno intercetta `Escape` e chiude il menu prima che l'app riceva PANIC. Il precedente test CTest verificava Escape soltanto dopo aver dismesso il menu.
- La testata `PinnedColumnHeaders` visualizza per 72px una modalità azioni **non modale**, con colonna identificata per nome, scorciatoie 1–5 e promemoria `Esc PANIC / Space GO`; non chiama `grabKeyboardFocus()`, non aggiunge pulsanti né modifica layout o trasporto.
- `5` arma l'eliminazione e soltanto il **diverso** tasto `D` la conferma: la ripetizione di 5 non può cancellare una colonna. L'ID colonna viene verificato prima di qualsiasi modifica; 1–4 riusano Rename, Reorder e Collapse senza duplicare la logica.
- Le normali azioni mouse, le altre scorciatoie e il comportamento dei progetti `.cuelist` schema JSON v10 rimangono invariati.

### QA e limiti
- Test nativi aggiunti per modalità, Esc/PANIC, Space/GO, rinomina/riordino/collasso, double-key delete e target eliminato. Windows CTest e screenshot della PR sono gate da verificare e non prove già effettuate.
- Gate Unlazy in `GATES.md` distingue controlli di codice/CI dal banco manuale Windows DPI, Narrator e periferiche hardware.
- **Rollback:** revert PR; nessun cambio audio callback, MIDI/OSC/LTC o schema JSON.

## [0.9.63] - 2026-11-13

**PR:** [#184](https://github.com/Askeladd22/The_Cuelist/pull/184) (draft, senza merge automatico).

**Stato:** UX-03B — accesso da tastiera alle azioni della colonna selezionata, senza aggiungere focus ai controlli di trasporto o cambiare layout JUCE.

### Migliorato
- `Shift+F10` apre un `juce::PopupMenu` nativo e navigabile da tastiera per la colonna selezionata: Rename (solo se espansa), Move left/right, Collapse/Expand e Delete. Le voci disabilitate indicano le operazioni non disponibili. Nessuna azione si esegue all'apertura del menu.
- Il callback asincrono recupera la colonna tramite ID stabile prima di eseguire gli handler già esistenti; una colonna riordinata o eliminata mentre il menu è aperto non deve provocare un intervento sul target sbagliato.
- Metadati descrittivi JUCE (`setTitle`, `setDescription`) sulla testata colonne, senza trasformare la testata in un pulsante con focus. Il tasto Space continua ad appartenere a GO, Esc a PANIC e le scorciatoie non vengono intercettate negli editor di testo.
- Guida utente aggiornata e regressione nativa sul solo shortcut, sulla conservazione di Show/VIEW/LIVE e sulla propagazione di Esc. Non viene cambiato lo schema `.cuelist` JSON v10.

### Evidenza, red team e gate
- Il workflow della PR #183 (baseline 0.9.62) è riuscito. Per UX-03B la compilazione Windows Release e tutti i CTest sono **da verificare** sulla nuova PR.
- Red team: Shift+F10 va testato con IME/layout tastiera Windows, editor di testo focalizzati, colonne fuori viewport e menu dismessi; i metadati JUCE non attestano lettura effettiva da Windows Narrator. Verify con hardware esterno resta indipendente dalla CI.
- **ND:** DPI reali, misure di errore operatori e screen reader, hardware Audio/MIDI/OSC/LTC, sincronismo e readiness LIVE. Revert della PR per rollback senza migrazioni.

## [0.9.62] - 2026-11-13

**PR:** [#183](https://github.com/Askeladd22/The_Cuelist/pull/183) (draft, senza merge automatico).

**Stato:** UX-03A — maggiore precisione e descrizioni contestuali dei comandi nelle intestazioni JUCE delle colonne; nessun redesign o modifica ai trasporti.

### Migliorato
- Le aree di click di Move Left/Right e Delete nelle intestazioni delle colonne espanse passano verticalmente da 19 a 24 unità logiche (y=8–32); i glifi restano 18×19 e mantengono 2 unità di separazione orizzontale. La prudenza per Delete prevale su hitbox da 24 unità in larghezza che si sovrapporrebbero alle frecce vicine.
- Il controllo Collapse usa una hitbox di 28×26 unità logiche (prima circa 26×21) senza ridurre la zona di rinomina e senza spostare l'icona. Le zone di click e le descrizioni hover condividono un unico helper.
- Tooltip operativi espliciti `Move [name] left/right`, `Delete [name]`, `Collapse/Expand [name]`; le frecce non disponibili descrivono il limite anziché simulare un'azione. La normale area del titolo conserva il nome completo.

### Verifiche e rollback
- Regressione nativa in `CueGridN3VisualTests.h`: tooltip specifici, hitbox aggiunte, assenza di click nel gutter verticale, invarianti VIEW/LIVE e colonne, hover Expand dopo collasso.
- La CI della [#182](https://github.com/Askeladd22/The_Cuelist/pull/182) era verde prima della baseline. Per questa PR, build Windows Release/CTest e screenshot sono gate da verificare, non risultati già misurati.
- **ND:** Windows DPI 100/125/150 reali, tastiera/Narrator, test operatori con pressione su X/Move, hardware Audio/MIDI/OSC/LTC e sincronismo. Non certifica readiness LIVE.
- **Rollback:** revert della PR, senza migrazione dei progetti JSON v10.

## [0.9.61] - 2026-11-13

**PR:** [#182](https://github.com/Askeladd22/The_Cuelist/pull/182) (draft, senza merge automatico).

**Stato:** correzione della compilazione del test diagnostico UX-02 (PR #181, versione 0.9.60); nessun cambiamento al codice applicativo.

### Corretto
- GitHub Actions [43514560543](https://github.com/Askeladd22/The_Cuelist/actions/runs/43514560543) ha fallito in compilazione: `check(bool,const char*)` non accetta un messaggio `juce::String`; MSVC C2664 su tre assert di `tests/UiRefinementTests.h`. Le verifiche di runtime non sono state eseguite in quel workflow.
- Un'unica funzione locale nel test `expectSafetyTooltip(phase)` mantiene un `juce::String` diagnostico valido fino alla chiamata sincrona di `check(..., diagnostic.toRawUTF8())`. La condizione e il tooltip di sicurezza non sono indeboliti.
- `VERSION.txt` e CMake aggiornati a 0.9.61, schema JSON v10 invariato.

### Gate e rollback
- Verifica statica delle firme `check(bool,const char*)` e degli assert aggiornata. **ND:** build Windows/CTest nuova fino al suo run, tooltip reale su Windows, screenshots e DPI, hardware esterno.
- Se CTest ancora fallisce, il log riporterà il testo letto effettivamente da JUCE; non unire la modifica automaticamente.
- Rollback tramite revert della PR.

## [0.9.60] - 2026-11-13

**PR:** [#181](https://github.com/Askeladd22/The_Cuelist/pull/181) (draft, senza merge automatico).

**Stato:** UX-02: follow-up dell'errore CTest sul tooltip di Import nella 0.9.59, senza ampliamento del perimetro a UX-03 o al trasporto.

### Corretto
- L'esecuzione Windows locale fornita dall'operatore (0.9.59) compila, ma fallisce la sola verifica `UX02: neutral audio import does not borrow the transport GO role`, specificamente `Import no longer warns that it stops active playback`: 420352 assert e un test fallito. Il sorgente assegna già un tooltip nel costruttore; il motivo dell'assenza nella lettura del test non è ancora misurato.
- L'avviso di sicurezza rimane una singola stringa canonica `audioImportTooltip`, applicata all'inizializzazione e riaffermata dopo il refresh LookAndFeel, affinché il cambio tema non possa lasciare Import senza spiegazione della conseguenza operativa.
- Le asserzioni JUCE indicano ora il testo realmente letto dal controllo in caso di fallimento e verificano la presenza dell'avviso in tema Industrial Orange, Legacy e nel ritorno a Industrial Orange.

### Gate e rollback
- Ispezione statica su codice JUCE e test eseguita; compilazione Windows, CTest, screenshot e prove a DPI 100/125/150 sono ancora da eseguire per questa PR. Se il test resta rosso, il nuovo messaggio è una prova diagnostica, non autorizza il merge.
- Il runtime di Import e le azioni GO/STOP/PANIC, MIDI/OSC/LTC e il formato progetti JSON v10 non cambiano.
- **Rollback:** revert integrale della PR, senza migrazione dati.

## [0.9.59] - 2026-11-13

**PR:** [#180](https://github.com/Askeladd22/The_Cuelist/pull/180) (draft, senza merge automatico).

**Stato:** hotfix UX-02: correzione della regressione CI introdotta in 0.9.58; invariati schema JSON v10, trasporto e controllo show.

### Corretto
- PR #179 compilava, ma il workflow Windows [43510860209](https://github.com/Askeladd22/The_Cuelist/actions/runs/43510860209) ha fallito CTest: `UX02: neutral audio import does not borrow the transport GO role` (1 test fallito su 420351 assert; screenshot CI non caricati).
- `MainComponent.cpp`: i colori JUCE espliciti del pulsante Import sono ora applicati dopo il caricamento della preferenza di tema; `setAppearanceTheme()` conferma la semantica grafite/testo anche dopo il rimapping. Nessun cambio di callback, tooltip, ordine delle operazioni di importazione, GO o PANIC.
- `tests/UiRefinementTests.h`: i singoli aspetti del contratto (etichetta, tooltip di stop, sfondo, testo, differenza rispetto a GO, transizione Legacy/Industrial e modello persistito) sono verificati separatamente per rendere diagnostici gli eventuali fallimenti.

### Test, rischi, rollback
- Il fallimento CTest della 0.9.58 è **misurato**. La causa proposta è la sequenza di inizializzazione dei colori prima del ripristino delle preferenze; deve essere confermata dalla nuova CI.
- Gate da verificare: Windows Release e tutti i CTest, screenshot native GUI dopo il build; DPI nativi e hardware esterno ND. CI verde non indica readiness LIVE.
- Revert della PR ripristina il precedente comportamento; nessuna migrazione dati.

## [0.9.58] - 2026-11-13

**PR:** [#179](https://github.com/Askeladd22/The_Cuelist/pull/179) (draft, senza merge automatico).

**Stato:** UX-02A, primo incremento di contrasto e semantica cromatica per i controlli di editing JUCE. Nessuna modifica al trasporto, alla disposizione dei pannelli o allo schema JSON v10.

### Migliorato
- `Import WAV / MP3` nell'Inspector è ora un controllo neutro (pannello grafite + testo primario) anziché un pulsante verde indistinguibile da GO. Resta la stessa action `importDialog()` e il tooltip avverte che l'importazione arresta il playback.
- Il token Industrial Orange per il testo attenuato attivo passa da `#718087` a `#99A6AB`, mantenendo il token `disabled` distinto e la palette Legacy invariata. Contrasto teorico su superfici opache `#303B40`: circa 2,82:1 → 4,60:1.
- Le istruzioni delle celle vuote, anche nelle colonne collassate, non vengono più rese con alpha 0,72/0,78: usano il testo attenuato leggibile (o il testo secondario in hover). Su righe di colore personalizzato rimane la selezione dell'inchiostro tramite `readableInk()`.
- Regressioni sul contratto dei token/caption e sul vero `TextButton` Import prima/dopo cambio tema (inclusa conservazione di tooltip, callback esistente e dati Show).

### Rischi, QA, rollback
- Risultati calcolati sui token a tinta piena non equivalgono a una prova di contrasto su tutti gli stati rasterizzati e tutte le superfici custom, le quali possono avere contrasti inferiori a 4,5:1. Verificare screenshot nativi, testi a DPI 100/125/150%, stati cue selezionati/hover, tema Legacy.
- Gate CI: Windows Release e CTest (non dichiarati eseguiti prima del run). Operatori, screen reader, hardware Audio/MIDI/OSC/LTC e prestazioni realtime: ND.
- **Rollback:** revert della PR, senza migrazione di dati.

## [0.9.57] - 2026-11-13

**PR:** [#178](https://github.com/Askeladd22/The_Cuelist/pull/178) (draft, senza merge automatico).

**Stato:** UX-01, testata Cue Grid JUCE con destinazione LIVE/GO persistente; intervento di presentazione e test, senza modificare trasporti o lo schema JSON v10.

### Migliorato
- La testata nativa della Cue Grid mantiene il nome della Cuelist visualizzata e usa una seconda riga informativa `LIVE L01–L12 / nome / GO nn` (oppure `END`). Anche con tab LIVE nascosta nell'overflow, la destinazione di Space è leggibile accanto alla VIEW.
- Terza riga di metadati con i contatori `VIEW / n cue / n clip`, già presenti prima; altezza 72px, geometria di righe, colonne, tab e pulsanti immutata. Nessun nuovo pulsante e nessuna semantica trasporto aggiunta.
- Ambra per il GO *armato* e la destinazione LIVE: il verde continua a identificare GO e playback, senza implicare che una cue sia già in riproduzione.
- Nuova regressione JUCE T03/UX01 che confronta gli snapshot del pinned header prima/dopo rename LIVE e GO/END, e controlla identità VIEW/LIVE e contatori.
- Aggiunta la variante e il protocollo A/B al documento `docs/research/RQ-001-T03-PILOT.md`.

### Test e gate
- **Eseguito:** analisi di sorgenti e geometrie, controllo statico dei consumer e invarianti, preparazione del test di regressione; Windows Release/CTest e snapshot aggiornati sono gate CI della PR.
- **ND:** tempi/errori di operatori, rendering a Windows DPI 100/125/150%, chiusura di show hardware Audio/MIDI/OSC/LTC, misure di sincronismo, scenari di show dal vivo. Nessuna certificazione LIVE.
- **Rollback:** revert della PR; nessuna migrazione dati.

## [0.9.56] - 2026-11-13

**PR:** [#177](https://github.com/Askeladd22/The_Cuelist/pull/177) (draft, senza merge automatico).

**Stato:** aggiornamento esclusivamente documentale; nessun cambiamento ai sorgenti runtime o allo schema `.cuelist` JSON v10.

### Corretto
- Rettificato il significato delle vecchie voci del changelog che indicavano `draft`: erano lo stato delle PR al momento della loro redazione, **non lo stato corrente**. La verifica diretta su GitHub attesta ora l'integrazione su `main` di tutte le PR [#170](https://github.com/Askeladd22/The_Cuelist/pull/170)–[#176](https://github.com/Askeladd22/The_Cuelist/pull/176). Le voci storiche sono conservate come richiesto da `docs/VERSIONING.md`, senza riscrittura retroattiva.
- `docs/refactor-roadmap.md` esplicita il proprio carattere storico (espressioni `current PR`, `next stage` e CI ancora da verificare) e registra la mappa S1–S8, le PR integrate, la regola di arresto della pulizia mirata e i gate residui.
- Separata la conclusione della campagna di semplificazione dall'idoneità LIVE: nessun successo CI certifica dispositivi esterni, sincronismo UDP/MIDI/LTC, recupero fault, DPI o licenze.

### Verifiche e gate
- **Verificato:** GitHub API, sette PR #170–#176 integrate. La CI Windows Release/CTest per il commit della #176 risulta `completed/success` (run [43503240808](https://github.com/Askeladd22/The_Cuelist/actions/runs/43503240808)); controllo statico della coerenza delle note con la versione.
- **Da verificare per questa PR:** version/changelog guard, Windows build/CTest, review dei documenti. **ND:** verifica manuale DPI, pulsanti di chiusura delle finestre, hardware Audio/MIDI/OSC, ricevitore LTC, misure realtime e licenze.
- **Rollback:** revert di questa PR documentale; nessuna migrazione dati.

## [0.9.55] - 2026-11-13

**PR:** [#176](https://github.com/Askeladd22/The_Cuelist/pull/176) (draft; nessun merge automatico).

**Stato:** unificazione della classe JUCE per le finestre staccabili MIDI/OSC, senza modifiche ai protocolli, allo stato LIVE, alla matrice cue o allo schema JSON v10.

### Semplificato
- Eliminati `FloatingPianoRoll` da `MidiUI.cpp` e `FloatingOscWindow` da `OscUI.cpp`: entrambi duplicavano l'identica callback di chiusura del `juce::DocumentWindow`.
- Introdotto il solo `FloatingClipWindow` in `src/FloatingClipWindow.h`. Il titolo è passato dal chiamante; l'ownership dell'editor, i diversi formati delle finestre (1000×620 MIDI, 1000×500 OSC), i limiti di resize, le funzioni Detach/Dock/Close e `SafePointer` restano invariati.
- Aggiunte asserzioni JUCE per il titolo nativo MIDI e per la sequenza OSC Detach/Dock/Close mantenendo la stessa istanza dell'editor.

### Verifiche e gate
- Ispezione statica delle due vecchie classi, consumer, titoli, geometrie e callback: eseguita. La regressione comportamentale è affidata a Windows Release e CTest CI: ND prima dell'esecuzione.
- Screenshot DPI, interazione manuale col pulsante chiusura nativo e hardware Audio/MIDI/OSC/LTC: ND. Green CI non certifica readiness LIVE.
- **Rollback:** revert della PR; nessuna migrazione dati.

## [0.9.54] - 2026-11-13

**PR:** [#175](https://github.com/Askeladd22/The_Cuelist/pull/175) (draft, senza merge automatico).

**Stato:** unificazione della ricerca della cella per Undo/Redo MIDI e OSC fra Cuelist visualizzata e Cuelist inattive; formato progetti JSON v10 invariato.

### Semplificato
- Un nuovo metodo privato `findStoredClipCell(id,type)`, in `CellOperations.cpp`, possiede la ricerca in ordine della cella tramite ID stabile: prima la VIEW corrente (`show.rows`), quindi le altre Cuelist. Restituisce la cella e se appartiene alla LIVE.
- `restoreMidiClip()` e `restoreOscClip()` riutilizzano lo stesso lookup, conservando integralmente le proprie differenze di assegnazione, aggiornamento editor, status, PANIC esclusivamente sul LIVE e pubblicazione dello stato runtime.
- Due regressioni JUCE sul seam reale: Undo/Redo MIDI dopo cambio VIEW e ripristino OSC in Cuelist inattiva mentre una cue Audio continua in LIVE, compreso il no-op se il tipo della colonna non corrisponde.

### Verifiche e gate
- Confronto statico del traversal identico prima delle modifiche, ricerca dei consumer e ispezione del diff: eseguiti.
- Build Windows, CTest e workflow della PR: ND fino alla loro esecuzione. Test fisici Audio/MIDI/OSC/LTC e screenshot DPI: ND. Una CI verde non implica idoneità LIVE.
- **Rollback:** revert della PR; nessuna migrazione dati.

## [0.9.53] - 2026-11-13

**PR:** [#174](https://github.com/Askeladd22/The_Cuelist/pull/174) (draft, nessun merge automatico).

**Stato:** semplificazione del dialogo di import/export Control Mapping, senza cambiare i due formati, le operazioni del modello, le assegnazioni LIVE o il progetto JSON v10.

### Semplificato
- `ControlUI.cpp` usa `exportJsonFile()` e `importJsonFile()` del pannello JUCE per gestire in un solo posto chooser asincrono, percorso predefinito, lifetime `SafePointer`, annullamento, salvataggio e controllo preventivo dei file importati oltre 256KB.
- Restano distinti `importControlPresetText()` e `importControlMap()`: il primo aggiorna Default Map e conserva le etichette, il secondo importa una nuova Map indipendente senza riassegnare celle. Restano anche le rispettive conferme ed errori.
- Nuovo test di separazione dei formati legacy preset e standalone Control Map, con rigetto transazionale dell'import nel formato sbagliato.

### Verifiche e gate
- Confronto statico dei quattro callback e delle rispettive azioni pre/post: eseguito. Build Windows, CTest e CI della PR: ND fino alla loro esecuzione.
- GUI nativa FileChooser (annullamento, percorsi locali, errori I/O, selezione file >256KB) e dispositivi hardware MIDI/OSC: ND. Non cambiano AudioEngine, GO/PANIC o JSON v10.
- **Rollback:** revert della PR, senza migrazione dati o sostituzione di progetti.

## [0.9.52] - 2026-11-13

**PR:** [#173](https://github.com/Askeladd22/The_Cuelist/pull/173) (draft, senza merge automatico).

**Stato:** refactoring mirato della serializzazione JSON v10 dei Control Binding, senza migrazione dati o modifiche alle funzioni di show control.

### Semplificato
- Introdotto un solo `controlBindingObject()` e un solo `parseControlBinding()` in `ShowModel.cpp`: Default Map, Control Set e route delle funzioni astratte usano lo stesso contratto dei 22 campi.
- Rimossi i due writer e due reader paralleli. Ordine dei 22 campi, tipi, validazione esistente, fallback dei campi opzionali `curve`/`functionId` e transazione `decodeShow()` conservati.
- Nuovo test nativo di parità JSON tra Default Map e Control Set, valori non-default, round-trip, documenti v10 senza campi opzionali e rifiuto atomico di un valore malformato.

### Verifiche e gate
- Verifica statica preventiva dei 22 campi nei due serializer e nei due parser, e confronto post-refactoring dei consumer. **Build e CTest Windows / CI:** ND fino all'esecuzione della PR.
- Prove con progetti reali, hardware MIDI/OSC, screenshot DPI e prestazioni/audio realtime: ND; una CI verde non certifica uso LIVE.
- **Rollback:** revert della PR. Nessuna migrazione dello schema `.cuelist` JSON v10.

## [0.9.51] - 2026-11-13

**PR:** [#172](https://github.com/Askeladd22/The_Cuelist/pull/172) (draft; senza merge automatico).

**Stato:** semplificazione focalizzata del reset Quick Learn, senza cambiamenti deliberati al trasporto, agli ingressi MIDI/OSC, all'Audio Engine o allo schema `.cuelist` JSON v10.

### Semplificato
- Estratto `clearQuickLearnSessionState()` come singolo proprietario dell'azzeramento dei flag di cattura MIDI/OSC, della selezione/pulsazione Quick Learn, del feedback e della visibilità dei tre controlli di sessione. Lo stesso blocco era duplicato in `resetActiveMapping()` e `finishQuickLearn()`.
- Mantenute le differenze operative: `finishQuickLearn()` esegue ancora `refresh()` e `updateQuickLearnVisuals()`, senza azzerare l'identità della cue LIVE; `resetActiveMapping()` continua invece a ripristinare `activeCueId` e `controlMapping.setActiveCue(0,0)`, senza nuovi refresh.
- Non toccati i selettori della Control Map, le associazioni persistenti, `learnedInput`, gli edge MIDI fisici, i listener UDP, GO e PANIC.

### Verifiche e gate
- Ispezione del codice: verificata l'identità esatta del blocco di stato prima della sua estrazione. Aggiunta regressione JUCE al seam pubblico di Quick Learn: annullamento tramite secondo click del protocollo e PANIC tramite Escape devono disarmare l'apprendimento senza inviare GO né creare binding.
- Controlli statici del nuovo codice e diff: eseguiti. Test JUCE Windows Release/CTest e CI: ND fino all'esecuzione; test fisici MIDI/OSC, prestazioni realtime e verifica DPI: ND.
- **Rollback:** revert della PR. Nessun dato migrato; schema dei progetti JSON v10 invariato.

## [0.9.50] - 2026-11-13

**PR:** [#171](https://github.com/Askeladd22/The_Cuelist/pull/171) (draft, nessun merge automatico).

**Stato:** consolidamento del menu Settings nascosto nella navigazione JUCE già esistente; schema .cuelist JSON v10 e comportamento LIVE invariati.

### Semplificato
- Eliminata la seconda implementazione di costruzione e dispatch del popup Settings mantenuta dal pulsante nascosto `preferencesButton`. Il vecchio callback resta disponibile come adattatore verso la stessa funzione `showOperatorMenu` dei menu nativi.
- Il callback legacy compone, attraverso l'unico builder corrente, i sottomenu Show, Cuelists, Devices e View: Pre-flight/Diagnostics, rinomina Cuelist, Audio/MIDI/OSC, aspetto, fader e orologio restano raggiungibili. A finestra stretta l'ancoraggio utilizza l'overflow More.
- Aggiornata la specifica dei menu e rimossa dal manuale l'indicazione `Settings → Appearance` non più presente come controllo visibile.

### Verifiche e rischi
- Aggiunto un test strutturale nel controllo dei menu nativi per impedire il ritorno di un secondo popup e confermare i quattro sottomenu di compatibilità.
- **Red/Green statico:** eseguito sul contratto del callback prima e dopo la modifica; ispezione del diff e controllo della persistenza dei comandi richiesti.
- **Windows Build, CTest e CI della PR:** ND fino a esecuzione. GUI reale, tastiera, larghezze compatte, device MIDI/OSC e screenshot DPI: ND; banco hardware e timing realtime: ND.
- **Nota compatibilità:** la struttura del solo popup attivato dal pulsante nascosto passa da voci dirette a quattro sottomenu; i comandi e i menu ordinari restano gli stessi.
- **Rollback:** revert della PR. Nessuna migrazione di dati e nessuna modifica allo schema JSON v10.

## [0.9.49] - 2026-11-13

**PR:** [#170](https://github.com/Askeladd22/The_Cuelist/pull/170) (draft, senza merge automatico).

**Stato:** semplificazione circoscritta delle superfici OSC non raggiunte e del manifest di compilazione CMake. Nessuna modifica voluta al comportamento operativo, al runtime realtime o allo schema .cuelist JSON v10.

### Semplificato
- Rimossa la funzione privata non chiamata `MainComponent::startOscControlInput(int)` (dichiarazione e implementazione). Restano invariati `configureOscInputPorts`, il ripristino delle porte OSC salvate e il listener `OscControlInput` in uso.
- Consolidato in `CUELIST_GUI_SOURCES` l'elenco identico dei 18 sorgenti JUCE condivisi tra eseguibile e `CuelistTests`. `src/Main.cpp` rimane esclusivo dell'applicazione; `tests/Tests.cpp` esclusivo dei test.

### Verifiche e gate
- Ispezione statica: prima del taglio la funzione rimossa era dichiarata e definita una sola volta, senza chiamanti individuati dall'indice del repository. Confrontati gli elenchi CMake: stesso insieme di file compilati per entrambi i target.
- Test locali statici sul diff e ricerca residui: eseguiti prima di aprire la PR; nessun test native Windows dichiarato qui.
- **Build Windows, CTest e CI della PR:** ND fino a esecuzione. Prove hardware OSC/MIDI, screenshot nativi DPI e timing end-to-end: ND.
- **Rollback:** revert della PR. Nessun dato o progetto migra; formato JSON v10 invariato.

## [0.9.48] - 2026-11-11

**Stato:** correzione mirata dei test nativi della Timeline e della telemetria Click Track; nessun cambiamento al motore Audio, ai trasporti o al formato dei progetti (.cuelist JSON v10).

### Correzioni dei tre test Windows
- Il test di doppio clic sul **nome del Marker** individua il testo usando la larghezza reale della Timeline dopo `changed() → refresh() → resized()`; non presume più che rimanga a 800 px.
- Il test di trascinamento **Snap OFF** individua il glyph del marker alle coordinate effettivamente ridisegnate e confronta lo spostamento in secondi con la scala corrente. La posizione della clip PCM deve rimanere invariata.
- Il test **Click Track LIVE dopo un Tempo Marker** imposta `monitoredRowId` prima di interrogare `rowPlaying` e `playheadSeconds`: entrambi sono indicatori della cue monitorata, non stati globali delle voci Audio. Verifica anche l'avanzamento del clock dopo il retiming.

### Evidenze e gate
- **RED acquisito dal log locale dell'operatore:** MSBuild Release compilato, CTest fallito per 3 test su 420291 assert. Riproduzione e cause ricostruite nel sorgente.
- **GREEN Windows/CTest:** ND fino a nuova esecuzione; misure di trasporto realtime, screenshot DPI e banco esterno: ND. La sola correzione dei test non certifica readiness LIVE.
- **Rollback:** revert della PR. Nessun dato o file di progetto migrato.

## [0.9.47] - 2026-11-11

**Stato:** hotfix candidato per la configurazione del Click Track. Schema JSON `.cuelist` v10 invariato.

### Correzione del rischio NEXT/LIVE
- Il comando contestuale **Generate/Configure Click Track** non invoca più `select()`: aprire e confermare il dialog non arma un'altra cue in NEXT e non cambia la selezione VIEW.
- Il salvataggio della configurazione passa per una transazione nativa separata: verifica la cella, applica prima eventuali modifiche pendenti dell'Inspector, convalida il modello, quindi pubblica un nuovo snapshot Audio con `preservePlayback=true`.
- Test di regressione: modifica del Click Track in un'altra cue senza cambio di VIEW, NEXT, LIVE o stop di audio già in riproduzione. Verifica del rifiuto per una cella Audio occupata.

### Gate residui
- Build e CTest Windows, screenshot nativi a DPI multipli, stress audio, misura jitter e banco fisico Audio/MIDI/OSC/LTC: **ND** finché non verificati. Non usare su palco come build certificata.
- Rollback: revert della PR, senza migrazione JSON; gli output CLICK persistiti richiedono comunque un eseguibile compatibile con il campo opzionale `click`.

## [0.9.46] - 2026-11-11

**Stato:** implementazione candidata della generazione Click Track, senza merge automatico. Schema `.cuelist` JSON **v10** invariato.

### Click Track procedurale
- Menu contestuale di una cella Audio vuota: **Generate Click Track...**; su clip già generata: **Configure Click Track...**.
- Dialog JUCE nativo: Classic Click, Woodblock, Electronic Beep, Rimshot; accento attivabile per primo movimento di battuta; subdivision 1/2/3/4; durata indipendente 0,1–600 secondi.
- Traccia sintetizzata direttamente nel motore come eventi musicali e one-shot preparati prima della callback. Nessun WAV o MIDI esterno, nessun nuovo fader nel dialog.
- Impulsi calcolati con la stessa funzione `secondsAtBeat` della Timeline, compresi marker BPM in tempi assoluti non allineati al beat. Metriche della Tempo Map usate per gli accenti.
- Parametri opzionali della sorgente Audio salvati/ripristinati nel JSON v10; normale routing cell/track/Master/Direct Out, mute, pan, gain, offset e loop invariati.
- Modifiche BPM e Tempo Marker ricostruiscono lo schedule di click preservando il cursore LIVE; Undo/Redo dei soli parametri click non invia PANIC. Clip dimensionabile anche dalla Timeline.

### Verifiche e limiti
- Test aggiunti: validazione e round-trip v10; suddivisioni e accenti; marker frazionario; uscita PCM; continuità LIVE dopo retiming.
- Windows Build/CTest, screenshot nativi DPI, misure di jitter e hardware Audio/MIDI/OSC/LTC: **ND** finché non verificati. CI verde non certifica idoneità LIVE.
- Rollback: revert della PR, riaprendo i progetti v10 senza sorgenti click solo dopo aver rimosso o convertito le clip procedurali. I vecchi writer v10 non conoscono il campo opzionale `click`.

## [0.9.45] - 2026-11-11

**Stato:** hotfix candidato della Timeline Markers integrata in 0.9.44; mantiene lo schema JSON v10 e la semantica di trasporto LIVE senza STOP/PANIC per i cambi di marker/BPM.

### Correzioni native e regression test
- La corsia marker e i relativi target di doppio clic/drag restano accessibili anche nelle dimensioni compatte della Timeline (altezza 120–169 px); le corsie media rimangono nascoste quando non c'è spazio per mostrarle senza overlap.
- Doppio clic, menu e trascinamento riconoscono anche la porzione di **nome visibile** del marker, dando precedenza al glyph se più marker sono vicini. Evitata la creazione accidentale di marker duplicati quando si tenta di rinominarne uno.
- Adeguato un test storico del drag MIDI alle coordinate delle nuove track, dopo l'introduzione della corsia marker. I restanti controlli di Snap, GRID, cue e clip restano nativi JUCE.
- Undo/Redo di modifiche non strutturali conserva la selezione VIEW di cue e colonna quando gli ID originari esistono ancora.
- Nuovi test di regressione per marker compatti, doppio clic sul nome, movimento con Snap OFF e invariabilità dell'offset Audio.

### Verifica
- Versione 0.9.44 già integrata in `main` attraverso PR #165. Sul commit della PR l'azione Windows Release and all tests ha riportato **failure**; dettaglio del log remoto: **ND (download non riuscito)**. Nessuna certificazione di CI verde.
- Build/CTest Windows della presente PR: **ND** fino all'esito; screenshot multi-DPI, misure realtime e banco esterno MIDI/OSC/LTC: **ND**.
- Rollback: revert della presente PR (solo geometrie/hit target, test e focus VIEW); compatibilità dei dati invariata.

## [0.9.44] - 2026-11-11

**Stato:** candidato redesign nativo Timeline Markers e Tempo Map. Schema progetti JSON v10 invariato. Nessuna modifica deliberata a GO/STOP/PANIC, Control Map, routing o generazione LTC.

### Timeline, modello e compatibilità
- Eliminato il vecchio pulsante/editor Tempo Map dall'Inspector; BPM iniziale della cue mantenuto. Nuova corsia marker JUCE sopra le clip, con doppio clic per creare Generic Marker, rinomina inline, Snap e drag, menu contestuale per conversione Tempo Marker, cambio BPM, riconversione ed eliminazione.
- Marker separati per cue: ID locali, nome, posizione assoluta in secondi, tipologia, BPM e metadati di indicazione di tempo. La Tempo Map a beat resta la struttura consumata dagli editor MIDI/OSC e dal playback, ricostruita dai marker. I vecchi punti tempo vengono importati senza spostare gli eventi.
- Persistenza tramite campo opzionale `markers` nelle righe v10, con controlli di validazione, limiti e roundtrip; vecchi file senza campo mantenuti leggibili.

### Continuità LIVE (requisito operatore)
- **Nessun edit di marker/tempo interrompe LIVE**: niente stopAll/PANIC e nessun reset intenzionale del cursore Audio/MIDI/OSC. `publish(..., preservePlayback=true, retimeSchedule=true)` conserva la posizione; gli eventi MIDI/OSC futuri sono rischedulati e le note sostenute vengono riconciliate senza retrigger indiscriminato. Anche Undo/Redo di modifiche esclusivamente ai marker preserva il transport.
- I cambi sono immediati; eventi il cui tempo viene retrodatato prima del playhead non vengono riprodotti una seconda volta. La riprogrammazione MIDI/OSC non implica certificazione di latenza a valle di hardware/UDP.

### Verifiche e gate
- Aggiunti test nativi per conversione legacy, persistenza v10, indipendenza delle cue, Snap/rename inline e continuità del trasporto Audio/MIDI/OSC; adeguati test UI alle nuove coordinate delle track.
- Compilazione Windows Release e CTest: **ND** fino a esecuzione CI. Screenshot JUCE 100/125/150% DPI, stress realtime, sessione hardware MIDI/OSC/LTC e test operatori: **ND**.
- Rollback: revert della PR; nessuna migrazione v11, rimuovendo il campo opzionale `markers` i vecchi reader v10 continuano a usare `tempo`.

## [0.9.43] - 2026-11-10

**Stato:** candidato UX e utility Windows per rinomina singola cella e individuazione della sorgente Audio. Schema progetti JSON v10 invariato; nessuna modifica al trasporto audio/MIDI/OSC/LTC.

### Interfaccia JUCE
- **Ctrl+R** rinomina direttamente la cella selezionata. L'azione **Rename cell** è disponibile dal menu destro delle celle Audio, MIDI, OSC e Control (compresi Control Map non bloccati), riutilizzando l'editor inline nativo di cue/colonne. Enter salva; Esc annulla. Il nome salvato è `cell.name` e non rinomina file o Control Map.
- I nomi personalizzati sono visibili anche per le celle vuote e per le celle Control, mantenendo riconoscibile lo stato della mappa assegnata. L'editor segue gli ID stabili, rispetta il viewport e conserva NEXT/LIVE.
- **Find in Explorer** compare soltanto nel menu destro Audio. È disabilitato se manca un file assoluto esistente. Usa `juce::File::revealToUser()`, senza avviare il contenuto, toccare la sorgente o alterare il routing. Il menu ricontrolla la sorgente al click.
- Conservati le altre azioni contestuali, menu Control Mapping, Undo/Redo, GO/STOP/PANIC e selezione diretta Auto Stop.

### Verifiche
- Aggiunti test nativi per scorciatoia Ctrl+R, Enter/Esc, nomi Audio/MIDI/OSC/Control, identità e contenuto immutati, persistenza JSON v10, isolamento NEXT/LIVE, disponibilità contestuale di Find in Explorer e file mancanti/relativi.
- Windows Release/CTest candidato: **ND** prima dell'esito CI; apertura effettiva di Explorer, test DPI e collaudo con hardware esterno: **ND**.
- Rollback: revert della PR; nessuna migrazione dati richiesta.

## [0.9.42] - 2026-11-10

**Stato:** hotfix del test grafico JUCE per la selezione Auto Stop inline, con rendering AUTO/CONT. verificato su area effettiva della Cue Grid. Nessuna modifica al runtime Audio/MIDI/OSC/LTC, VIEW/LIVE o allo schema JSON v10.

### Corretto
- Il fixture `N3.4+ picks individual Audio MIDI OSC Control cells without popup or transport` aggiunge quattro colonne dopo la costruzione di `MainComponent`. Invocare `setSize(1440,900)` su un componente già di quelle dimensioni non garantisce un ricalcolo layout: la posizione del marcatore della colonna Control può cadere fuori dalla dimensione precedente della griglia, rendendo lo snapshot JUCE non valido.
- Ora il fixture chiama `refreshForTest()` per aggiornare geometria e viewport reali prima dei test. Verifica esplicitamente i limiti del componente e cattura badge di cella inclusa/esclusa, controllando un effettivo cambiamento dei pixel (AUTO → CONT.) dopo il clic.
- Il log Windows 0.9.41 fornito dall'operatore mostra `The Cuelist.exe` e `CuelistTests.exe` compilati, ma CTest **FAIL**: 420.051 asserzioni, 1 test fallito (`Direct picker cell marker is not rendered`). Nessun esito verde rivendicato per questa versione.

### QA e rollback
- Candidato 0.9.42: compilazione Windows Release e CTest **ND**, in attesa del workflow; screenshot con DPI reali, prova operatori e hardware **ND**.
- Rollback: revert della PR senza migrazione `.cuelist`; il flusso Auto Stop rimane intatto.

## [0.9.41] - 2026-11-10

**Stato:** hotfix test Windows della modalità di selezione celle Auto Stop introdotta nella 0.9.40. Non modifica i componenti JUCE operativi, il motore, VIEW/LIVE o i file `.cuelist` JSON v10.

### Corretto
- `tests/CueGridN3VisualTests.h`: il test di sicurezza PANIC costruisce esplicitamente `juce::KeyPress{juce::KeyPress::escapeKey}` prima di invocare `MainComponent::keyPressed`. La costante `escapeKey` è un intero e non viene convertita implicitamente dal costruttore `explicit` di JUCE.
- La build Release 0.9.40 fornita dall'operatore compilava `The Cuelist.exe` ma **falliva** su `CuelistTests.vcxproj` (MSVC C2664 alla riga 581), pertanto non era una compilazione completa riuscita.

### Verifiche e limiti
- Nuovo candidato 0.9.41: Windows CI Release/CTest da verificare prima del merge. Hardware Audio/MIDI/OSC/LTC, DPI e prove con operatori: **ND**.
- Rollback: revert della PR senza migrazione di progetto.

## [0.9.40] - 2026-11-10

**Stato:** intervento UX richiesto dall'operatore: Auto Stop inline per cella, senza PopupMenu. Candidato non certificato per uso live; verificare Windows CI/CTest e screenshot nativi.

### Interfaccia JUCE
- Lo switch ON/OFF della fascia Auto Stop viene spostato verso destra, con hitbox aggiornata e separata dal tasto +.
- **+** attiva/disattiva una modalità temporanea di selezione sulla cue (nessun menu a tendina). Le celle della cue si evidenziano immediatamente con marker `AUTO` per le incluse e `CONT.` per le escluse; è disponibile il contatore incluse/totali. Cliccare una cella alterna la partecipazione usando `setCellAutoStop`, poi si possono modificare altre celle senza riaprire un menu.
- Durante la modalità i clic sulle celle non avviano Play/Stop, aprono Control Map, effettuano drag o cambiano NEXT/LIVE. Le celle collassate sono selezionabili. Un clic esterno termina la selezione senza attivare la cella diversa; + termina esplicitamente.
- La modalità si disattiva cambiando VIEW/caricando un nuovo progetto o invocando GO/PANIC. Esc mantiene il comportamento PANIC. Restano accessibili il menu contestuale e l'Inspector.
- Nessun cambiamento a schema JSON v10, motore Audio/MIDI/OSC/LTC, trasporto o Control Mapping.

### Test e gate
- Test nativi aggiornati per hitbox separate, click dalla griglia, isolamento celle, persistenza JSON v10, riordino con ID stabili, celle Control, clip Audio e colonne collassate, uscita da modalità e PANIC.
- Compilazione Windows/CTest del candidato, screenshot 100/125/150% DPI e misure con hardware: **ND** in attesa di risultati verificabili. La sola CI non certifica il live.
- Rollback: revert della PR; i flag `cell.stopOnNext` già salvati restano compatibili.

## [0.9.39] - 2026-11-10

**Stato:** hotfix di compilazione e documentazione sintetica; nessun cambiamento ai motori o allo schema JSON v10.

### Corretto
- CueMetricsOverlay implementa juce::SettableTooltipClient, coerentemente con CueGrid e le intestazioni fisse. Risolve gli errori MSVC C3861 di setTooltip in src/CueViews.cpp e C2039 di getTooltip nei test introdotti nella 0.9.38.

### Documentazione
- Riscritto README.md come panoramica professionale breve: obiettivo, funzioni reali, compilazione, progetti, documentazione e licenze.
- Rimossi dettagli di versione obsoleti e l'affermazione errata che LTC non è implementato: rimane disponibile la **generazione di LTC audio**, non un lettore timecode generalizzato.

### Verifiche e limiti
- Input: build Windows dell'operatore su 0.9.38 **FALLITA** per C3861/C2039 (log 2026-11-10); questo commit corregge le dichiarazioni JUCE che causavano gli errori.
- Candidato 0.9.39: compilazione Windows/CTest da verificare nella CI; test hardware Audio/MIDI/OSC/LTC e DPI reali **ND**.
- Rollback: revert della PR; nessuna migrazione dei file .cuelist necessaria.

## [0.9.38] - 2026-11-10

**Stato:** ripristino UX del selettore celle Auto Stop nella fascia fissa; PR da verificare con CI Windows/CTest e collaudo nativo, non certificata per show live.

### Interfaccia nativa JUCE
- Reintrodotto il pulsante **+ sotto il toggle Auto Stop** di ciascuna cue (184 px di fascia, riga 62 px). BPM, toggle ON/OFF e + hanno hitbox separate, anche con scorrimento.
- Il + apre un menu nativo con tutte le celle/colonne Audio, MIDI, OSC e Control della cue e spunte che indicano quelle incluse nell'Auto Stop. La voce scelta usa lo stesso `setCellAutoStop` del menu destro e dell'Inspector; ogni cambio è reversibile con Undo.
- Il popup usa gli ID stabili di cue, colonna e cella e verifica la VIEW prima di applicare una scelta differita. Rimangono invariati il toggle di riga, i marker CONT., il menu destro, la semantica LIVE e la persistenza JSON v10.

### Test e limiti
- Aggiunti/aggiornati test JUCE per geometrie non sovrapposte, tooltip del +, spunte del menu, esclusione per tipo di colonna, isolamento fra righe, persistenza, scelta dopo riordino e spazio inattivo.
- Esecuzione locale Windows/CTest, screenshot DPI 100/125/150%, monitor ultrawide e prova Audio/MIDI/OSC/LTC con hardware esterno: **ND**. Il risultato CI della PR va verificato prima dell'integrazione.
- Rollback: ripristinare la PR oppure togliere + e relativa voce di accesso senza migrare dati; `stopOnNext` per cella resta immutato.

## [0.9.37] - 2026-11-09

**Stato:** RQ-001/T04, indagine reversibile sulla comprensibilità dei due comandi GO e sulla distinzione di Cuelist LIVE con nomi lunghi simili. Nessun merge automatico; non certificata per uno spettacolo live.

### Interfaccia JUCE
- Il pannello ON DECK / LIVE include ora un piccolo marcatore `L01`–`L12` della posizione corrente della Cuelist LIVE. Non è un ID persistente: segue l'ordine delle tab e affianca il nome LIVE e `GO NN` senza cambiare layout o hitbox.
- Il tooltip del vero GO di riga nel pannello CueGrid distingue il GO sulla LIVE corrente dal click da una VIEW diversa, che interrompe la precedente LIVE e trasferisce il controllo alla Cuelist visualizzata. Il pulsante laterale GO / SPACE continua ad agire sulla LIVE senza aprire la VIEW.
- Il sottotitolo della Cuelist usa correttamente `1 cue` e `1 clip` al singolare, mantenendo gli altri conteggi invariati.

### Test e documenti
- Test T04 di interazione JUCE sulla superficie GO della riga, separata dal GO globale; verifica conferma cue, cambio della LIVE e tooltip prima/dopo.
- Test T04 con due nomi diversi di 64 caratteri con prefisso identico: il nome completo rimane nel modello e nell'overflow, i due marcatori LIVE producono snapshot distinguibili.
- La suite genera tre screenshot nativi JUCE `interface-rq001-t04-*.png` a 1180/1920 per ispezione senza sostituirli a un test DPI Windows.
- Protocollo `docs/research/RQ-001-T04-GO-DPI.md` e CSV anonimo vuoto `RQ-001-T04-OBSERVATIONS.csv`: criteri, controesempi, 100/150% DPI reale, pilot con operatori, test hardware separati.
- Baseline 0.9.36: Windows CI `42691760123` PASS, **420.039 asserzioni / 0 fallimenti**. Candidato 0.9.37 sul commit `2ee48e4cb00ff0fbccc39aece346fae5caaa68f9`: Windows CI `42700175769` PASS, **420.071 asserzioni / 0 fallimenti**, CTest 5,06 s; artefatto screenshot caricato. Immagini esaminate: L01/L02 visibili, nomi lunghi ancora troncati in modo identico; 100/150% DPI reali e pilota operatori **ND**.
- Nessuna modifica a motore Audio/MIDI/OSC/LTC, GO/STOP/PANIC runtime, Control Map, Undo/Redo o schema .cuelist JSON v10.

## [0.9.36] - 2026-11-09

**Stato:** RQ-001/T03-B, variante sperimentale nativa JUCE per rendere più esplicito il rapporto VIEW/LIVE. Nessun merge automatico; la sola CI non certifica un utilizzo live.

### Sperimentazione UI
- ON DECK conserva la geometria 156×51 e presenta il nome effettivo della Cuelist LIVE (ellissi se necessario) accanto al successivo `GO 02` o `END`; il numero e il nome derivano dallo stato LIVE, non dalla VIEW.
- Il sottotitolo dell'header della lista indica `VIEW` mantenendo il titolo e le hitbox di rinomina e trasporto.
- La riga armata di una Cuelist visualizzata che non è LIVE viene denominata `VIEW NEXT`; la riga LIVE conserva `NEXT CUE`. Non alterate le precedenze PLAYING/PAUSED/MUTED.
- L'overflow delle tab nascoste indica esplicitamente `LIVE` per la Cuelist che controlla GO; il tooltip del pulsante GO rivela il target completo nel timer UI.
- Nessuna variazione al routing/audio callback, GO/STOP/PANIC, mapping, Undo/Redo o serializzazione JSON v10.

### Test e ricerca
- Regressione T03-B verifica nome LIVE, cue armata, tooltip dell'overflow, GO da VIEW, END, rinomina e riarmo; mantiene il test T03 originale.
- Due nuove immagini da componenti JUCE (`interface-rq001-t03b-*.png`) affiancabili agli screenshot della 0.9.35 ricevuti dall'utente; le due precedenti immagini T03 continuano a essere generate nello stesso CTest.
- Protocollo e controesempi: `docs/research/RQ-001-T03B-COMPARISON.md`; CSV vuoto dei dati comparativi `docs/research/RQ-001-T03B-OBSERVATIONS.csv`. Misure operatori, screenshot desktop a DPI 100/150 e hardware esterno: **ND**.
- Compilazione/CTest Windows della PR: **da verificare**. Il log 0.9.35 fornito dall'utente aveva 420.023 asserzioni e 0 fallimenti.

## [0.9.35] - 2026-11-09

**Stato:** correzione puntuale della compilazione dei test nativi RQ-001/T03 dopo l'integrazione della 0.9.34; runtime e interfaccia JUCE invariati.

### Corretto
- `tests/CuelistGuiMockupTests.h`: il controllo del tooltip LIVE usa il vero `juce::TextButton` tramite `dynamic_cast` verificato prima di chiamare `getTooltip()`. Il lookup generico `controlsV3::named()` restituisce `juce::Component*`, che non dispone di tale membro.

### Verifiche
- Windows locale 0.9.34, log dell'utente: MSBuild ha prodotto `The Cuelist.exe`, ma `CuelistTests.vcxproj` non compila a causa di C2039 (`getTooltip` non appartiene a `juce::Component`), alla riga 452 del test; CTest non eseguito. Lo stesso workflow Windows della PR #155 risulta fallito.
- La correzione 0.9.35 richiede nuova build/CTest Windows e verifica degli artefatti screenshot T03. **ND fino all'esito**, senza attestare risoluzione non misurata.
- Nessuna modifica al trasporto, a Audio/MIDI/OSC/LTC, a GO/PANIC, alla GUI nativa o allo schema JSON v10. Validazione operatore/DPI/hardware ancora aperta.

## [0.9.34] - 2026-11-09

**Stato:** strumentazione di ricerca RQ-001/T03, nessuna modifica al runtime né alla grafica. Non si tratta di una release certificata per impiego live.

### Aggiunto
- Test JUCE di regressione che copre GO dalla VIEW `Emergency` verso la Cuelist LIVE nascosta nell'overflow a 1180×820 e controprova con tab visibili a 1920×1080.
- Due snapshot native JUCE della stessa scena VIEW/LIVE a 1180×820 e 1920×1080, pubblicati come `interface-rq001-t03-*.png` fra gli artefatti del workflow Windows.
- Protocollo preregistrato `docs/research/RQ-001-T03-PILOT.md` e modello CSV anonimo `RQ-001-T03-OBSERVATIONS.csv` per la misura con operatori reali.

### Documentazione e verifiche
- `RQ-001-VALIDATION.md` aggiornata con il log Windows 0.9.33 fornito dall'utente: 419.999 asserzioni, zero fallimenti, durata CTest 5,44 s; T01 e T02 superati.
- CI Windows/CTest 0.9.34, review visiva dei PNG, DPI Windows reali 100%/150% e pilota operatori: **da verificare**; la sola rasterizzazione off-screen non convalida scaling DPI o usabilità.
- Banco Audio/MIDI/OSC/LTC con dispositivi esterni: **non eseguito**, gate separato.
- Restano intatti GO/STOP/PANIC, engine Audio/MIDI/OSC, editor, Undo/Redo, Control Map e lo schema .cuelist JSON v10.

## [0.9.33] - 2026-11-09

**Stato:** correzione mirata RQ-001 su VIEW/LIVE e isolamento del selettore file nei test. Non certificata per lo show live; schema .cuelist JSON v10 invariato.

### Corretto
- `triggerCell()` non chiama PANIC né trasferisce LIVE quando il comando dalla VIEW si limita a importare un file, aprire editor OSC/MIDI o modificare Control Map. Un Cell Play con clip reale continua invece ad assumere LIVE intenzionalmente e ora pubblica il nuovo RenderState prima dell'invio.
- `triggerCellAt()` preserva il cursore GO `liveNextCue` oltre a `nextCue`: l'azione Cell Play diretta non ri-arma accidentalmente un altro GO.
- Spostato il test T02 con `juce::FileChooser::launchAsync` all'ultima posizione della suite, per non interferire con i test successivi di input/focus su finestre native.

### Test
- Log Windows dell'utente sulla 0.9.32: compilazione completata, 419.987 asserzioni e 5 test falliti, di cui T02/T01 rossi attesi e tre fallimenti UI successivi da distinguere da possibili effetti del dialogo modale.
- Aggiunto test per il trasferimento LIVE esplicito con una cella Audio già caricata nella VIEW, inclusa adozione del relativo RenderState.
- Questa PR richiede un nuovo CTest Windows, verifica dei tre fallimenti UI, prova operatore, screenshot DPI e banco MIDI/OSC/LTC hardware: **ND** fino a risultati reali; non attestare gate superati.

### Rischi residui
- Misurazione timing end-to-end e hardware esterno non eseguiti.
- Il comportamento del dialogo asincrono e la semantica del GO vanno verificati anche tramite UI nativa Windows.

## [0.9.32] - 2026-11-09

**Stato:** RQ-001, branch di prova con asserzioni di regressione intenzionalmente strette. PR draft, non pronto al merge se CI rossa. Nessuna modifica al runtime o allo schema JSON v10.

### Aggiunto
- Due test nativi nel percorso Cuelist per riprodurre le ipotesi T02 (import Audio dalla VIEW non LIVE durante playback) e T01 (Cell Play non deve modificare il cursore GO armato).
- Protocollo `docs/research/RQ-001-VALIDATION.md` con scenario T03 per la comprensibilità della LIVE nascosta nell'overflow, metrica, rischi e gate hardware/UX.

### Verifiche
- Analisi statica di `src/MainComponent.cpp`, `src/CueOperations.cpp`, `src/CueViews.cpp`, `src/ControlRuntime.cpp` e dei test al commit base `21e75aab`.
- Windows build/CTest: non ancora eseguiti al momento dell'apertura della PR; registrare l'esito effettivo senza interpretare i test rossi come certificazione di un bug finché non sono riprodotti.
- Hardware, operatori, screenshot DPI: non eseguiti. Per T02 il selettore file asincrono potrebbe richiedere un controllo manuale su Windows.
- Nessuna modifica all'engine Audio/MIDI/OSC, al modello di GO/PANIC o alla serializzazione.

## [0.9.31] - 2026-11-09

**Stato:** bootstrap del laboratorio R&D, senza modifica del runtime audio/MIDI/OSC, del gameplay operatore o dello schema .cuelist v10.

### Aggiunto
- Laboratorio documentale `docs/research/`: protocollo, baseline verificata, brief Deep research, benchmark concorrenti, metodologia di validazione, registro RQ/ADR, istruzioni ChatGPT e schede fonti/esperimenti/operatore/hardware.
- Priorità di ricerca su workflow VIEW/LIVE, timing end-to-end, fault recovery, interoperabilità MIDI/OSC/LTC, UX Windows e gate commerciali.

### Modificato
- README collega il laboratorio e usa VERSION.txt come fonte della versione, eliminando l'etichetta iniziale 0.9.28 non aggiornata.
- VERSION.txt e versione CMake aggiornate coerentemente a 0.9.31.

### Verifiche
- Letti su main@ae671d77 README, checklist release, versioning, test, build/hardware check e Windows workflow; verificata la struttura del repository.
- Nessuna build, CTest, misura hardware, test operatore o esecuzione Deep research effettuata per questa PR.
- Da verificare: CI Windows e revisione del protocollo da parte del team; prima campagna RQ-001 da avviare separatamente.

## [0.9.30] - 2026-11-05

**Stato:** fix della suite Windows dopo la 0.9.29.

### Corretto
- Allineata la seconda riga dei controlli OSC alla baseline da 35 px dell'editor MIDI (Add, Delete, Duplicate, Grid, Snap, Resolution, Unit e zoom): nessuna modifica ai callback o alla gestione dei dati OSC.
- Aggiornati i test di geometria nativa che conservavano le vecchie coordinate di Timeline Collapse (+112 px, ora +12 px) e le vecchie dimensioni di Undo/Redo (28 px, ora 24 px). Le asserzioni confermano anche le separazioni reali dalle altre aree della barra.
- Rafforzati i controlli di parità della seconda riga OSC/MIDI, inclusi i pulsanti Add e Fit.

### Verifiche
- Il log Windows della 0.9.29 mostra compilazione di eseguibile e CuelistTests riuscita, 419498 asserzioni e tre test UI falliti per geometrie non allineate. Versioning e CI/CTest della 0.9.30 da verificare sulla nuova PR; nessun test hardware reale né build portable.

## [0.9.29] - 2026-11-05

**Stato:** affinamento UI/UX nativa; modifica non ancora certificata da build e dispositivi reali.

### Aggiunto
- Rotellina sul fader PCM di ciascuna colonna audio e sul Master, con passo di 0,5 dB, Shift per incrementi fini di 0,1 dB, supporto wheel/trackpad e rispetto del silenzio digitale. Il valore raggiunge gli stessi callback del fader trascinato.

### Modificato
- Grid Timeline trasformato in icona vettoriale con stato ON/OFF leggibile e snap indipendente; comando di collasso spostato prima del titolo.
- Pulsanti Undo/Redo globali compattati a 24 px; titoli delle Cuelist ingranditi nella tab e nella zona fissa delle cue; comando Collapse/Expand all convertito in icona con direzioni distinte.
- La maniglia di ridimensionamento del dock MIDI/OSC condivide con la Timeline dimensioni, posizione centrata e geometria grafica. La linea superiore del Master Audio segue l'accento delle strip.
- Barra OSC rivista con la stessa gerarchia a due righe del MIDI: titolo, storico e comandi di finestra nel primo livello, editing e zoom nel secondo. Collasso/Detach/Close hanno la stessa geometria e lo stesso stile vettoriale del MIDI.

### Corretto
- Eliminata la precedente sovrapposizione tra i controlli OSC e i pulsanti di collasso/chiusura del dock; stato Audio nell'header reso più compatto per ridurre il rischio di ritaglio.

### Test
- Aggiornate le verifiche di Timeline, Undo/Redo, layout dei due splitter e dock OSC/MIDI. Aggiunti test unitari per la conversione dei tick della rotellina, i limiti del fader e il silenzio digitale.
- Verifica Windows MSVC/CTest e controllo visivo DPI ancora da confermare; nessun test con dispositivi hardware reali. Nessuna build portable.

## [0.9.28] - 2026-11-05

### Interfaccia operatore

- Regolazione del gain tramite rotellina sul Master Audio e sui fader delle colonne Audio. La linea superiore del Master è allineata a quella delle strip.
- Nome della Cuelist più leggibile, comandi Undo/Redo compatti e comando grafico per espandere o comprimere tutte le colonne.
- La Timeline usa un simbolo per la griglia. Timeline e dock condividono la posizione dei comandi di collasso e l'aspetto della maniglia di ridimensionamento. I controlli dell'editor OSC seguono quelli del Piano Roll MIDI.

### Verifiche

- La CI Windows compila la versione Release, esegue i test JUCE e conserva le schermate native del mixer e degli editor. Il controllo del prototipo HTML viene eseguito soltanto quando il relativo file è presente.

## [0.9.27] - 2026-11-04

**Tipo:** sistema temi selezionabili e revisione cromatica Industrial Orange.

### Aggiunto
- `palette::ThemeTokens`: due preset immutabili (Legacy e Industrial Orange) assegnati alla stessa tabella di token JUCE già utilizzata da tutti i componenti; nessun secondo sistema di styling.
- Preferenza applicativa persistente `appearanceTheme`, selezionabile in View → Appearance e Settings → Appearance, con spunta sul tema attivo. Il cambio aggiorna immediatamente i colori di LookAndFeel, popup, controlli con override, tab e altri componenti visibili; non modifica il contenuto di Show o Undo.
- Legacy usa i valori esatti del commit `e6f1c384` della 0.9.15, ultima palette pre-Industrial Orange: superfici, rame, testo, PANIC, GO, indicatori MIDI/OSC e tab selezionata.

### Modificato
- Industrial Orange applica i valori esatti della specifica: `bgMain #101619`, `bgPanel #182227`, `bgElevated #202C32`, `bgHover #29373D`, `bgSelected #303B40`, `borderDefault #35434A`; `accent #C66B42 / #D47A4F / #A95534`.
- Tipi colonna: Audio `#8FA8B2`, MIDI `#B79A72`, OSC `#A18BB9`, Control `#7186A6`. GO/PLAY `#55B875`, PANIC `#E05A46`, WARNING `#E5A52E`, INFO `#55AFC5`, testo `#E5E7E3 / #A5B0AE / #718087 / #536168`. Il nuovo schema sostituisce i colori funzionali desaturati della 0.9.23.
- Riempimento visibile delle clip Timeline in tinta tipologica con outline indipendente più luminoso, selezione in arancio, contrasto interno adattivo e feedback hover solo visivo.
- Le cue colorate mostrano in Industrial Orange una superficie derivata da ciascuno dei sette colori personalizzati, con variante selected/hover; cue colorate selezionate marcate con un doppio rail chiaro/arancio per non confonderle con cue non colorate selezionate. I label scelgono automaticamente un inchiostro chiaro o scuro per massimizzare il contrasto.
- Nessun cambiamento ai sette valori originali `cue1..cue7`, a `Row.colour`, ai file multimediali, all'audio/MIDI/OSC, alle azioni o al JSON v10. La Timeline usa i colori di **tipo** già definiti nel modello; non introduce nuovi campi colore per i clip.

### Test
- Asserzioni sui token Industrial Orange e sulle identità Legacy originali; controlli di contrasto e indipendenza fra selezione, stato, tipo e colore salvato.
- Test cambio tema in tempo reale, salvataggio e riapertura preferenza, risoluzione popup LookAndFeel e integrità di `encodeShow()`.
- Windows MSVC/CTest e verifica visiva DPI da confermare in CI; nessuna build portable.

## [0.9.26] - 2026-11-04

**Tipo:** correzione compilazione e allineamento test di regressione Windows.

### Corretto
- In `tests/WorkspaceLayoutTests.h`, il pulsante Timeline Collapse viene risolto come `juce::TextButton*`, evitando `error C2039: getTooltip non è un membro di juce::Component`. Mantiene la verifica di Collapse/Expand ai quattro formati di finestra.
- In `tests/CuelistGuiMockupTests.h`, le prove LIVE richiamano `engine.collectRetired()` tra i blocchi audio quando si pubblicano modifiche ripetute. La simulazione segue così il recupero degli snapshot che `MainComponent::timerCallback()` esegue nel programma reale; non modifica le asserzioni su posizione, stato o GO.
- Nessun cambiamento all'applicazione, al motore di playback, alle interfacce o al formato `.cuelist` v10.

### Verifica
- Log Windows riportato: compilazione di `The Cuelist.exe` riuscita, fallita `CuelistTests` sui tre accessi `getTooltip()` a tipo non adatto.
- Nuova compilazione MSVC/CTest della PR richiesta prima dell'integrazione; nessuna build portable.

## [0.9.25] - 2026-11-04

**Tipo:** UI nativa, collasso pannelli/editor MIDI.

### Modificato
- Aggiunti pulsanti vettoriali `EditorDetachIconButton` e `EditorCloseIconButton`, con lo stile e la stessa hitbox da **28×26 px** dell'attuale `DockCollapseButton`. Nessuna dipendenza da font o glifi Unicode.
- Nell'header del Piano Roll, `MidiEditorToolbarLayout` dispone Collapse/Detach/Close nello stesso gruppo sul lato destro, con 4 px di spazio tra i pulsanti, per le larghezze compact e desktop. Il MainComponent e il Piano Roll usano gli stessi rettangoli senza sovrapposizioni. Detach/Reattach e Close mantengono i callback preesistenti.
- Aggiunto `timelineCollapseButton` con freccia su/giù nell'header della Timeline. È un figlio di `MainComponent`, per cui rimane accessibile come grip di riapertura anche quando la Timeline è nascosta.
- Il menu View e il nuovo pulsante condividono `setTimelineVisible`; la preferenza `viewTimelineVisible` viene aggiornata in entrambi i percorsi e le geometrie del workspace restano gestite da `calculateWorkspaceLayout`.
- Nessuna modifica a forme della Cue Grid, timing Audio/MIDI/OSC, VIEW/LIVE, serializzazione v10, palette o funzioni dell'editor.

### Test
- Test geometria uniforme e screenshot iconici MIDI in 710–1180 px, controlli del dock in 1180–1640 px.
- Test Timeline Collapse/Expand a 1180, 1440, 1647 e 1920 px con riapertura e consistenza View, senza sovrapposizione a footer o strumenti della Timeline.
- CI Windows/CTest e test visivo DPI ancora da verificare; nessuna build portable.

## [0.9.24] - 2026-11-04

**Tipo:** correzione stabilità trasporto LIVE e routing MIDI.

### Corretto
- `addCue`, `showColumnMenu`, `moveCueRow`, `reorderRow`, `reorderColumn`, duplicazione e inserimento cue non eseguono più `changed(..., true)`: il motore adotta snapshot con `preservePlayback`, mantenendo le voci esistenti grazie agli ID di cella stabili.
- Riordini e inserimenti mantengono l'ID della cue **armata per il prossimo GO**, anziché fare coincidere impropriamente il cursore di playback con il nuovo indice visuale.
- `setMidiOutput` non invia più PANIC e non forza il reconnect di dispositivi già aperti. Anche una destinazione MIDI indisponibile rimane una selezione esplicita: le sole uscite MIDI sono sospese, mentre cue e audio continuano.
- `RenderState::inheritPlayback` trasferisce clock, cursore e stato delle voci MIDI tra route differenti, eseguendo il resync delle note sulla nuova destinazione senza ripartire da zero.
- `MidiOutputRouter::stream` ritira la vecchia stream, ne incrementa l'epoch per scartare i pacchetti programmati, e segnala al worker il rilascio delle note ancora in carico. Il callback non invia ulteriori pacchetti a stream ritirate.
- Al ritorno di un dispositivo precedentemente indisponibile, il timer tenta il recupero esclusivamente per ID configurato e ripubblica lo snapshot conservando il playback, senza sostituzioni automatiche.

### Limiti e sicurezza
- L'eliminazione della cue o colonna LIVE, di una Cuelist LIVE intera e l'editing della mappa tempi non sono modificati. Attualmente rimangono operazioni distruttive con confine di sicurezza: servono regole esplicite per le voci già attive o eliminate. Nessuna politica arbitraria introdotta.
- Il formato `.cuelist` v10, Audio/OSC, palette e layout grafico non cambiano.

### Test
- Nuovi test di audio in esecuzione mentre vengono aggiunte/spostate cue e colonne, incluso controllo della posizione e dell'ID del prossimo GO.
- Test di cambio MIDI Output verso un device inesistente e ritorno a output vuoto durante una cue audio; test unitari su eredità del cursore MIDI e invalidazione dei pacchetti del vecchio stream.
- Windows CTest, MIDI fisico hotplug e invio sul nuovo port da verificare prima del merge; nessun pacchetto portable.

## [0.9.23] - 2026-11-04

**Tipo:** miglioramento esclusivamente cromatico di Industrial Orange.

### Modificato
- Verde degli stati GO/PLAYING/AVAILABLE `#8EAE91` → `#9BB892`: salvia leggermente più chiara, senza verde lime.
- Rosso di PANIC/errori `#C66E69` → `#C97870`: terracotta più distinguibile sul grafite; i piccoli indicatori rossi continuano a usare il derivato più chiaro già previsto per la leggibilità del testo.
- Blu Audio/Info `#8FA8B2` → `#86A6BD`: acciaio leggermente più distinto dal fondo; Control `#7186A6`, MIDI `#B79A72` e OSC `#A18BB9` conservano la loro identità.
- L'arancio di selezione `#C66B42`, i livelli grafite, il Warning ambra, il testo e i sette colori assegnabili alle cue restano esattamente invariati.
- Nessuna modifica a layout, hitbox, rendering, MIDI, OSC, trasporto LIVE/VIEW, schema dei progetti `.cuelist` v10 o motore audio.

### Test
- Aggiornati gli HEX attesi e aggiunte asserzioni di contrasto per il verde in riproduzione e il blu informativo sul grafite, preservando le verifiche di testo e PANIC.
- Verifica visiva Windows e test hardware ancora necessari; nessun pacchetto portable generato.

## [0.9.22] - 2026-11-04

**Tipo:** correzione test di regressione del layout Undo/Redo, senza modifica del software operativo.

### Corretto
- Il test `Native Undo/Redo remain accessible in header or Edit/More without overlap` rispetta il requisito introdotto nella 0.9.21: le icone Undo/Redo si vedono dopo Guide quando c'è spazio, altrimenti i comandi restano nei menu Edit/More e tramite scorciatoie.
- Verifica responsive a 1180, 1440, 1647 e 1920 px con assenza di sovrapposizioni a Guide, telemetria e Cuelist tabs.
- Lo snapshot della telemetria audio usa le coordinate responsive correnti, non il vecchio rettangolo fisso `width-365`.
- Identificata la causa dell'unico test fallito nel collaudo Windows 0.9.21: il test precedente pretendeva che i pulsanti fossero sempre nascosti, in contraddizione con la specifica UI vigente. Il codice applicativo non cambia.

### Verifiche
- Compilazione applicazione 0.9.21 riuscita sul PC Windows dell'operatore e 419.906 asserzioni eseguite con un unico test di layout obsoleto fallito.
- Da verificare: nuova build Windows e CTest su GitHub Actions per 0.9.22.
- Nessuna build portable, nessuna modifica a motori, routing, progetto `.cuelist` v10, rendering o palette.

## [0.9.21] - 2026-11-04

**Tipo:** completamento mirato primo ciclo di rifinitura OSC e navigazione globale.

### Modificato
- Nell'OSC Editor, il trascinamento col pulsante destro seleziona eventi e segmenti tramite la cornice già esistente; un singolo clic destro continua ad aprire il menu contestuale. La soglia di trascinamento impedisce aperture involontarie.
- Il doppio clic nell'area interna di una Ramp OSC inserisce un nuovo punto tipizzato, quantizzato quando Snap è attivo e validato prima della modifica. I punti già esistenti restano selezionabili e trascinabili; Shift+clic rimane disponibile per compatibilità.
- Eliminato il dialogo ridondante `OSC ramp values`: gli estremi si regolano direttamente tramite i punti della curva, mentre address, template argomenti numerici e range rimangono nelle impostazioni di traccia. `argumentIndex` viene mantenuto: identifica l'argomento giusto nei messaggi multiargomento e preserva i file esistenti.
- Due icone vettoriali Undo e Redo sono ora visibili subito a destra del menu Guide negli header che le possono contenere senza sovrapposizioni; su finestre strette restano accessibili tramite Edit/More e scorciatoie. Usano lo storico JUCE globale già implementato, senza duplicarlo.
- Nessuna modifica a schema `.cuelist` v10, audio, MIDI/OSC network, VIEW/LIVE, colori utente, palette Industrial Orange o layout della Cue Grid.

### Test
- Aggiunti test per selezione OSC multi-elemento con tasto destro, inserimento di un punto con doppio clic, protezione dalla duplicazione, e hitbox delle icone globali rispetto al menu Guide e alla telemetria.
- Compilazione Windows e CTest richiesti prima dell'integrazione; nessuna build portable.

## [0.9.20] - 2026-11-04

**Tipo:** Industrial Orange — revisione semantica esclusivamente cromatica.

### Modificato
- Rafforzata la separazione tra il colore identitario `#C66B42` e i colori di funzione: selezione e playhead arancio, GO/PLAYING in `#8EAE91`, NEXT/PAUSED e avvisi in `#D1A45B`, informazioni tecniche in `#8FA8B2`.
- Cue Grid: rail della selezione e bordi delle celle selezionate in arancio; indicatori LIVE e pulsanti GO/Play in verde. Le fasce di tipo contenuto Audio/MIDI/OSC/Control restano indipendenti, le superfici neutre.
- Timeline: indicatore temporale e cornice del clip selezionato in arancio; la striscia tipologica di ciascun clip mantiene il suo colore originale.
- OSC Editor: curve/indicatori di tipo restano viola, mentre track, evento, punto e bordo del segmento selezionati hanno l'accento arancio; indicatore temporale arancio e warning ambra.
- Mixer e diagnostica: meter audio con progressione verde/ambra/rosso, presenza audio in verde, attività MIDI/OSC nei rispettivi colori, CPU/RAM in tinta tecnica neutra.
- Nessun cambiamento a forme, dimensioni, spazi, layout, gesture, motori, audio, MIDI/OSC, schema JSON v10 o colori personalizzati delle cue.

### Test
- Nuovo controllo automatico sull'indipendenza fra colori di selezione, protocollo, riproduzione e informazioni tecniche, nonché sul contrasto dei bordi di selezione e dello stato LIVE.
- Build Windows/CTest e controllo visivo dei rendering DPI rimangono da verificare; nessuna build portable.

## [0.9.19] - 2026-11-04

**Tipo:** Undo/Redo unificato per l'editing nativo dello Show.

### Modificato
- Introdotta `ShowHistory.cpp`: snapshot transazionali di modello e cache media (solo puntatori condivisi, non campioni PCM) nello stesso `juce::UndoManager` già usato da MIDI e OSC; limiti di memoria configurati.
- Le modifiche principali a cue, righe, celle, colonne, colori, Auto Stop, durata/offset, Master e impostazioni mixer vengono registrate come operazioni globali Undo/Redo in ordine cronologico.
- L'Undo/Redo di editor MIDI e OSC mantiene una transazione per gesto, sincronizza lo storico e localizza le celle in qualsiasi Cuelist, inclusa una tab non visualizzata.
- Il ripristino convalida lo Show, ritrova l'identità LIVE tramite ID stabili, confronta le sequenze e le sorgenti PCM e non interrompe il playback quando la modifica è circoscritta alla VIEW. Se il ripristino tocca realmente la LIVE, arresta cautelativamente il trasporto prima di pubblicare la nuova versione.
- Le semplici operazioni di navigazione VIEW, GO, Stop e PANIC non vengono archiviate. Nuovi Show e apertura file azzerano la baseline della cronologia.
- Gli aggiornamenti Master e mixer pubblicano ora `makeLiveRenderState()`, evitando che l'editing di una tab VIEW sostituisca accidentalmente la sequenza operativa.

### Test
- Test Undo/Redo su colori, Auto Stop, duplicazione/ripristino di righe e continuità della riproduzione audio LIVE durante un Undo sulla VIEW.
- CMake include la nuova unità `ShowHistory.cpp` sia nel programma che in CTest. File di progetto `.cuelist` (schema v10), renderer audio e formati media non modificati.
- Test CI Windows e collaudo di MIDI/OSC fisici, performance e memoria con Show grandi necessari prima di una distribuzione live. Nessuna build portable.

## [0.9.18] - 2026-11-04

**Tipo:** OSC Editor — configurazione tracce e selezione multipla.

### Modificato
- Il doppio clic sulla testata di una traccia OSC apre un unico dialogo di configurazione condivisa: indirizzo, argomenti tipizzati (anche più argomenti), argomento numerico automatizzato, intervallo minimo/massimo.
- Il dialogo di un singolo segmento Ramp contiene soltanto i valori iniziale e finale; mantiene gli eventuali punti intermedi e non modifica implicitamente la configurazione degli altri segmenti.
- L'operazione `updateOscTrackSettings` copia il clip, applica il nuovo template a tutti i segmenti della traccia e convalida tutto prima di sostituire il modello; rifiuta type/range incompatibili senza alterazioni parziali.
- Conservata la selezione multipla OSC già presente: Ctrl/click, rettangolo, spostamento collettivo, eliminazione e duplicazione. Un drag completo genera una sola transazione Undo.
- Nessuna modifica alla pipeline OSC di rete, ai payload e tempi delle automazioni, al formato salvato (JSON v10) o alla palette Industrial Orange.

### Test
- Test automatico con due Ramp, argomenti OSC multipli e mutazione transazionale della testata; rollback su valori incompatibili e range insufficienti.
- Test simulato della selezione Ctrl e dello spostamento di due eventi con un'unica transazione Undo.
- Build/CTest Windows e verifica con ricevitori OSC reali da effettuare prima dell'uso in produzione; nessuna build portable.

## [0.9.17] - 2026-11-04

**Tipo:** gestione indipendente delle Cuelist VIEW e LIVE.

### Modificato
- La tab visualizzata (`show.activeCuelist`) è indipendente dalla Cuelist con trasporto live (`liveCuelist`); il cambio di VIEW non invia PANIC, non ripubblica un'altra sequenza e non annulla il mapping live.
- Il GO globale continua ad agire sul cursore LIVE anche navigando un'altra Cuelist; il GO esplicito di una cue nella VIEW trasferisce intenzionalmente la riproduzione, fermando il precedente trasporto.
- L'engine e il controller mapping usano uno snapshot della Cuelist LIVE per le pubblicazioni e per la risoluzione dei preset, mantenendo le identità stabili dei media.
- Tab con un indicatore verde LIVE e tooltip VIEW/LIVE; `ON DECK` mostra il cursore live. Reorder e cancellazione di una Cuelist non-live conservano trasporto e indici; l'eliminazione della LIVE esegue PANIC.
- Apertura e creazione di Show ripristinano il contesto LIVE; tornando alla tab LIVE non si azzera il cue già armato.

### Test
- Aggiunta prova di regressione: playback Audio mentre si visualizza una seconda Cuelist e GO che continua ad avanzare la LIVE.
- Il formato `.cuelist` resta JSON v10 e conserva `activeCuelist` come selezione VIEW; l'identità LIVE corrente è runtime-only.
- Build/CTest Windows, verifica di audio/MIDI/OSC fisici, changeover e mapping da collaudare prima dell'impiego critico. Nessuna build portable.

## [0.9.16] - 2026-11-04

**Tipo:** redesign cromatico Industrial Orange, esclusivamente visivo.

### Modificato
- Allineati ai valori della specifica i token in `src/Theme.h`: `bg-main #101619`, `bg-panel #182227`, `bg-elevated #202C32`, `bg-hover #29373D`, `bg-selected #303B40`, `border-default #35434A`; arancio `#C66B42 / #D47A4F / #A95534`.
- Tipi colonna indipendenti dall'accento: Audio `#8FA8B2`, MIDI `#B79A72`, OSC `#A18BB9`, Control `#7186A6`.
- Stati indipendenti: GO `#8EAE91`, PANIC `#C66E69`, Warning `#D1A45B`; gerarchia testo `#E5E7E3 / #A5B0AE / #718087 / #536168`.
- Conservati esattamente i sette colori custom delle cue assegnati prima di questo redesign, anche quando le stesse cue vengono caricate da file precedenti.
- Gli alias storici del tema rimangono per compatibilità, ma i token semantici sono la fonte delle scelte cromatiche. Arancio leggermente schiarito solo per il testo piccolo, PANIC con testo scuro leggibile.
### Test
- Aggiornati i test dei token, delle cue, del mixer e del contrasto. **Nessuna modifica a layout, file .cuelist, engine, trasporto o input**, né a dimensioni e hitbox.
- Build e CTest Windows da verificare in CI, controllo visivo reale su DPI/32:9 ancora necessario; nessun pacchetto portable.

## [0.9.15] - 2026-11-04

**Tipo:** aggiornamento esclusivamente visivo — tema Graphite / Copper.

### Modificato
- Sostituita la palette Forest/Teal in `src/Theme.h` con sei livelli di grafite neutra: background `#121416`, input `#1B1E20`, panel `#24272A`, toolbar `#2B2F32`, raised `#34393C`, divider `#474B4F`.
- L'accento interattivo principale ora è rame chiaro `#E4A77E` (focus, selezione, sottolineature, icone, menu e colonne Audio); tab attiva `#5C3B2D` e testo `#F2EDE5` / `#C2B8AA`. Il token sorgente `palette::teal` rimane un alias del rame per compatibilità con i componenti esistenti.
- Colori operativi conservati come categorie distinte e meno saturate: GO/disponibilità verde `#8CBD9F`, MIDI/standby ambra `#DEBD88`, OSC lilla `#B7A1BF`, errore `#E48C7F`, PANIC `#AB5146`.
- Aggiornati i colori opzionali delle cue 5–7 a blu ardesia, salvia desaturata e ottone; gli indici colore dei progetti salvati sono invariati.
- Allineati i testi descrittivi nel README e nella guida alla palette corrente.

### Test
- Aggiornati i test delle sottolineature, delle strip, degli indicatori di clip e delle cue che in precedenza si aspettavano l'acquamarina.
- Controllato da test automatico il contrasto del testo caldo, del rame e dei colori MIDI/OSC su superfici grafite: soglie WCAG 4.5:1 per testo piccolo, almeno 7:1 per testo primario sul pannello.
- Nessuna modifica a modello, playback, MIDI/OSC, Undo/Redo, file `.cuelist`, geometria, logo embedded o logica dei comandi. Build/CTest Windows e revisione visiva su schermi DPI da verificare in CI e sul dispositivo dell'operatore.

## [0.9.14] - 2026-11-04

**Tipo:** primo blocco indipendente del ciclo di rifinitura (routing MIDI e Quality of Life UI).

### Modificato
- Il GO di una cue o di una cella non viene più bloccato dall'assenza, disabilitazione o disconnessione dell'uscita MIDI; audio, OSC e cronologia di trasporto procedono normalmente. Il router esistente mantiene la sua policy di scarto dei pacchetti senza route e il ripristino sullo stesso identificatore del dispositivo.
- Nella strip Utility/Mixer delle colonne MIDI il selettore mostra sotto di sé l'avviso rosso `No MIDI Output` quando la destinazione non è valida; l'inventario aggiorna lo stato a cadenza di un secondo.
- Le Cuelist trascinate mostrano un sottile indicatore di inserimento tra le tab anziché una tinta di sfondo; identità e algoritmo di riordino invariati.
- Nel Piano Roll MIDI il bordo destro delle note espone un cursore orizzontale di ridimensionamento, con grip adattivo e corpo ancora trascinabile.
- Gli editor MIDI e OSC condividono pulsanti vettoriali Undo/Redo: OSC ora ha anche i due pulsanti visibili, collegati alle callback Undo/Redo già presenti; nessuna seconda cronologia.
- Il dock MIDI/OSC dispone di un pulsante di collasso compatto a freccia, coerente con il grip dell'Inspector, e di una sottile linea di separazione nella parte superiore.
- Nel menu View aggiunti `Show Clock`, `Show CPU Meter` e `Show RAM Meter` come voci indipendenti; `Collapse/Expand Timeline` restituisce lo spazio alla cue grid senza modificare il dock o il collasso Inspector. Preferenze solo locali, non salvate nello Show. `Fader Range` invariato.

### Test e limiti
- Aggiunto test sul rendering audio mentre è presente una colonna MIDI senza output; i test precedenti del router verificano gli identificatori esatti senza fallback.
- Aggiunti test della geometria Timeline collassata/ripristinata e aggiornati i contratti di hitbox dei dock e della toolbar OSC.
- Build Windows, CTest e test manuali su dispositivi fisici rimangono da verificare in CI e sul PC dell'operatore; nessuna build portable.
- Il documento di specifiche ammette interventi incrementali. Restano per tranche successive, dopo audit di stato/modello: Cuelist selezionata vs Cuelist in playback, riorganizzazione dati/finestre OSC Ramp e interazione multi-select OSC, Undo/Redo globale. Non sono stati implementati né dichiarati completati in questa tranche.

## [0.9.13] - 2026-11-04

**Tipo:** correzione del blocco CMake introdotto con la 0.9.12.

### Corretto
- La regex in `CMakeLists.txt` verificava ancora `0.9.11` anche dopo che `project(... VERSION ...)` e `VERSION.txt` erano stati portati a `0.9.12`: la configurazione Windows falliva prima della compilazione.
- Il controllo ora genera la propria regex direttamente da `PROJECT_VERSION`, eliminando il numero duplicato e ammettendo gli eventuali suffissi prerelease; in caso di incoerenza mostra entrambi i valori.
- Aggiunto un controllo nel gate Python che intercetta un diverso valore fra `VERSION.txt` e il `project VERSION` di CMake. Nessun cambio al funzionamento di audio, MIDI, OSC o UI.

### Verifiche e limitazioni
- Verificato il comportamento del controllo di versione in CMake minimo: accettazione di release e prerelease coerenti, rifiuto di versioni differenti o malformate.
- Build MSVC, test JUCE e collaudo operativo ancora da confermare tramite Windows CI e PC locale; nessuna build portable.

## [0.9.12] - 2026-11-04

**Tipo:** redesign cromatico nativo Forest/Teal, senza modifiche ai comportamenti operativi.

### Modificato
- Adottati i dodici colori della palette di riferimento approvata, campionati esattamente nell'ordine originale: `#070802`, `#112710`, `#182F25`, `#243F2E`, `#32642F`, `#2A6B71`, `#57A29D`, `#6AE7E3`, `#31B964`, `#EEF5C1`, `#B84729`, `#F8F8F8`.
- Aggiornati i token globali in `src/Theme.h` usati da Cue Grid, Inspector, mixer, header, Timeline, editor e popup: sfondi verde molto scuro, testi quasi bianchi e accenti acquamarina/avorio/verde/terracotta.
- Riassegnate anche le tre tinte personalizzate non centralizzate della Cue Grid; mantenute tutte le sette opzioni di colore e le rispettive interazioni.
- Colori di avvertimento e testo secondario derivati esclusivamente dai campioni approvati con miscelazione controllata per conservare il contrasto su toolbar e superfici rialzate.
- Comandi GO e Panic distinguibili tramite verde brillante e terracotta. La variabile storica `palette::purple` resta per compatibilità con i componenti OSC ma ora punta al verde brillante; non si introduce alcun viola fuori palette.

### Verifiche e limitazioni
- Aggiunti test JUCE dei dodici valori ARGB e del contrasto minimo 4,5:1 per testo primario, secondario e avvisi sulle superfici di base.
- Non eseguiti in questa sessione build MSVC, CTest o collaudo grafico su Windows. Verificare in CI e poi su monitor reali, inclusi 125%, 150%, 32:9, riga selezionata, stati di allarme, celle audio/MIDI/OSC/Control, Control Mappings, editor e PopupMenu.
- Nessuna modifica al modello show, al formato JSON v10, alla temporizzazione o ai motori Audio/MIDI/OSC; nessun pacchetto portable prodotto.

## [0.9.11] - 2026-11-02

**Tipo:** rifinitura del layout I/O Activity.

### MIDI/OSC Learn
- I pulsanti native `M Learn` e `O Learn` sono ora impilati in verticale a destra nel pannello `I/O Activity`; il primo è sopra il secondo con identica posizione X e stessa larghezza.
- MIDI RX/TX e OSC RX/TX conservano le quattro lampade e le etichette su due righe, con colonne più compatte che non invadono i pulsanti Learn a 1024, 1250, 1550 e risoluzioni ultrawide.
- Identità, callback, tooltip, stato armato e logica Quick Learn invariati; si modifica solo la geometria nativa.
- Aggiunti controlli JUCE della geometria verticale e dell'assenza di sovrapposizioni fra pulsanti e lampade. Nessuna build portable.

## [0.9.10] - 2026-11-02

**Tipo:** aggiornamento nativo dei controlli I/O, dell'apertura rapida dei progetti e della strip BPM/Auto Stop.

### I/O Activity
- Spostati i pulsanti originali `M Learn` e `O Learn` nella prima fascia di `I/O Activity`; le lampade MIDI RX/TX e OSC RX/TX restano nel gruppo e le callback/stati Learn sono invariati.
- Eliminata la vecchia cornice Learn sotto il wordmark; layout responsivo verificato a 1024px e su schermi ultrawide.

### Recent Projects
- Sotto `Workspace` pannello nativo con progetti aperti/salvati di recente, pulsante `Open...`, nomi progetto, cartella/data e menu `All` per la cronologia completa.
- Riutilizzato il `PropertiesFile` per la cronologia (massimo 8 progetti) e il caricatore asincrono esistente. L'apertura rapida mantiene la conferma per lavori non salvati; gli elementi vengono aggiunti dopo apertura o salvataggio riusciti.
- Nessun nuovo schema dei file .cuelist né effetti sul motore Audio/MIDI/OSC.

### BPM / Auto Stop
- Allineamento e bilanciamento delle intestazioni e del valore BPM, accento di stato sottile condiviso tra le due celle e divisoria centrale continua anche nelle aree senza cue e sotto la striscia del mixer.
- Hitbox originali e logica BPM/Auto Stop invariate.

### Test e limitazioni
- Test JUCE dei pulsanti Learn nel nuovo pannello, presenza dei controlli recenti e rendering della separazione BPM/Auto Stop.
- Test Windows CMake/CTest, versioning e changelog richiesti in CI prima del merge. Nessun pacchetto portable e nessun collaudo hardware reale.

## [0.9.9] - 2026-11-02

**Tipo:** unificazione effettiva della presentazione del Control Mappings editor, compresi gli override della Control Column.

### Control Mappings / Control Column
- Eliminati il disegno, il layout e l'editor form legacy attivati da `editSet`: tutte le mappe usano la stessa disposizione nativa di Control Mappings (lista a tre colonne e form di modifica).
- Map salvata e override locale mantengono la selezione corretta per ID e il rispettivo percorso di modifica nel modello esistente; campi di destinazione e assegnazioni di funzione sono read-only per gli override locali.
- Conservate anteprima del mapping effettivo, salvataggio locale come Map riutilizzabile e applicazione di una Map salvata, accessibili dal medesimo menu `...` del nuovo editor.
- Il titolo del dialogo e la geometria dell'editor sono identici per Default, Saved e Cue-local scope; il clic destro delle Control Column continua a gestire le assegnazioni.

### Test e compatibilità
- Test JUCE dell'identità della UI tra Map salvate e locali, del form moderno per acquisizioni locali e della mancata riassegnazione implicita.
- Nessuna modifica a MIDI/OSC routing, prioritizzazione, Show JSON v10, audio engine o generatore LTC. Nessuna build portable.
- PR #127; verifiche Windows CMake/CTest e versione obbligatorie prima del merge.

## [0.9.8] - 2026-11-02

**Tipo:** collaudo LTC end-to-end senza hardware fittizio; documentazione operativa dei limiti.

### Test e affidabilità
- Nuovo test-only receiver indipendente dal generatore: recupera timecode BCD, flag drop-frame, sync SMPTE e parità direttamente dal PCM WAV per 24, 25, 29.97 drop/non-drop e 30 fps, includendo il passaggio di minuto.
- Nuovo test della vera pipeline Audio: WAV LTC caricata come clip, trasporto avviato e segnale verificato sul solo Direct Out dedicato, con Master silenziato e device a 48/44.1 kHz. Confronto sample-per-sample sul percorso 48 kHz a guadagno unitario.
- Nessun inserimento di un timecode engine globale né di un ricevitore LTC runtime.

### Guida e limiti
- Descritta in guida la generazione LTC, la distinzione rispetto a LTC/MTC chase, e la necessità di evitare le uscite verso diffusori/cuffie.
- Aggiunto `docs/ltc-hardware-acceptance.md` per la verifica con decoder fisico indipendente. La CI non certifica hardware, patch fisici o sincronizzazione esterna.
- Nessuna modifica allo schema dei progetti v10, UI, routing o DSP esistenti. Nessuna build portable.
- PR #126; risultati Windows CI/CTest richiesti prima dell'integrazione.

## [0.9.7] - 2026-11-02

**Tipo:** rafforzamento dei test nativi delle Control Map dopo il consolidamento 0.9.6.

### Test e verifica
- Il test dell'editor condiviso per una **Map salvata** ora contiene un'altra Map non assegnata: verifica che la scelta venga effettuata mediante l'ID effettivo, anziché usare sempre la prima Map disponibile.
- Nuovo test click di una **Control Column con override locale**: apertura dell'unico `controlMappingPanel`, preselezione dell'override locale e nessuna duplicazione, riassegnazione o alterazione degli identificatori.
- Nessun cambiamento a UI, Control Mappings, Default Map, saved Map, assegnazioni, motore Audio, generator LTC, frame rate, routing, formato JSON v10 o timecode globale.
- Windows CMake/CTest e version guard da verificare tramite CI della PR; non sono inclusi pacchetti portable né verifiche hardware reali.

## [0.9.6] - 2026-11-02

**Tipo:** verifica e consolidamento dell'editor Control Map condiviso e delle sorgenti Audio LTC; correzione della CI 0.9.5.

### Control Column e Control Mappings
- Confermato e preservato il percorso nativo unico `ControlPanel`: il clic su una Control Column con Map assegnata chiama `editControlSet`, che seleziona la Map salvata usando il suo ID; la sezione Control Mappings usa la stessa classe e gli stessi dati di progetto. Il menu contestuale rimane per assegnare, cambiare o rimuovere una Map, senza mutare gli scope Default/saved/local.
- Conservato il test JUCE della selezione automatica della Map e della mancata duplicazione o riassegnazione quando si apre l'editor.

### Generate LTC Track
- Un'unica regola `canGenerateLtcTrack` determina sia la visibilità della voce nel menu destro sia la validità della conferma: colonna Audio aperta, nessun file o clip PCM esistente e nessun contenuto MIDI/OSC. La regola è riesaminata prima dell'import; nessuna sovrascrittura.
- Corretto il ciclo di vita del dialogo asincrono: la finestra conserva i campi fino al completamento della callback ed è rilasciata in modo deterministico, senza leggere un `AlertWindow` già cancellato automaticamente.
- Aggiunto il frame rate **29,97 non-drop**, accanto ai precedenti **24, 25, 29,97 drop-frame e 30 fps**; il numeratore/denominatore 30000/1001 resta preciso e la numerazione 29,97 NDF non salta le etichette di frame.
- Il WAV continua a essere **PCM mono a 48 kHz/16 bit**, salvato in Documenti e importato come un normale Audio (routing e memoria invariati), senza introdurre un timecode globale.
- Estesi i test: validità del dialogo (mediante regola condivisa), timecode e rollover, generazione non distruttiva e decodifica dei bit audio su numerosi frame consecutivi a ogni rate, oltre al normale caricamento WAV.

### Build e compatibilità
- Corretti due errori C2664 in `tests/PianoRollTests.h` e `tests/OscAutomationV9Tests.h`: il test harness `check(bool,const char*)` richiede messaggi letterali, non espressioni `juce::String`. La compilazione di `0.9.5` era fallita su questi due errori, non nel codice applicativo.
- Nessun cambio agli engine, alle Control Map assegnate, agli effetti del mixer, al Master Audio, al formato dei progetti JSON v10 o alle altre interazioni del redesign. Nessuna build portable.
- Prima del merge: Windows Release, CTest completo e version/changelog guard tutti verdi; decodifica LTC mediante ricevitore indipendente e prove hardware restano verifiche aggiuntive prima dell'uso in uno spettacolo.

## [0.9.5] - 2026-11-02

**Tipo:** redesign nativo V2 — fase N7.2, toolbar responsive e docking editor MIDI/OSC.

### Ergonomia operativa
- Piano Roll MIDI: nel dock compatto (710–999px), **Undo, Redo, Detach, Collapse e Close** sono coordinati nella prima riga senza sovrapposizioni; Snap, Channel, Velocity, Beat Length, Delete, Zoom − e Zoom + restano disponibili e distinguibili nella seconda. Il profilo esteso conserva le posizioni preesistenti.
- La posizione dei pulsanti **Collapse/Close del dock principale** usa, quando MIDI è compatto, la stessa geometria condivisa del Piano Roll. In modalità collassata torna alla posizione originale nella fascia da 36px, senza coprire la Timeline.
- Toolbar OSC: le coordinate dei controlli nativi vengono centralizzate in `EditorToolbarLayout.h`; rimangono invariati i breakpoint, le azioni, gli hitbox e l'organizzazione compatta già verificata.
- Tooltip più chiari per le azioni MIDI/OSC, per distinguere Snap da Grid, zoom, Undo/Redo, eliminazione e Detach/Reattach senza aggiungere pulsanti fittizi o modificare i callback.

### Test e compatibilità
- Test JUCE a larghezze 710, 820, 940, 999, 1000 e 1180px: integrità delle dimensioni effettive, assenza di collisioni tra tutti gli hitbox, comandi sempre visibili e stato Detach/Reattach.
- Test su `MainComponent` a 1180, 1280, 1440 e 1640px: verifica dei veri pulsanti del dock rispetto agli editor e della modalità Collapse/Expand senza sovrapposizioni con la Timeline.
- **Nessuna modifica** a eventi MIDI/OSC, note, curve, Snap, Grid, Undo/Redo, motori, rendering Audio, generazione LTC, formato `.cuelist` JSON v10, routing e Master Audio.
- Nessun pacchetto portable. Richiesti Windows CI/CTest verdi e revisione manuale Windows/DPI/32:9 prima di considerare N7.2 definitivamente approvata.

## [0.9.4] - 2026-11-02

**Tipo:** redesign nativo V2, fase N7.1 — editor MIDI Piano Roll e OSC Automation meno decorativi.

### Interfaccia
- Il Piano Roll usa una testata neutra, una singola barra identificativa MIDI color ambra e una gerarchia editoriale compatta. I tasti del pianoforte conservano la distinzione tra neri e bianchi senza pannelli chiari estranei alla palette Midnight Slate.
- Le note MIDI sostituiscono i pillole arrotondate e sature con rettangoli tecnici; mantengono la lettura della velocity tramite riempimento e indicatore ambra, una selezione visibile e i valori quando lo spazio lo permette. La geometria dei clip e dei target mouse resta invariata.
- L'editor OSC usa testata neutra, un identificatore viola OSC, corsie eventi piatte, marcatori evento rettangolari e automazioni tecniche con bordo selezione. Colori semantici: viola OSC, ambra selezione, testo neutro. Rimossi gli sfondi pastel e le istruzioni sempre visibili; il footer sintetizza eventi e tracce.
- Non cambiano toolbar, note, eventi, curve, manipolazione dei punti, Snap, Grid, parametri, Undo/Redo, selezione, clipboard, zoom, resize/docking, sintesi Audio/MIDI/OSC o serializzazione.

### Test e gate
- Nuovi test nativi JUCE del disegno del Piano Roll (indicatore di velocity, finestra normale e compatta) e dell'editor OSC (eventi e curve a più dimensioni, modello immutato).
- Prosecuzione della regola: una superficie per zona funzionale, colore come segnale e non decorazione. Nessun mockup HTML né build portable.
- Prima del merge: Windows CI/CTest e controllo versione verdi; successivamente prove manuali a DPI 100/125/150%, docking/undocking e dispositivo OSC/MIDI reale.

## [0.9.3] - 2026-11-02

**Tipo:** correzione del test About dopo la transizione alle versioni senza suffisso beta.

### Corretto
- Il test nativo `Native Guide and About stay reachable at every supported width` falliva perché richiedeva ancora il prefisso `0.9.0-beta.`, nonostante `About` già mostrasse correttamente il valore definito da `VERSION.txt` tramite `THE_CUELIST_VERSION`.
- Ora il test verifica la terna numerica SemVer senza essere vincolato a una specifica beta o patch; la corrispondenza con `VERSION.txt` continua a essere garantita da CMake e dal controllo version/changelog.
- Invariati runtime, controlli Guide/About, routing e motori Audio/MIDI/OSC, LTC, Control Mapping, Inspector, mixer, Timeline, docking e schema JSON v10.
- Incrementata per policy la versione applicativa a `0.9.3` con CMake, README e documentazione allineati. La CI precedente della `0.9.2` aveva compilato ma fallito un unico test su 407.122 asserzioni.

### Verifiche richieste
- Windows Release, CTest e version/changelog guard verdi prima del merge; prove hardware, licenze JUCE, DPI/ultrawide e decoder LTC restano gate indipendenti. Nessuna distribuzione portable.

## [0.9.2] - 2026-11-02

**Tipo:** hotfix di build/test dopo il passaggio dalla numerazione beta alla baseline 0.9.1.

### Corretto
- Il nuovo test N6.1 della Timeline utilizzava `controlsV3::named`, helper privato dichiarato in `ControlsV3Tests.h`, incluso **dopo** `TimelineV3Tests.h` da `tests/Tests.cpp`. MSVC produceva `C2653` / `C2672` e interrompeva la compilazione di `CuelistTests`, pur compilando l'eseguibile principale.
- Il test individua ora i pulsanti nativi `timelineSnapToggle` e `timelineGridVisibility` tramite `Timeline::getChildren()` e `Component::getName()`, evitando dipendenze accidentali dall'ordine degli header. Conservati i controlli di visibilità, il test compatto e il controllo che la Timeline non alteri lo show.
- Allineati `VERSION.txt`, versione numerica/regex di `CMakeLists.txt`, README e normativa di versionamento alla patch `0.9.2` prevista dalla regola di incremento SemVer obbligatorio per ogni PR.

### Compatibilità e gate
- **Nessun cambio al runtime JUCE, motori Audio/MIDI/OSC, LTC, Control Mapping, Timeline e docking, Master Audio o formato `.cuelist` JSON v10.** Solo infrastruttura dei test e metadati di versione.
- La CI della 0.9.1 era fallita in compilazione test su questo errore; la 0.9.2 deve superare Windows CMake Release, CTest e il version/changelog guard prima del merge. Le prove hardware/DPI/licenze restano gate separati per la distribuzione commerciale.

## [0.9.1] - 2026-11-02

**Tipo:** passaggio dalla serie `0.9.0-beta` alla versione applicativa **0.9.1** e redesign N6.1 della Timeline.

### Versione e distribuzione
- `0.9.1` diventa la versione canonica in `VERSION.txt`, CMake, About e nell'eseguibile. L'ipotizzata `0.9.0-beta.33` non era stata integrata: N6.1 entra in `0.9.1` senza un'ulteriore versione beta.
- Nessuna GitHub Release o build portable pubblicata automaticamente. Il cambio di numero **non certifica l'affidabilità per spettacoli dal vivo**: prove hardware, DPI/ultrawide, decoder LTC indipendente e licenze di terzi (soprattutto JUCE) restano gate prima della distribuzione commerciale.
- Il formato dei progetti `.cuelist` rimane JSON v10. Conservata integralmente la cronologia cumulativa delle beta.



### Interfaccia
- La Timeline ora utilizza una **toolbar piatta Midnight Slate** al posto della fascia rialzata e del tratto teal puramente decorativo; i tre pulsanti zoom (-, +, Fit) mantengono esattamente gli hitbox reali e hanno superfici tecniche rettangolari.
- La **griglia BPM** è resa in toni neutri e graduati per battuta, beat e suddivisione; è indipendente dal magnete **Snap**, che continua a funzionare anche con griglia nascosta e con cambi BPM in corso di cue.
- Le corsie Audio/MIDI/OSC/Control hanno fondo neutro e un sottile **marcatore del tipo** nella corsia selezionata. I media hanno ora un corpo scuro rettangolare, un bordo misurato e un indicatore di tipo da 3 px, invece delle card traslucide pastel arrotondate.
- La Timeline conserva **waveform Audio, note MIDI, eventi e automazioni OSC, resize grip**, posizioni iniziali e durate, selezione, scrollbar, playhead amber e modalità docked/floating degli editor. Nessuna variazione ai target di drag o alla scala tempo.
- I Control Set mantengono il loro contenuto e si presentano come una corsia strumentale piatta; il footer mostra cue, tempo e zoom senza istruzioni ripetute. La guida delle azioni resta nei tooltip pertinenti.

### Compatibilità e test
- Test JUCE per il nuovo look a 800×250 e la riduzione a 800×115, preservando le azioni Grid e Snap senza collisioni e confermando che il repaint non modifichi il modello.
- Restano attivi i test precedenti su zoom ancorato, pan, snap musicale, grid division separata, drag, MIDI/OSC editor docking e layout responsivo in 1280×768, 1920×1080 e ultrawide.
- Master Audio fissato, mixer N5, editor Control Map, generatore LTC, motori Audio/MIDI/OSC, Undo/Redo, Inspector e JSON v10 invariati. Nessuna build portable.
- Merge subordinato a Windows CI e CTest verdi; verifica hardware e visuale Windows (100/125/150% DPI) obbligatoria prima dell'approvazione della distribuzione. Consultare `docs/0.9-release-checklist.md`.

## [0.9.0-beta.32] - 2026-11-02

**Tipo:** redesign nativo V2 / N5.2 — ergonomia e leggibilità dei controlli mixer.

### Interfaccia
- I meter L/R di `GainMeterFader` mantengono picchi istantanei, peak-hold e scala dBFS, ma sostituiscono la vecchia card rialzata con **due binari rettangolari** su fondo tecnico, un separatore centrale e graduazioni neutre. La tacca 0 dB è leggibile senza usare teal decorativo.
- Il fader adotta un **cursore fisico ribassato** con scanalatura centrale e contorno di focus solo quando riceve input da tastiera. La versione compatta usa la medesima grammatica visiva.
- Il pan pot nativo, condiviso fra Master e colonne Audio, mostra **riferimenti L / centro / R** mediante tre tacche calibrate, una ghiera sobria e un arco teal solo per scostamenti reali dal centro. Il focus tastiera è distinto dallo stato del controllo.
- Tutti i parametri reali, hitbox, drag, doppio clic 0 dB/pan centrato, guadagno e bilanciamento audio, taratura della scala, colori di picco rosso/ambra, rendering L/R, regole +0/+6 dB e scelta delle uscite restano invariati. Le posizioni e le dimensioni di strip e Master Audio non sono state modificate.

### Test
- Test JUCE nuovi per verificare superficie tecnica piatta, risposta visiva dei due meter stereo, movimento del cursore, modalità fader compatta, pan centrato e spostato, oltre all'uso del medesimo LookAndFeel tra Master e Audio. La logica PCM e i test di routing già presenti restano il gate funzionale.
- Nessuna modifica a Engine Audio/MIDI/OSC, Control Mapping, LTC, Cue Grid, Inspector, Auto Stop, Timeline o formato progetto JSON v10. Nessuna build portable.
- PR integrabile solo con Windows CI/CTest verde e verifica visiva Win 100/125/150% DPI, 1280×768 e 32:9.

## [0.9.0-beta.31] - 2026-11-02

**Tipo:** Editor Control Map condiviso e generazione audio LTC da celle Audio vuote.

### Funzioni
- Clic su una Control Column con Map assegnata: apre direttamente il **ControlPanel** originale già utilizzato da Control Mappings, senza passare dal vecchio menu contestuale. Le Map riutilizzabili vengono selezionate per ID; gli override locali mantengono il loro scope. Il menu destro continua a gestire assegnazione, rimozione, salvataggio e Auto Stop. Il Default Map resta distinto.
- Menu destro di una cella Audio **davvero vuota**: nuova voce **Generate LTC Track**. Dialogo nativo per durata (0,1–600 secondi), timecode iniziale HH:MM:SS:FF e frame rate **24 / 25 / 29.97 drop-frame / 30 fps**.
- Generatore LTC SMPTE a **80 bit**: tempo in BCD, sync word, correzione della polarità, codifica biphase-mark, frequenza 30000/1001 per 29,97 DF. Salto delle etichette frame 00/01 ai minuti non multipli di dieci; input non validi rifiutati.
- Output **WAV PCM mono 48 kHz / 16 bit** persistente in Documents/The Cuelist/Generated LTC, importato nel normale percorso Audio (clip, player, mixer, routing, controlli memoria, salvataggio e riapertura). Indipendente dal timecode globale.
- Sicurezza: nessuna sostituzione di celle già occupate; ricontrollo ID riga/colonna alla conferma asincrona; nessuna sovrascrittura di file; output incompleto o import fallito eliminato. Massimo iniziale 10 minuti per sorgente, per contenere la memoria PCM.

### Verifiche
- Test SMPTE per 80 bit, sync, BCD, parità, rollover giornaliero, drop-frame 29.97, validazione input e import del WAV tramite il caricatore Audio reale; verifica delle transizioni biphase dei primi 80 bit.
- Invariati modelli Control Mapping e scope Default/saved/local, motori Audio/MIDI/OSC, Master/Direct routing, Quick Learn, Inspector, editor, metering e schema JSON v10. Nessuna build portable.
- Merge subordinato a Windows CI/CTest verde e verifica con decodificatore LTC esterno.

## [0.9.0-beta.30] - 2026-11-02

**Tipo:** redesign nativo V2 / N5.1 — strip del mixer come strumenti operativi, non card decorative.

### Interfaccia
- **Audio, MIDI, OSC e Control** adottano una superficie piatta condivisa con il mixer, delimitata da separatori sottili anziché da una card rialzata, arrotondata e bordata per ciascuna colonna.
- L'accento funzionale del **tipo di segnale** resta visibile in una sottile linea superiore (Audio teal, MIDI ambra, OSC viola, Control azzurro); le descrizioni Control sono più concise e prive di evidenziazione teal meramente decorativa.
- Anche il **Master Audio** perde il contenitore arrotondato rialzato e usa una superficie sobria. Restano visibili l'intestazione MASTER AUDIO, le sezioni MAIN OUT, PAN, MUTE, LIMITER e le misure reali di guadagno e peak.
- **Geometria e comportamento invariati**: Master fissato nella colonna Cuelist da 318px, altre strip scorrevoli orizzontalmente, fader/meter L/R, pan, mute, polarity, PRE/POST, master send, routing e comandi MIDI/OSC/Control e relativi Learn originali.

### Verifiche
- Nuovi test JUCE su mixer a 1280×820, 1440×900, 1920×1080 e 3840×1080: superficie piatta, colore dei tipi, separazione dal Master, larghezza delle strip e presenza di controlli reali, con serializzazione Show invariata.
- Confermato il principio di design: colore solo per funzione/stato e riduzione di contenitori ridondanti, senza riscrittura di motori Audio/MIDI/OSC, BPM/Auto Stop, Inspector, Cue Grid, Timeline o formato JSON v10.
- Nessuna build portable. Windows CI/CTest e una validazione visiva su Windows/DPI sono necessari prima del merge.

## [0.9.0-beta.29] - 2026-11-02

**Tipo:** redesign nativo V2 / N4 — Inspector più sobrio e professionale, progettato per l'uso operativo.

### Interfaccia
- Rimossi dall'Inspector i tre contenitori rialzati annidati (Entire Cue, Cell e Notes), con ombre simulate, bordi arrotondati e titoli teal ripetuti. L'Inspector diventa una **superficie continua Midnight Slate**, con divisori sottili, etichette neutre e una gerarchia tipografica chiara.
- La testata mantiene il contesto operativo **CUE NN** e il tipo Audio/MIDI/OSC/Control, usando il colore funzionale **solo per identificare il tipo**, non come decorazione su ogni sezione.
- Le sezioni `CUE PARAMETERS`, `CELL PARAMETERS` e `CUE NOTES` usano uno stile editoriale compatto. La struttura dei controlli, la scala dell'Inspector, la funzione del campo Note e le posizioni del toggle di collasso rimangono inalterate.
- Anche la modalità **MIDI/OSC Learn** utilizza ora la medesima superficie piatta, mantenendo il colore del protocollo per il solo stato di apprendimento e lasciando invariati pulsanti, selettore mappe e target.
- L'operatore conserva tutti i veri editor e callback di nome cue, BPM, tempo map, loop, nome cella, gain, import, MIDI/OSC editing e note. Nessun campo o pulsante è stato nascosto o sostituito da un placeholder.

### Test e salvaguardie
- Nuove asserzioni JUCE alle dimensioni 1180×820, 1440×900 e 1920×1080: editor e note restano visibili e nella stessa geometria; i padding dei pannelli sono una superficie unica invece di tre card sovrapposte; il rendering non altera il modello.
- Master Audio ancorato nella colonna Cuelist da 318 px, Cue Grid, BPM/Auto Stop, M/O Learn, motori e routing Audio/MIDI/OSC, Inspector focus/validazione, Undo/Redo, docking e formato JSON v10 invariati.
- Nessuna build portable. Windows CI + CTest verde e verifica visiva a DPI differenti sono requisiti per il merge e per la chiusura definitiva di N4.

## [0.9.0-beta.28] - 2026-11-02

**Tipo:** redesign nativo V2 — affinamento visivo delle celle Audio/MIDI/OSC e test di scroll.

### Interfaccia
- Le celle contenenti realmente una clip mostrano una superficie e un accento di tipo più visibili al passaggio del mouse. I pulsanti **Play e Stop** evidenziano soltanto il loro hitbox effettivo e mostrano tooltip specifici per Audio/MIDI/OSC.
- I rettangoli `cellPlayBounds` e `cellStopBounds` sono usati da **un unico `cellActionAt`** condiviso tra input e rendering. Un clic conserva gli handler originali `triggerCellAt` e `stopCellAt`. Nessuna nuova azione su celle vuote o mancanti; nessun pulsante non visibile aggiunto.
- La selezione, gli indicatori `CONT.` di Auto Stop, looping, clip mancanti, posizione/durata, progress bar e stato PLAY/PAUSE/WAIT restano presenti e distinti. Il movimento del mouse è puramente visivo e non modifica lo show.

### Correzioni
- Sistemata la fixture del test beta.27 `N3.6 cue transport hover targets are exact, distinct and safe`: il test aggiungeva righe direttamente al modello ma richiamava `setSize(1440,900)` sulla dimensione già in uso, senza ricostruire il viewport. Ora esegue `ui.resized()` esplicitamente e distingue un errore di scrolling da un tooltip rimasto visibile.
- Nuovo test con clip Audio realmente caricato e celle MIDI/OSC vuote: hover dei controlli, suggerimenti Play/Stop, ripristino dello stato idle, nessuna mutazione di selezione, Next Cue o Show.

### Compatibilità
- Nessuna modifica a motori Audio/MIDI/OSC, gestione clip, Play/Stop, drag&drop, Inspector, BPM inline, Master Audio fisso nella fascia Cuelist 318px, telemetria e M/O Learn, formato JSON v10 o build portable.
- Windows CI e CTest devono risultare verdi prima di integrare. Resta necessaria verifica visiva DPI/ultrawide.

## [0.9.0-beta.27] - 2026-11-02

**Tipo:** redesign nativo V2 — gerarchia visiva dei trasporti nella colonna Cuelist.

### Interfaccia
- I comandi **GO, Play/Pausa, Stop e menu della singola cue** mostrano ora un feedback differenziato al passaggio del mouse: contorno leggibile per GO, superfici attive per Play/Stop e menu contestuale discreto.
- I quattro comandi conservano rigorosamente **le stesse dimensioni, gli hitbox, i callback e la semantica operativa**. Non vengono cambiate le azioni GO/Stop, la sospensione, il trasporto MIDI/OSC, la selezione, il drag delle righe o la rinomina inline.
- La vista fissa `PinnedCueHeaders` utilizza la medesima geometria di hit-test e painting del `CueGrid`; ogni comando espone un tooltip descrittivo. Il vecchio tooltip sul nome troncato resta invariato.
- L'hover non cambia il progetto né avvia uno show; viene cancellato quando il puntatore esce o quando il viewport scorre, evitando highlight su una riga differente.

### Test e compatibilità
- Nuovi test nativi JUCE per confermare rettangoli indipendenti e coerenti di GO/Play/Stop/Menu, modifica visuale realmente disegnata, tooltip corretti, ripristino dello stato idle e assenza di modifiche a modello, selezione e next cue.
- Restano inalterati header globale da 72px, M Learn e O Learn sotto il logo, I/O Activity passivo, Master Audio fisso nella colonna Cuelist da 318px, audio/MIDI/OSC engines, routing/mapping, Inspector, BPM inline, Auto Stop e schema JSON v10.
- Nessuna build portable; Windows CI + CTest e controllo visivo su Windows/DPI restano necessari prima del merge.

## [0.9.0-beta.26] - 2026-11-02

**Tipo:** redesign nativo V2 — comandi MIDI/OSC Learn separati dalla telemetria.

### Interfaccia
- I due pulsanti **M Learn** e **O Learn** sono stati spostati fuori da **I/O ACTIVITY**, nella seconda fascia dell'header, **sotto il logo The CUElist** e prima delle schede Cuelist (X=10 e 92, Y=43, 78×26 px).
- La posizione rimane fissa e sempre raggiungibile anche a 1024 px; entrambi conservano i **callback reali** `beginQuickLearn(false/true)`, gli stati attivi esistenti e l'evidenziazione MIDI ambra / OSC viola. Tooltip più chiari; nessun menu aggiuntivo e nessun clic in più.
- Il blocco **I/O ACTIVITY** presenta ora esclusivamente MIDI RX/TX e OSC RX/TX, con intestazione più leggibile, nessun controllo sovrapposto e righe di stato distinte.
- `OperatorLearnLayout` centralizza i bounds degli originali pulsanti e della superficie sottostante; `HeaderTelemetryLayout` non contiene più hitbox Learn. L'header resta **72px** e i tab Cuelist iniziano sempre a X=190.

### Test e compatibilità
- Regressioni JUCE: comandi Learn ancora presenti, visibili, con le geometrie nativamente assegnate e senza collisioni con logo, Cuelist, tab e telemetria; I/O ora passivo e righe RX/TX senza sovrapposizioni. Verifiche alle larghezze 1024, 1180, 1249/1250, 1440, 1549/1550, 1920 e 3840.
- Nessuna modifica a logica Quick Learn, mapping MIDI/OSC, engine, routing, controller, clock, CPU/RAM, Master Audio ancorato nella colonna Cuelist, BPM inline o JSON v10. Nessuna build portable.
- Merge subordinato a Windows CI/CTest verde e a verifica visiva Windows/DPI.

## [0.9.0-beta.25] - 2026-11-02

**Tipo:** stabilizzazione del redesign nativo V2 e miglioramento leggibilità telemetria.

### Correzioni
- Risolta la regressione CI ereditata dalla beta.24: il test `N3.4C global header retains 72px tiers and active Cuelist tab` pretendeva ancora la vecchia coordinata `Y=8` e un confine fisso `width-365`, incompatibili con il nuovo cluster responsivo `Y=7`. Il test ora convalida le coordinate **reali condivise**, l'altezza da 72px, tutte le separazioni dei quattro gruppi e l'accessibilità del menu operatore.
- Le misure fondamentali di `HeaderTelemetryLayout` (`top=7`, `panelHeight=58`, `groupGap=10`) sono definite in un'unica sede, senza duplicare valori nei test.
- Barre CPU/RAM più lunghe in modo adattivo: **30px** nel profilo compatto, **40px** nel profilo intermedio, **48px** in quello ampio. Rimangono allineate ai valori percentuali reali e dentro il pannello.
- Nuovo test JUCE sulle larghezze **1024, 1180, 1249, 1250, 1280, 1440, 1549, 1550, 1647, 1920, 3840** per verificare meter, testi, menu e overflow soprattutto nei punti di cambio profilo.

### Compatibilità e verifica
- Invariati refresh e sorgenti clock/CPU/RAM, I/O MIDI/OSC, MIDI/OSC Learn, disponibilità audio, altezza header 72px, schede Cuelist e relativo overflow, BPM inline, Auto Stop, Master Audio fissato nella colonna da 318px, engines e schema JSON v10.
- Nessuna build portable. Windows CI + CTest e verifica visiva a DPI differenti restano necessari prima del merge.

## [0.9.0-beta.24] - 2026-11-01

**Tipo:** redesign nativo V2 — ingrandimento dell'intero monitor operativo nell'header globale.

### Interfaccia
- **Orologio, CPU/RAM, I/O ACTIVITY, MIDI Learn/OSC Learn e AUDIO** condividono ora una fascia Midnight Slate più ampia con separatori sottili, tipografia aumentata, indicatori e valori più leggibili. Nessun ingrandimento verticale dell'header oltre **72px**.
- `HeaderTelemetryLayout::atWidth` definisce **tre larghezze responsive** per 1024–1180, 1250–1549 e ≥1550 px; geometrie unificate di ogni gruppo e dei veri pulsanti Learn. Orologio 30/33/35 px secondo lo spazio, sigla AM/PM separata; valori CPU/RAM 14 px e meter più lunghi; indicatori MIDI/OSC da 10px; stato audio più evidente.
- L'orologio mantiene il formato 12/24 ore, gli indicatori utilizzano i contatori e lo stato audio reali, e le barre CPU/RAM mantengono i valori misurati. I pulsanti MIDI Learn e OSC Learn conservano gli handler esistenti e hanno un target più grande.
- La barra delle schede Cuelist calcola ora l'overflow **rispetto all'effettivo bordo sinistro del monitor**, non a una larghezza precedente fissa: nessuna scheda o menu termina nascosta sotto la telemetria. Lo spazio utile per la Cue Grid e la posizione del Master Audio non cambiano.

### Verifiche
- Aggiornati snapshot/layout della telemetria e dei tab nelle suite JUCE; verifica di tutti e cinque i gruppi, pulsanti Learn, status dots, separatori, non-sovrapposizione, schede e menu su larghezze 1024, 1180, 1280, 1440, 1647, 1920 e 3840.
- Invariati Master Audio fissato nella colonna Cuelist da 318px, routing, motori Audio/MIDI/OSC, Inspector, Timeline, BPM inline e schema `.cuelist` JSON v10; nessuna build portable.
- Richiesti compilazione Windows e CTest verdi prima del merge; verifica visiva Windows/DPI necessaria per il rilascio.

## [0.9.0-beta.23] - 2026-11-01

**Tipo:** hotfix compilazione Windows della topbar V2 introdotta in beta.22.

### Correzioni
- In `src/MainComponent.cpp` l'orologio nativo ingrandito ora usa la variabile `clockTime` di tipo `juce::Time`, distinta dal precedente contatore numerico `now` di attività MIDI/OSC nello stesso metodo.
- Risolti `C2371` (ridefinizione con tipi differenti) e i conseguenti quattro `C2446` (confronto fra `juce::Time` e `double`) per **TheCuelist** e **CuelistTests**.
- Nessuna modifica a orario reale, formato 12/24 ore, refresh, monitor MIDI/OSC, CPU/RAM, layout da 72 px, Master Audio bloccato nella colonna Cuelist o motori audio. La build Windows e CTest devono essere verificati prima dell'integrazione.

## [0.9.0-beta.22] - 2026-11-01

**Tipo:** redesign nativo V2 — rifinitura Cuelist/Transport e Master Audio fisso, con correzione della regressione Windows beta.21.

### Colonna Cuelist
- L'intestazione fissa della Cue Grid conserva l'altezza da **72 px**, l'identità della Cuelist attiva e il conteggio di cue/clip ma elimina la scritta ridondante **CUE / TRANSPORT**. Il titolo è leggermente più leggibile, con separatori e accento teal più discreti.
- Layout, hitbox di rinomina inline, riordino e trasporto GO/Play/Stop/More non modificati. Il Master rimane sotto, allineato alla stessa colonna da **318 px**.

### Master Audio
- Risolta la **regressione CTest Windows beta.21** (`Compact Master Mute is centred below Pan and not over its label`): il Pan torna sopra il Mute con almeno 55 px di separazione e gli indicatori numerici/etichette sono ora in righe non sovrapposte anche a 768 px di altezza.
- La superficie fissa mantiene tutti i controlli nativi e realmente attivi: fader e meter L/R, Main Out, Pan, limiter, mute, Quick Learn, output routing e stato runtime. Eliminata la ridondante etichetta **L/R** in alto che interferiva con il titolo PAN.
- `MasterAudioLayout::atHeight` continua ad alimentare sia paint sia bounds e test, compresi i 180/208 px del pannello. `scrollableStrips` inizia sempre a X=318; il Master non scorre né con il viewport X né con le cue Y.

### Orologio dell'header globale
- Ingrandito il clock nativo da **23 a 30 px**, mantenendo la sua posizione nella fascia di telemetria e il refresh dell'orario reale.
- Le modalità 24 ore e 12 ore restano disponibili. In modalità 12 ore la sigla AM/PM viene renderizzata separatamente in piccolo, per non ridurre l'ora principale o sovrapporsi a CPU/RAM.
- Test del layout e del rendering alle larghezze 1280, 1440, 1647, 1920 e 3840 px; nessuna modifica alle dimensioni generali dell'header (72 px).

### Verifiche
- Espanso il test regressione Pan/Mute di `UtilityPolishTests.h`: finestra 1280×768, 1440×820, 1440×900, 1920×1080; controllo puntuale di allineamento, padding, caption e card containment.
- Restano i test Master Bus/routing/limiter e tutte le verifiche di beta.21, cue, BPM inline, Auto Stop/CONT., Inspector e formati 32:9.
- Nessuna modifica alla logica Audio/MIDI/OSC/Control, routing, mapping, JSON v10 o build portable. Windows CI/CTest verde e revisione visiva su PC sono gate di integrazione.

## [0.9.0-beta.21] - 2026-11-01

**Tipo:** redesign nativo V2 — Master Audio fisso nella colonna Cuelist e stabilizzazione CONT.

### Master Audio
- Il Master Audio resta **ancorato in basso a sinistra, dentro la fascia Cuelist da 318px**, fuori dal contenitore che scorre le strip audio/MIDI/OSC. La geometria di fader, output, pan, limiter, mute e caption è ora centralizzata in `src/MasterAudioLayout.h`.
- Pulizia visiva in JUCE: superficie Midnight Slate uniforme, un accento teal sottile, header leggibile `MASTER AUDIO`, contrassegno `L/R`, divisori discreti e gruppi `MAIN OUT`, `PAN`, `LIMITER`, `MUTE`. Nessun control button fittizio.
- I cinque controlli sono sempre nativi e realmente collegati ai modelli e ai callback esistenti. Meter L/R, Quick Learn, output routing, gain, limiter reduction, pan e mute non vengono sostituiti né semplificati.
- La striscia Cuelist mantiene dimensione fissa in tutte le risoluzioni; la `scrollableStrips` inizia a X=318 e non può sovrapporsi alla superficie Master. Nessuna alterazione di `WorkspaceLayout`, dei viewport o della geometria globale.

### Regressioni
- Risolto il test JUCE `N3.4B CONT.` fallito durante CI beta.20: la comparazione di pixel ora rimuove l'hover dalla cella prima di verificare l'identità visiva del badge ripristinato. La modifica è del **test**, non della persistenza di Auto Stop.
- Aggiunto test di Master Audio pinned per larghezze 1280, 1440, 1647, 1920 e 3840, inclusi scrollbar X/Y: controlli interni alla fascia da 318px, associazione alla utility nativa, strip separatamente scrollabili.
- Nessuna modifica ai motori, alle funzioni GO/Stop, all'Inspector, al mapping/routing, al formato JSON v10 o alla build portable.
- Merge solo con Windows CI/CTest verde e successivo controllo visuale Windows/DPI.

## [0.9.0-beta.20] - 2026-11-01

**Tipo:** Native V2 / N3.5 — stabilizzazione regressioni e rendering finale dell'header e della Cue Grid.

### Correzioni
- Risolta la **compilazione Windows fallita della beta.19**: gli ultimi test `AutoStopV7Tests.h` non fanno più riferimento a `CueMetricsOverlay::cellDetailsBounds` e `CueGrid::autoStopDetailsVisible`, eliminati nella beta.18. I test ora verificano l'area CELLS effettivamente inerte, il metodo esistente `setCellAutoStop`, il marker `CONT.` e la sincronizzazione Inspector.
- L'editor BPM inline segue una cue mediante ID stabile e **si chiude necessariamente** quando quella cue esce dall'area visibile, anche se il valore è invalido: un valore valido viene salvato, uno invalido scartato, senza lasciare un campo attivo nascosto sotto la testata fissa.
- Il passaggio tra due editor BPM di cue differenti termina prima l'editing precedente e non riutilizza il vecchio testo sulla cue successiva.
- Corretti due warning MSVC in `CueViews.cpp` (conversione implicita Y del separatore e shadowing di `localY`) senza modifiche alle interazioni.

### Verifiche
- Estesi test nativi JUCE: hover/scroll dei BPM, riposizionamento nell'area visibile, chiusura con inserimento valido/invalido, identità della cue, persistenza/Inspector dell'Auto Stop senza CELLS.
- **Master Audio intoccato**; invariati tutte le geometrie e gli handler della topbar beta.19, cue grid, mixer, motori, routing e progetto `.cuelist` JSON v10. Nessuna distribuzione portable.
- La CI Windows/CTest e la revisione visiva sul PC restano gate per la PR.

## [0.9.0-beta.19] - 2026-11-01

**Tipo:** redesign nativo delle due intestazioni — topbar globale e fascia fissa BPM/Auto Stop.

### BPM / Auto Stop
- Rimosse le etichette di stato ridondanti NEXT/LIVE/PAUSED/EDIT BPM sotto i BPM e la testata **CUE METRICS**; restano un valore BPM ben leggibile e le sole intestazioni BPM / AUTO STOP.
- Il clic sul BPM apre un **TextEditor JUCE inline** direttamente nella cella, non una finestra popup. **Enter** conferma, **Esc** annulla, perdita di focus conferma un dato valido o annulla quello errato.
- Validation numerica rigorosa per **20–400 BPM** e `validateShow` prima di modificare il modello; modificato solo il BPM iniziale della cue identificata tramite ID stabile. I tempi successivi, l'Inspector e il comportamento preesistente `changed(...,true)` restano invariati.
- L'editor segue la geometria condivisa del viewport ed evita di comparire sopra la testata fissa quando si scorre; resta invariata la logica del toggle Auto Stop di riga.
- Resta integralmente valida la rimozione di **CELLS** della beta.18: le eccezioni delle singole celle sono accessibili dai menu/Inspector, con marker CONT.

### Header globale
- La barra principale resta alta **72 px** con due piani visivi: navigazione operatore e schede Cuelist; separatori più leggeri e tipografia Exo 2 più leggibile.
- Scheda Cuelist attiva con linea teal sottile e background meno marcato; rinomina inline, drag & drop e pulsanti di gestione mantengono gli handler nativi.
- CPU/RAM, orologio, I/O MIDI/OSC e disponibilità AUDIO mantengono **coordinate e dati reali** ma condividono un'unica fascia compatta con separatori discreti invece di tre riquadri autonomi.
- Le funzioni File/Edit/Show/Cuelists/Devices/View/Guide e overflow restano accessibili, con hover minimale.

### Verifica e compatibilità
- Test JUCE: Enter/Esc e valori invalidi; editing senza `DialogWindow`; scroll e ID stabile; geometrie e marker delle schede; menu e indicatori ai diversi formati.
- Master Audio, mixer/strip, Inspector, Timeline, motori Audio/MIDI/OSC, routing, mapping e struttura JSON v10 invariati. Nessuna build portable.
- PR subordinata a Windows CI + CTest e verifica visiva/operativa DPI prima del merge.

## [0.9.0-beta.18] - 2026-11-01

**Tipo:** N3.4B — semplificazione Auto Stop approvata: eliminare `CELLS` e mostrare solo le eccezioni.

### Interfaccia e funzionamento
- Rimosso completamente il pulsante **CELLS** dalla fascia BPM/Auto Stop e lo stato temporaneo legacy `autoStopVisibleRows` da `CueGrid`: nessuna nuova modalità di espansione o pulsante sostitutivo.
- Il controllo **Auto Stop della cue** resta fisso nella colonna da 184 px, con identici hitbox BPM e toggle e senza azioni nascoste nell'area precedentemente occupata da CELLS.
- Le singole celle seguono l'Auto Stop della cue per impostazione predefinita. Solo quelle con `cell.stopOnNext=false` mostrano un badge discreto **CONT.** (continuazione), con spiegazione al passaggio del mouse. Nelle colonne compresse il badge si riduce a una tacca ambra con lo stesso tooltip.
- Il menu contestuale della cella offre **Includi nell'Auto Stop** con spunta; la voce è presente anche nei menu Control Map, mentre il controllo preesistente dell'Inspector continua a funzionare.
- Nessuna perdita di informazioni: le esclusioni vengono conservate nel campo esistente `cell.stopOnNext` del progetto `.cuelist` e continuano a usare `MainComponent::setCellAutoStop`.
- Nome file/clip, loop, progressione e Auto Stop rimangono leggibili senza sovrapporsi al badge; il badge non attiva clip, trasporti o cue.

### Test e limiti
- Aggiornati i test N3.4: due soli target cliccabili nella fascia BPM/Auto Stop, nessun click residuo nella vecchia area CELLS, scroll X/Y.
- Aggiunti test native JUCE per comparsa/scomparsa di CONT., tooltip, indipendenza e persistenza di eccezioni Audio/Control, tacca nelle colonne compresse.
- Nessuna modifica a motori Audio/MIDI/OSC, routing, mapping, Master Audio/strip/mixer, Timeline, Inspector, dati JSON v10 o pacchetti portable.
- Merge subordinato alla CI Windows + CTest e alla validazione visiva/operativa.

## [0.9.0-beta.17] - 2026-11-01

**Tipo:** redesign nativo V2 / N3.4 — BPM e Auto Stop fissi, allineati alle righe N3.3.

### Interfaccia JUCE
- La fascia fissa da **184 px** segue la stessa alternanza Midnight Slate, selezione a tutta larghezza, evidenziazione NEXT e playback reale delle cue; i separatori orizzontali sono sincronizzati con i 62 px delle righe native.
- Header fisso da **72 px** coerente con N3.2, titolo contestuale `CUE METRICS`, etichette BPM/Auto Stop, divider leggero e binario teal continuo da 3 px.
- BPM sempre modificabile tramite l'editor corrente; sotto il valore appare una piccola indicazione `NEXT`, `LIVE`, `PAUSED` o `EDIT BPM` derivata dallo stato effettivo.
- I tre controlli restano indipendenti: area BPM 91×62, switch Auto Stop 78×26 e disclosure CELLS 68×20. I rettangoli sono ora **condivisi fra painting, hover e mouseDown**, senza introdurre dati persistiti né pulsanti simulati.
- Hover discreto e indicazione del cursore soltanto su zone attive, senza selezionare la cue o cambiare il trasporto; intestazione e binario sono ridisegnati dopo le righe scrollabili.

### Regressioni e salvaguardie
- Test nativi JUCE di geometrie, corrispondenza cromatica della riga selezionata fra griglia e fascia fissa, azioni Auto Stop/CELLS con scroll X/Y e hover che non muta lo show.
- Preservati editor BPM, toggle Auto Stop di riga e cella, scroll singolo JUCE, 13 cue/11 colonne e viste 1280×720, 1647×895, 1920×1080 e 3840×1080.
- **Master Audio e tutti i suoi controlli invariati.** Nessun cambiamento a `ColumnUtilities.cpp`, Inspector, Timeline, mixer, editor, motori, routing/mapping, dati `.cuelist` v10 o pacchetti portable.
- N3.5 sarà la verifica integrata visuale/operativa e la stabilizzazione su Windows/DPI; merge subordinato a CI Windows/CTest e revisione.

## [0.9.0-beta.16] - 2026-11-01

**Tipo:** redesign nativo V2 / N3.3 — righe, celle e stati della Cue Grid, senza riscrivere la logica operativa.

### Interfaccia JUCE
- Le righe cue formano una matrice continua con separatori molto sottili, lieve alternanza Midnight Slate e selezione estesa all'intera larghezza, anche oltre l'ultima colonna.
- Selezione, NEXT e playback reale hanno indicatori indipendenti anche quando coesistono; i testi operativi PLAYING, PAUSED, NEXT CUE, MUTED e STANDBY restano presenti. Non sono introdotti stati READY/ERROR/STOPPED artificiosi nel runtime.
- Celle vuote senza riempimenti, bordi o trasporti disabilitati ripetuti; hover discreto senza effetti sullo show. Le aree dei vecchi pulsanti disabilitati non bloccano la selezione della cella; focus/selezione restano separati dall'attivazione.
- Clip e mappe caricate mantengono accenti per tipo, nome, durata, progress bar, play/stop, loop e Auto Stop per cella. Indicazione puntuale di attività soltanto da `getCellPlayback`; attese/pausa e media missing restano segnalate con testo.
- Le colonne collassate distinguono il contenuto reale da una cella vuota; gli indicatori Auto Stop per cella sono portati a 10pt.

### Regressioni e vincoli
- Test JUCE di snapshot della selezione a tutta larghezza, hover reversibile senza mutazione del modello, media mancante e clip caricata; rimangono i test di N3.1/N3.2 e i test dell'esecuzione.
- Invariati gli hitbox di GO, Play/Stop, import, menu, duplicazione, cancellazione, drag/drop ed editing Audio/MIDI/OSC/Control.
- Conservate le metriche originarie (62px riga, 72px header, 318px lane cue, 152/48px colonne, 184px BPM) e la sincronizzazione mixer/scroll.
- **Master Audio intatto**: nessuna modifica a `ColumnUtilities.cpp`, Inspector, Timeline, barre di menu, motori/routing/mapping o formato `.cuelist` JSON v10. Nessuna build portable.
- CI Windows/CTest e revisione visiva su PC con DPI differenziati restano criteri necessari prima del merge. N3.4/N3.5 rimangono incrementi separati.

## [0.9.0-beta.15] - 2026-11-01

**Tipo:** complemento incrementale della N3.2 — layout su due fasce della colonna Cue Name/Transport.

### Modificato
- Titolo della cue ampliato da 94 a **196 px**, preservando la larghezza complessiva di 318 px della lane Cue/Master e la riga da 62 px.
- Play, Stop e menu operativo disposti sulla seconda fascia accanto al numero/stato; GO resta separato e non viene attivato dalla semplice selezione.
- Stessa geometria per paint, hit testing, drag della cue, doppio click e posizione dell'editor inline. Gli offset dello scroll verticale continuano a essere condivisi con la lane fissa.
- Tooltip della lane Cue mostrato soltanto se il testo eccede la larghezza visibile, mantenendo i tooltip delle colonne e lo stile delle intestazioni già integrati nella beta.14.

### Regressioni e limiti
- Test JUCE aggiornati per le nuove hitbox e l'assenza di intersezioni, incluso click sulla zona aggiuntiva del titolo e rinomina inline.
- Invariati mixer, strip, **Master Audio**, Inspector, Timeline, sidebar, motori, formati `.cuelist` JSON v10, gestione delle celle e sincronizzazione BPM/Auto Stop.
- Mantiene N3.1 e N3.2 originali. Non include ancora le nuove superfici delle celle e gli stati della N3.3 né la compattazione definitiva 36–40 px.
- Verifiche Windows Actions/CTest e screenshot su Windows sono gate prima del merge. Nessuna build portable.

## [0.9.0-beta.14] - 2026-11-01

**Tipo:** fase N3.2 del redesign nativo Cue Grid: intestazioni e Cue/Transport.

### Grafica JUCE
- Intestazioni della matrice in Midnight Slate continuo:
  superfici meno stratificate, regole di tipo sottili da 3px,
  separatori verticali e etichette Audio/MIDI/OSC/Control
  identificate anche testualmente.
- Gerarchia migliorata per nome Cuelist, contatore cue/clip,
  titoli delle colonne e nomi/stati Cue; gli stati PLAYING,
  PAUSED, NEXT CUE e MUTED sono distinti senza affidarsi
  soltanto al colore.
- Frecce, pulsanti di gestione, collasso e menu contestuale
  visualmente più leggeri, senza restringere la relativa
  superficie interattiva.
- Tooltip con titolo completo sulle intestazioni cue e
  colonna pinned, che seguono gli offset reali del viewport.

### Regressioni e perimetro
- Test nativi in `tests/CueGridN3VisualTests.h`: paint
  dello header tramite screenshot JUCE, accenti cromatici,
  hitbox Play/Stop/Menu, tooltip di nomi lunghi e selezione
  senza GO accidentale. Restano le suite precedenti.
- `docs/UI_V2_NATIVE_PHASE_N3_2.md` documenta scope e
  verifiche della fase.
- Non è ancora una riduzione delle righe a 36–40px:
  dimensioni originali, celle, motori, layout Inspector,
  Timeline, menu, mix e formato show JSON v10 invariati.
- **Master Audio non modificato**: nessun intervento al
  componente JUCE originale o alle sue funzionalità.
- Nessuna build portable.

## [0.9.0-beta.13] - 2026-11-01

**Tipo:** prima implementazione JUCE della fase N3 (N3.1 — fondazioni geometriche della Cue Grid), senza ancora cambiare la grafica.

### Modifiche native
- Nuovo contratto puro `src/CueGridGeometry.h` con le **dimensioni originali**, larghezze di colonne espanse/collassate, coordinate e hit test delle celle, conversioni per le righe e gli overlay fissi, scroll e banda BPM/Auto Stop.
- `CueGrid` ora delega alla stessa fonte le posizioni e larghezze delle colonne e il relativo hit testing; anche `PinnedCueHeaders`, `PinnedColumnHeaders` e `CueMetricsOverlay` usano le conversioni comuni.
- Default `WorkspaceLayoutInput` e margine di scorrimento derivati dalla medesima sorgente.
- Nessuna diminuzione prematura della riga da 62 px o delle hitbox: la compattezza 36–40 px richiederà la fase N3.2/N3.3 con test dedicati.

### Verifiche
- `tests/CueGridGeometryTests.h`: 13 cue, 11 colonne, collasso misto, scroll X/Y, limiti dei click e geometria a 1280×720, 1647×895, 1920×1080 e 3840×1080.
- Inventario dei componenti, responsabilità e limiti della fase:
  [`docs/UI_V2_NATIVE_PHASE_N3_1.md`](docs/UI_V2_NATIVE_PHASE_N3_1.md).

### Funzionalità preservate
- Il mixer continua a sincronizzarsi con l'unico viewport esistente.
- Banda BPM/Auto Stop, GO/Stop, riga/cella, editor, DnD, mapping, tema, Inspector e Timeline invariati.
- **Master Audio rigorosamente intatto**: nessuna modifica al paint, dimensioni o callback.
- Nessuna modifica al formato dei salvataggi `.cuelist` JSON v10, audio/MIDI/OSC o al motore. Nessuna build portable.

## [0.9.0-beta.12] - 2026-11-01

**Tipo:** correzione dell'architettura dei menu JUCE N2.1; parità funzionale con le opzioni del vecchio Settings.

### Corretti
- Rimossi i duplicati **Pre-flight** e **Diagnostics** da File/View:
  compaiono ora **una sola volta**, nel menu Show.
- Nessun generico "Settings" o "MIDI / OSC Settings" che ripresenti
  il vecchio menu in un altro punto: Devices include ora
  direttamente **Audio Devices**, **MIDI Inputs**,
  **MIDI Outputs**, **OSC Input** e **Control Mapping**.
- Ripristinate e rese visibili le impostazioni del menu originale:
  input MIDI principale e aggiuntivi, attivazione/disattivazione
  delle uscite MIDI, configurazione/ascolto UDP OSC e IP locale,
  intervallo fader 0/+6 dB (**View → Fader Range**),
  formato orario 24/12 h (**View → Clock**).
- Le scorciatoie MIDI/OSC Learn rimangono nella telemetria
  superiore; eliminati i doppioni nel menu Devices. Collapse all
  rimane nel cue header anziché apparire anche in View.
- I menu MIDI usano i device ID rilevati all'apertura, evitando
  associazioni errate se l'enumerazione cambia durante il popup.
- Il menu Cuelists mostra spostamento/eliminazione soltanto
  quando l'operazione è effettivamente possibile, evitando
  voci disabilitate inutili con un solo elenco.

### Verifiche e garanzie
- Nuovo test `test_native_operator_menu_parity.py` eseguito
  nelle CI di versione e Windows; documentato l'inventario
  originale nel
  [menu audit N2.1](docs/UI_V2_NATIVE_MENU_AUDIT.md).
- **Master Audio immutato**; nessuna modifica a
  `ColumnUtilities.cpp`, codec, motori, progetto JSON v10,
  GO/Panic, cue grid e strip.

## [0.9.0-beta.11] - 2026-11-01

**Tipo:** fase N2 del redesign JUCE — rimozione delle duplicazioni dalla sidebar; continuazione del branch `feat/native-operator-header-beta10`.

### Modificato
- Nascosti i pulsanti duplicati **New, Open, Save, Settings,
  Preferences e Control Mapping** nella sidebar, poiché sono ora
  richiamabili dai menu nativi File, Show e Devices della fase N1.
- Gli stessi callback JUCE originali sono conservati; File
  continua a gestire apertura/salvataggio e conferme di abbandono.
- Settings conserva ogni voce e impostazione esistente:
  il popup è ora ancorato al menu Show visibile, anziché
  al vecchio pulsante Preferences nascosto.
- Eliminato il riquadro PROJECT con numero cue/colonne duplicato.
  Rimane un solo nome show editabile, spostato nella zona
  WORKSPACE; l'indicatore modifiche non salvate appare
  soltanto quando serve.
- **ON DECK, GO/PANIC, shortcut, cue grid, inspector,
  mixer, Master Audio e diagnostica restano invariati.**
- Aggiornati i test nativi per verificare sidebar pulita,
  show editabile, accessibilità File/Show/Devices/More
  e il formato show v10 senza regressioni.

### Documentazione
- [UI V2 N2 — Sidebar](docs/UI_V2_NATIVE_PHASE_N2.md).
- Nessuna nuova dipendenza, nessuna build portable.

## [0.9.0-beta.10] - 2026-11-01

**Tipo:** prima integrazione reale JUCE del redesign V2 (fase N1: navigazione superiore); nessun intervento sul Master Audio.

### Implementato
- Barra superiore nativa con icona compatta, identità The CUElist e
  menu **File, Edit, Show, Cuelists, Devices, View, Guide**.
- Popup JUCE funzionali collegati ai metodi già esistenti
  dell'applicazione: Nuovo/Apri/Salva, Undo/Redo,
  cue e colonne, Settings, Cuelist, dispositivi,
  Control Mapping, MIDI/OSC Learn, Inspector, Guide/About.
- Overflow **More** a larghezze ridotte, che raggruppa
  le categorie non visibili senza perdere i comandi
  (1024/1180/1280 px testati).
- Sostituiti nella vecchia barra superiore i pulsanti diretti
  Help, Undo e Redo con i nuovi menu; mantenuti i callback
  e le scorciatoie originali.
- Conservati temporaneamente i comandi di sidebar
  per evitare regressioni prima della sua fase nativa separata.
- Inseriti test CTest JUCE in `OperatorNavigationTests.h`
  su geometria, overflow, presenza pulsanti veri, collegamenti
  originali e permanenza del Master.

### Vincoli
- **Master Audio originale intoccato**: nessuna modifica a
  `ColumnUtilities.cpp`, routing, fader/meter stereo,
  pan, mute, Main Out, limiter e Quick Learn.
- Nessuna modifica a motori, modello Show v10, cue,
  timeline, Inspector, MIDI/OSC/Control, mixer o
  comportamenti di salvataggio.
- Fase N1 documentata in
  [UI_V2_NATIVE_PHASE_N1.md](docs/UI_V2_NATIVE_PHASE_N1.md).
  L'integrazione del resto della GUI è successiva.

## [0.9.0-beta.9] - 2026-11-01

**Decisione di design aggiuntiva:** MASTER AUDIO ESCLUSO DAL REDESIGN.
Il componente JUCE originale, il suo disegno e la sua geometria, tutti
i controlli e i callback devono essere preservati senza modifiche:
fader/meter stereo, Pan, Mute, Main Out, limiter/gain reduction e Quick Learn.
La preview HTML sostituisce il Master sperimentale con una semplice
rappresentazione statica indicativa del componente originale; non è un
nuovo Master né una specifica da implementare in JUCE. Le altre
strip Audio/MIDI/OSC/Control possono essere rifinite.

**Tipo:** rifinitura mirata del concept Operator Workspace V2.3; nessun cambiamento ai sorgenti e ai motori JUCE di produzione.

### Migliorato
- **Mixer integrato più leggibile:** mantenuta la continuità geometrica
  delle 11 colonne con la matrice e lo stesso scroll orizzontale.
  Audio conserva fader, meter e segnali visivi di routing, pan,
  mute, inversione di fase, invio Master L/R e PRE/POST;
  MIDI mostra porta, stato TX e mute della trasmissione;
  OSC mostra endpoint UDP, TX e mute della trasmissione;
  Control mostra priorità, stato del mapping e accesso a Edit set.
- **Gerarchia tipografica:** valori di gain, cue name, legenda
  colonne e stati delle strip più leggibili, con contrasto dei
  testi secondari leggermente incrementato senza introdurre
  nuove cornici o cambiare l'identità Midnight Slate.
- **Sidebar ripulita:** eliminata la nota temporanea sulla
  collocazione dei menu, sostituita con un breve riepilogo
  di show (12 cue list, 13 cue, 11 colonne).
  GO/Panic e on-deck rimangono immediatamente visibili.
- I menu superiori non scompaiono più nei layout stretti
  (fino a 1280px), evitando che funzioni spostate
  dalla sidebar diventino irraggiungibili.
- **Master V2.2 invariato:** conservati gain/meter stereo,
  Main Out, limiter e gain reduction, pan/mute e Quick Learn;
  nessuna riduzione funzionale del prototipo.

### Verifica e limitazioni
- Test esteso di parità del concept a confronto con i nomi
  dei controlli JUCE realmente esistenti in
  `src/ColumnUtilities.cpp`, più test per la sidebar.
- La preview HTML non attiva transport, dispositivi,
  fader, mute, routing o mapping: i dati sono dimostrativi.
  Nessun file `src/` modificato, nessun intervento a
  AudioEngine, MIDI, OSC, Control o schema dello show.
- Misure e leggibilità native su Windows ancora da
  validare prima del trasferimento in JUCE.

## [0.9.0-beta.8] - 2026-11-01

**Tipo:** concept UI V2.2 e gate di parità funzionale — soltanto design, documentazione e test, nessuna modifica ai sorgenti JUCE.

### Corretto
- Rimossa la doppia navigazione dalla sidebar della preview: lo show
  attivo, NEXT, GO e PANIC restano visibili a sinistra; File,
  Edit, Show, Cuelists, Devices, View e Guide espongono le voci
  organizzate in menu superiori dimostrativi, inclusi Settings,
  Preferences, Project info e Control Mapping.
- Sostituito il Master generico a due controlli con la rappresentazione
  delle funzioni verificate nel codice nativo: gain e peak L/R,
  pan, mute audio, selezione Main Out, limiter di sicurezza
  -1 dBFS, gain reduction e tre target Quick Learn
  (gain, pan, limiter).
- Master mantenuto sotto la cue label nella fascia mixer integrata;
  la parte superiore della cue matrix resta invariata.

### Parità funzionale obbligatoria
- `docs/UI_V2_FEATURE_PARITY.md`: inventario Master proveniente da
  `MainComponent.cpp` e `ColumnUtilities.cpp`, vincoli per Audio,
  MIDI, OSC, Control, inspector, timeline, trasporto, mapping,
  tab, salvataggi e dispositivi. Il redesign nativo richiederà
  una corrispondenza verificabile per ogni funzione preesistente.
- Estesi i test della preview per rilevare omissioni nel Master
  o accessi duplicati e per confrontarli con il codice JUCE reale.

### Limiti
- Nessuna modifica a `src/`, eventi, audio/MIDI/OSC/Control,
  routing, mapping, motori, cue, progetto `.cuelist`
  (schema v10) o GUI attualmente utilizzata.
- I controlli HTML rimangono **preview non operativa**.
  Il limiter, i meter e le opzioni di routing reali saranno
  ancora quelli dei componenti JUCE nativi; la preview
  serve solo a definire la futura posizione dei controlli.
- QA visuale Windows e implementazione JUCE native successive.

## [0.9.0-beta.7] - 2026-11-01

**Tipo:** fase P0 del redesign strutturale UI V2 — studio e prototipo autonomo, senza cambiamenti alla GUI operativa.

### Aggiunto
- `design/operator-workspace-v2.html`: prototipo offline ad alta densità, con 12 Cuelist, 13 cue, 11 colonne miste Audio/MIDI/OSC/Control, cue fissi, banda BPM/Auto Stop fissa, selezione evidente, stato on-deck, Inspector, mixer dock e Timeline. Supporta solo interazioni visive reversibili: cambio tab, selezione cue, densità e collasso pannelli.
- `docs/UI_V2_OPERATOR_REDESIGN.md`: audit JUCE, specifica di product design, geometrie candidate e piano di migrazione graduale senza toccare il runtime.
- `scripts/test_operator_workspace_v2.py`: test standard library su densità, regioni fisse, interazioni accessibili e isolamento hardware del prototipo.

### Iterazione V2.1 — feedback sulla demo
- Parte superiore del concept confermata; modificata solo la regione inferiore.
- Mixer non più disegnato come dock indipendente pieno di card:
  Master allineato alla zona cue, ogni canale nella stessa colonna
  Audio/MIDI/OSC/Control della matrice, scroll orizzontale derivato
  dall'unico viewport della cue list.
- Colonne non-audio senza meter/fader fittizi, ma con destinazione,
  stato e controllo semantico; placeholder grafici, non comandi hardware.
- Aggiunta [specifica geometrica V2.1](docs/UI_V2_INTEGRATED_MIXER_SPEC.md)
  e test di allineamento della demo.

### Verifiche e limitazioni
- La nuova preview non usa AudioEngine, MIDI, OSC o il modello show e non può eseguire GO/Panic: i comandi sono dimostrativi.
- Nessuna modifica a `src/`, routing, persistenza v10 o build portable. Il mockup impiega un font browser surrogato; la futura UI nativa conserva Hind/Exo 2.
- La direzione grafica è proposta, non approvata: servono feedback su 1647×895, 720p, 1080p e l'applicazione successiva con test CTest e hardware.

## [0.9.0-beta.6] - 2026-10-31

**Tipo:** aggiornamento di manutenzione e tracciabilità; non è una release pubblica o una certificazione per spettacoli live.

### Aggiunto
- Registro versionato `CHANGELOG.md`, con ricostruzione retrospettiva delle fasi documentate e inventario delle pull request effettivamente integrate.
- Procedura obbligatoria per ogni modifica in `docs/VERSIONING.md`: versione, descrizione, prove e limiti.
- Verifica CI che confronta `VERSION.txt` e la sezione più recente del changelog e richiede incremento di versione per ogni PR successiva.

### Modificato
- Versione sorgente da `0.9.0-beta.5` a `0.9.0-beta.6`, mantenendo l'unica fonte esistente `VERSION.txt`. CMake, versione in-app About, eseguibili e packaging continuano a derivarla da quel file. I pacchetti portable manuali includono ora sia `VERSION.txt` sia `CHANGELOG.md`, verificati dal controllo di integrità.
- README e checklist beta rimandano alla versione corrente in `VERSION.txt`, evitando numeri obsoleti copiati a mano.

### Verifiche e limitazioni
- Validatore SemVer e ricostruzione storica verificati sui sorgenti GitHub; i test Windows JUCE/CTest restano di competenza della CI e dell'ambiente locale.
- Nessuna modifica a routing audio, MIDI, OSC, modello show, interfaccia operativa, API o formato progetto v10.
- La raccolta delle PR precedenti è un **registro retrospettivo**, non implica che quelle versioni fossero pubblicate come release GitHub.

## [0.9.0-beta.5] - 2026-10-28

**Versione attestata:** `VERSION.txt` portato a beta.5 nel commit `3b3a9db`.
**Evoluzione successiva sulla stessa beta:** fino al 31 ottobre sono state integrate ulteriori modifiche senza bump del file VERSION; sono tutte elencate nell'appendice PR, non retroattivamente attribuite a nuove versioni.

### Aggiunto / modificato
- Aggiornamento della documentazione beta, licenza applicativa unica, guida completa e percorsi di packaging (PR #48 e interventi correlati).
- Redesign Midnight Slate; Cuelist multiple/tab, note e Inspector a scomparsa; rinomina inline, Undo/Redo su editor supportati e compact monitor (PR #39–#47).
- Finestra Guide in-app, About con crediti, caratteri Hind/Exo 2 e simboli Noto espliciti, controlli per ultrawide, icone e dialoghi uniformati (PR #50–#72).
- Funzioni astratte e Default Map, MIDI/OSC Learn, override locali per cue, feedback lampeggiante, diagnostica e correzioni di compilazione (PR #73–#80).
- Riordino e rinomina inline di Cuelist e cue; pannello Control Mapping e archivi mappe indipendenti, sincronizzazione MIDI/OSC dei controlli fader/pan e correzioni Windows (PR #81–#88).
- **Limite:** alcune funzioni dell'editor e del mapping sono cambiate dopo il commit che assegnava beta.5; per lo stato preciso consultare il codice e i test dell'ultimo `main`, non una fotografia congelata al 28 ottobre.

### Test / rischio
- Modifiche verificate tramite le PR e le rispettive CI; compatibilità reale con hardware e tempi MIDI/OSC resta da collaudare su postazioni di spettacolo.

## [0.9.0-beta.4] - 2026-10-27

**Versione attestata:** commit `f2334c3`, dichiarato beta.4.
- Test e note di regressione per gain audio, retrigger e Inspector, con comportamento precedente preservato.
- La numerazione del **formato show v10** è distinta dalla versione dell'applicazione.

## [0.9.0-beta.3] - 2026-10-27

**Versione attestata:** commit `48cfa291`, dichiarato beta.3.
- Mixer più compatto, mute MIDI/OSC, attività LED e test del limiter condiviso, con copertura di regressione.

## [0.9.0-beta.2] - 2026-10-27

**Versione attestata:** commit `4efca83`, dichiarato beta.2.
- Routing Master/Direct indipendente e checklist di accettazione hardware esplicita.

## [0.9.0-beta.1] - 2026-10-27

**Versione attestata:** commit `ea1d52e`, dichiarato beta.1.
- Suite di regressione, gate di release e workflow manuale per il packaging portable separato dalla normale CI.
- Il superamento della CI non equivale a certificazione per un utilizzo live.

## [0.8.0-alpha.1] - 2026-10-27

**Versione attestata:** commit `129a22a`.
- Fase architettura/integrazioni: separazione di ciclo progetto, Inspector, operazioni su cue/celle, modello transazionale, layout e routing Control Mapping.
- Controllo più rigoroso del dispatch MIDI e politica di riconnessione/One-shot Learn, con test (PR #25–#33).
- Piano di accettazione hardware esplicitamente non completato.

## [0.7.0-alpha.1] - 2026-10-27

**Versione attestata:** commit `023bab77`.
- Inizio della fase di stabilizzazione: checkpoint di recupero, preflight, diagnostica, caricamento progetti più sicuro e correzione di warning/build, secondo cronologia e note beta.
- Versionamento centralizzato in `VERSION.txt`, senza confondere la versione del programma con lo schema JSON.

## Cronologia precedente (0.5 → sviluppo 0.6) - 2026-10-25 / 2026-10-26

**Nota:** `0.6` compare nelle denominazioni delle PR, ma non risulta un tag di release GitHub o una serie completa di versioni `VERSION.txt` da cui ricostruire patch/alpha numerici con certezza. Per questo **non vengono inventate versioni di patch**.

### Funzioni ricostruite dalle PR integrate
- Prime pipeline Windows e pacchetto portable, controlli di build e checksum (PR #1–#2); documentazione/licenza iniziale (PR #5).
- Editor clip integrato e flottante, modifica MIDI immediata, griglia beat e snap indipendenti (PR #6, #8).
- Editor OSC, messaggi UDP, automazioni tempo/beat, per-cella Auto Stop e mixer Master con gain/pan/meter (PR #9–#16).
- Control Mapping v10, MIDI Learn, preset e routing; test/fix compilazione e miglioramenti della UI (PR #17–#20).
- Le PR #4 e #7 **non furono integrate**; non si presentano come funzionalità consegnate.

## Appendice A — cronologia completa delle PR fino al 31 ottobre 2026

Questo indice registra **ogni PR integrata** visibile nella cronologia GitHub
al momento del bootstrap (PR #1–#88); una riga non indica necessariamente
una versione nuova o una release. Data = data del merge UTC, titolo = titolo
della PR originale. Le PR non integrate sono riportate separatamente.

- **2026-10-31 — [PR #88](https://github.com/Askeladd22/The-CUElist/pull/88):** Fix Control Mappings: in-place map selection, function names and Save/Cancel
- **2026-10-31 — [PR #87](https://github.com/Askeladd22/The-CUElist/pull/87):** Control Mappings: separate map archive, compact mapping list and contextual editor
- **2026-10-30 — [PR #86](https://github.com/Askeladd22/The-CUElist/pull/86):** Fix Windows C2248 compilation error in ControlRuntimeTests
- **2026-10-30 — [PR #85](https://github.com/Askeladd22/The-CUElist/pull/85):** Fix MIDI/OSC real-time fader and pan UI synchronization
- **2026-10-30 — [PR #84](https://github.com/Askeladd22/The-CUElist/pull/84):** Fix independent Control Maps and empty Control Column display
- **2026-10-30 — [PR #83](https://github.com/Askeladd22/The-CUElist/pull/83):** Document mapping rework and validate Windows regression suite
- **2026-10-30 — [PR #82](https://github.com/Askeladd22/The-CUElist/pull/82):** Inline rename and three-way reordering of Cuelists and cue rows
- **2026-10-29 — [PR #81](https://github.com/Askeladd22/The-CUElist/pull/81):** Polish header clock/CPU telemetry and compact cue action buttons
- **2026-10-29 — [PR #80](https://github.com/Askeladd22/The-CUElist/pull/80):** Fix MIDI Quick Learn yellow blinking when no MIDI input is connected
- **2026-10-29 — [PR #79](https://github.com/Askeladd22/The-CUElist/pull/79):** Fix OSC Timeline layout and add MIDI/OSC Learn blinking feedback
- **2026-10-29 — [PR #78](https://github.com/Askeladd22/The-CUElist/pull/78):** Fix MIDI/OSC Learn: clickable GUI parameters and persistent multi-binding mode
- **2026-10-29 — [PR #77](https://github.com/Askeladd22/The-CUElist/pull/77):** Rework Control Mapping: direct MIDI/OSC Learn, advanced functions and cue overrides
- **2026-10-29 — [PR #76](https://github.com/Askeladd22/The-CUElist/pull/76):** Fix Windows compilation: remove duplicate Control Mapping implementation from PR #74
- **2026-10-29 — [PR #75](https://github.com/Askeladd22/The-CUElist/pull/75):** Fix OSC Control Learn float saturation for extreme finite values
- **2026-10-29 — [PR #74](https://github.com/Askeladd22/The-CUElist/pull/74):** Control Mapping: stable functions and cue-local physical overrides (work in progress)
- **2026-10-29 — [PR #73](https://github.com/Askeladd22/The-CUElist/pull/73):** Introduce abstract control functions, Default Map inheritance and cue-local input overrides
- **2026-10-29 — [PR #72](https://github.com/Askeladd22/The-CUElist/pull/72):** Fix false blank Noto glyph failure by finalizing JUCE Graphics before pixel inspection
- **2026-10-29 — [PR #71](https://github.com/Askeladd22/The-CUElist/pull/71):** Fix MSVC C2664 in Noto UI regression tests
- **2026-10-29 — [PR #70](https://github.com/Askeladd22/The-CUElist/pull/70):** Fix blank Noto transport glyphs and failing 0.9 UI regression
- **2026-10-29 — [PR #69](https://github.com/Askeladd22/The-CUElist/pull/69):** Unify and soften icon-button frames across the operator UI
- **2026-10-29 — [PR #68](https://github.com/Askeladd22/The-CUElist/pull/68):** Correct ultrawide test for intentionally off-screen collapsed Inspector
- **2026-10-29 — [PR #67](https://github.com/Askeladd22/The-CUElist/pull/67):** Diagnose and resolve 32:9 ultrawide workspace test regression
- **2026-10-29 — [PR #66](https://github.com/Askeladd22/The-CUElist/pull/66):** Pin all Cue List column headers and centre Noto Mute glyphs
- **2026-10-29 — [PR #65](https://github.com/Askeladd22/The-CUElist/pull/65):** Fix disappearing Audio/MIDI/OSC headers during cue scrolling
- **2026-10-29 — [PR #64](https://github.com/Askeladd22/The-CUElist/pull/64):** Audit Unicode UI icons and use explicit Noto Sans Symbols 2 glyphs
- **2026-10-29 — [PR #63](https://github.com/Askeladd22/The-CUElist/pull/63):** Eliminate the 9px Cue List / BPM gutter at its source
- **2026-10-29 — [PR #62](https://github.com/Askeladd22/The-CUElist/pull/62):** Fix Control Mapping test race from asynchronous ComboBox notifications
- **2026-10-29 — [PR #61](https://github.com/Askeladd22/The-CUElist/pull/61):** Fix ControlsV3 test compilation after shared editor refactor
- **2026-10-29 — [PR #60](https://github.com/Askeladd22/The-CUElist/pull/60):** Unify cue editor dialogs and Control Mapping; polish reports and BPM seam
- **2026-10-29 — [PR #59](https://github.com/Askeladd22/The-CUElist/pull/59):** Polish Diagnostics/About dialogs, Noto fader menu, Inspector grip and cue divider
- **2026-10-29 — [PR #58](https://github.com/Askeladd22/The-CUElist/pull/58):** Fix Auto Stop and Play/Stop regression tests after visual polish
- **2026-10-29 — [PR #57](https://github.com/Askeladd22/The-CUElist/pull/57):** Polish cue actions, telemetry, faders and explicit Noto font rendering
- **2026-10-29 — [PR #56](https://github.com/Askeladd22/The-CUElist/pull/56):** Fix stale Collapse-all layout assertion after cue header resize
- **2026-10-29 — [PR #55](https://github.com/Askeladd22/The-CUElist/pull/55):** Fix pinned cue headers, transport spacing and Expand all tooltip
- **2026-10-29 — [PR #54](https://github.com/Askeladd22/The-CUElist/pull/54):** Fix Unicode symbols using embedded Noto font fallback
- **2026-10-28 — [PR #53](https://github.com/Askeladd22/The-CUElist/pull/53):** feat(help): native in-app Guide viewer and consistent menu labels
- **2026-10-28 — [PR #52](https://github.com/Askeladd22/The-CUElist/pull/52):** fix(build): restore JUCE 8 Windows compilation for Exo 2 / Hind typography
- **2026-10-28 — [PR #51](https://github.com/Askeladd22/The-CUElist/pull/51):** design(type): Exo 2 for small text (min 10pt), Hind for headings
- **2026-10-28 — [PR #50](https://github.com/Askeladd22/The-CUElist/pull/50):** feat(ui): Help/About, co-author credits and compact Inspector panel icons
- **2026-10-28 — [PR #49](https://github.com/Askeladd22/The-CUElist/pull/49):** fix(tests): make Midnight Slate popup checks renderer-independent
- **2026-10-28 — [PR #48](https://github.com/Askeladd22/The-CUElist/pull/48):** Docs/refresh beta5 and license
- **2026-10-28 — [PR #47](https://github.com/Askeladd22/The-CUElist/pull/47):** design(ui): clean Cuelist heading and improve left sidebar spacing
- **2026-10-28 — [PR #46](https://github.com/Askeladd22/The-CUElist/pull/46):** feat(ui): bidirectional Cuelist renaming, Undo/Redo and compact signal monitor
- **2026-10-28 — [PR #45](https://github.com/Askeladd22/The-CUElist/pull/45):** fix(cuelists): inline responsive tabs, delete action, numbering and popup test
- **2026-10-28 — [PR #44](https://github.com/Askeladd22/The-CUElist/pull/44):** design(ui): consistent Midnight Slate dropdown and context menus
- **2026-10-28 — [PR #43](https://github.com/Askeladd22/The-CUElist/pull/43):** fix(inspector): Escape from cue notes reaches global Panic
- **2026-10-28 — [PR #42](https://github.com/Askeladd22/The-CUElist/pull/42):** design(theme): balanced Midnight Slate palette between both UI references
- **2026-10-28 — [PR #41](https://github.com/Askeladd22/The-CUElist/pull/41):** fix(windows): Inspector collapse test build and unused variable
- **2026-10-28 — [PR #40](https://github.com/Askeladd22/The-CUElist/pull/40):** feat(ui): Inspector collapsible to the right via persistent side tab
- **2026-10-28 — [PR #39](https://github.com/Askeladd22/The-CUElist/pull/39):** feat(gui): Cuelist tabs, cue notepad, collapsible columns and live system panels
- **2026-10-27 — [PR #38](https://github.com/Askeladd22/The-CUElist/pull/38):** Fix stale expanded-dock regression after gap-free Timeline layout
- **2026-10-27 — [PR #37](https://github.com/Askeladd22/The-CUElist/pull/37):** Paint collapsed MIDI OSC dock region as panel to avoid a black Timeli…
- **2026-10-27 — [PR #36](https://github.com/Askeladd22/The-CUElist/pull/36):** Fix Timeline minimum-height clipping and remaining workspace gutters
- **2026-10-27 — [PR #35](https://github.com/Askeladd22/The-CUElist/pull/35):** Fix layout regressions: full-width lower editors, restored columns, resizable Timeline
- **2026-10-27 — [PR #34](https://github.com/Askeladd22/The-CUElist/pull/34):** UI density, pinned cue headers/metrics, selectable audio fader range
- **2026-10-27 — [PR #33](https://github.com/Askeladd22/The-CUElist/pull/33):** Refactor phase 4c: deterministic MIDI reconnect and one-shot Learn session policy
- **2026-10-27 — [PR #32](https://github.com/Askeladd22/The-CUElist/pull/32):** Refactor phase 4b: isolate physical MIDI dispatch decisions and output target lookup
- **2026-10-27 — [PR #31](https://github.com/Askeladd22/The-CUElist/pull/31):** Refactor phase 4a: separate Control Mapping runtime from Control Panel UI
- **2026-10-27 — [PR #30](https://github.com/Askeladd22/The-CUElist/pull/30):** Refactor phase 3b: responsive docked editors on compact windows
- **2026-10-27 — [PR #29](https://github.com/Askeladd22/The-CUElist/pull/29):** Refactor phase 3a: centralize deterministic workspace layout geometry
- **2026-10-27 — [PR #28](https://github.com/Askeladd22/The-CUElist/pull/28):** Refactor phase 2c: transactional cue/column deletion and mapping cleanup
- **2026-10-27 — [PR #27](https://github.com/Askeladd22/The-CUElist/pull/27):** Refactor phase 2b: pure transactional show-model operations
- **2026-10-27 — [PR #26](https://github.com/Askeladd22/The-CUElist/pull/26):** Refactor phase 2a: extract cue and cell editing operations
- **2026-10-27 — [PR #25](https://github.com/Askeladd22/The-CUElist/pull/25):** Refactor phase 1: separate Inspector and project lifecycle from MainComponent
- **2026-10-27 — [PR #24](https://github.com/Askeladd22/The-CUElist/pull/24):** Tighten cue grid margins and Inspector alignment; edit cell Gain as dB text
- **2026-10-27 — [PR #23](https://github.com/Askeladd22/The-CUElist/pull/23):** Remove redundant Cue List heading
- **2026-10-27 — [PR #22](https://github.com/Askeladd22/The-CUElist/pull/22):** Fix crooked Loop Playback glyph alignment
- **2026-10-27 — [PR #21](https://github.com/Askeladd22/The-CUElist/pull/21):** UI/usability polish: project title, contextual actions, activity LEDs and lock icon
- **2026-10-26 — [PR #20](https://github.com/Askeladd22/The-CUElist/pull/20):** Feat/master fader osc timeline 20261026
- **2026-10-26 — [PR #19](https://github.com/Askeladd22/The-CUElist/pull/19):** Control Mapping v10 — conferma GO e icona approvata The Cuelist
- **2026-10-26 — [PR #18](https://github.com/Askeladd22/The-CUElist/pull/18):** Fix Windows compilation: invalid break in ControlUI.cpp
- **2026-10-26 — [PR #17](https://github.com/Askeladd22/The-CUElist/pull/17):** v0.6 — Control & Mapping v10: MIDI Learn, preset cue e routing + icona
- **2026-10-26 — [PR #16](https://github.com/Askeladd22/The-CUElist/pull/16):** Feat/v06 osc automation editor
- **2026-10-26 — [PR #15](https://github.com/Askeladd22/The-CUElist/pull/15):** v0.6 OSC Clip Editor v0.2 — timeline, automazioni e Stop/Restart
- **2026-10-26 — [PR #14](https://github.com/Askeladd22/The-CUElist/pull/14):** v0.6 — GainMeterFader stereo integrato con scala dB comune
- **2026-10-26 — [PR #13](https://github.com/Askeladd22/The-CUElist/pull/13):** v0.6 UX polish — Auto Stop compatto, azioni cue, pan e nomi automatici
- **2026-10-26 — [PR #12](https://github.com/Askeladd22/The-CUElist/pull/12):** v0.6 Fase 6 — revisione UX: celle, Auto Stop, timeline, OSC e mixer
- **2026-10-26 — [PR #11](https://github.com/Askeladd22/The-CUElist/pull/11):** v0.6 Fase 5 — mixer compatto, pan rotativo, meter e Master audio
- **2026-10-26 — [PR #10](https://github.com/Askeladd22/The-CUElist/pull/10):** v0.6 Fase 4 — Auto Stop 2.0 per cella audio, MIDI e OSC
- **2026-10-26 — [PR #9](https://github.com/Askeladd22/The-CUElist/pull/9):** v0.6 Fase 3 — OSC UDP, editor secondi/beat e formato show v6
- **2026-10-26 — [PR #8](https://github.com/Askeladd22/The-CUElist/pull/8):** v0.6 Fase 2: griglia musicale e snap indipendenti nella timeline
- **2026-10-26 — [PR #6](https://github.com/Askeladd22/The-CUElist/pull/6):** v0.6 — Fase 1: Clip Editor integrato, flottante e editing MIDI immediato
- **2026-10-26 — [PR #5](https://github.com/Askeladd22/The-CUElist/pull/5):** Documentazione essenziale e licenza proprietaria perpetua
- **2026-10-26 — [PR #3](https://github.com/Askeladd22/The-CUElist/pull/3):** Correzioni immediate: griglia BPM, snap e selezione multipla MIDI
- **2026-10-25 — [PR #2](https://github.com/Askeladd22/The-CUElist/pull/2):** ci: stabilizzare il pacchetto portable Windows x64
- **2026-10-25 — [PR #1](https://github.com/Askeladd22/The-CUElist/pull/1):** ci: build and verify distributable Windows portable ZIP

### PR non integrate, escluse dalle funzionalità consegnate

- [PR #7](https://github.com/Askeladd22/The-CUElist/pull/7) — The Cuelist 0.6 Fase 1 — Clip Editor integrato e modifiche MIDI immediate (**non integrata**)
- [PR #4](https://github.com/Askeladd22/The-CUElist/pull/4) — Fix Windows portable CI and provide one-extraction download (**non integrata**)
