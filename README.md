# The Cuelist

**Native Windows show control for audio, MIDI and OSC.**

The Cuelist is a desktop application for preparing and executing cue-based live productions, theatre performances, concerts and installations. It brings audio playback, MIDI sequences, OSC events and MIDI-controller mappings into one operator-focused workspace. It is not a digital audio workstation and does not include native video or timecode playback.

**Application version:** see [`VERSION.txt`](VERSION.txt) · **Platform:** Windows x64 · **Project format:** `.cuelist` JSON schema v10 · **Technology:** C++17, JUCE 8.0.12 · **License:** proprietary.

> **0.9.28 adds mouse-wheel gain adjustment and aligns the Timeline, MIDI and OSC editor controls; it is not a live-show certification.** The beta numbering has ended, but rehearsals with real devices, licensing verification and Windows DPI/ultrawide visual review remain necessary before commercial distribution or production reliance. CI alone does not prove hardware reliability or end-to-end timing.

## Versione e storico aggiornamenti

La versione dell'applicazione è definita **esclusivamente**
in [`VERSION.txt`](VERSION.txt), già utilizzato da CMake, About e
packaging. Ogni aggiornamento — funzionalità, correzione, UI, refactor,
test, documentazione o pulizia — incrementa la versione e aggiunge una
sezione in testa al [`CHANGELOG.md`](CHANGELOG.md) con modifiche
e verifiche eseguite. La CI controlla obbligatoriamente la corrispondenza
fra i due file e il bump rispetto a `main`; vedere
[`docs/VERSIONING.md`](docs/VERSIONING.md).

Il changelog include anche una **ricostruzione retrospettiva** delle
versioni 0.7/0.8/0.9 e delle 86 PR precedentemente integrate (con
link alle PR), distinguendo la cronologia di sviluppo dalle release
pubbliche. La versione del progetto salvato `.cuelist` (**schema v10**)
è indipendente dal numero del software.

## 0.9.27 — Temi Legacy e Industrial Orange

- **View → Appearance** (o Settings → Appearance) permette di selezionare **Legacy — Graphite / Copper** o **Industrial Orange**, immediatamente e senza riavvio. La scelta è memorizzata nelle preferenze locali dell'applicazione, non nei file `.cuelist`.
- Il tema Legacy recupera i valori reali da `src/Theme.h` della **0.9.15** (commit pre-Industrial `e6f1c384`): sfondi grafite, accento rame, testo caldo, indicatori GO/PANIC e colori delle colonne originali. Non è un'approssimazione grafica.
- Industrial Orange mantiene la struttura `#101619 / #182227 / #202C32 / #29373D / #303B40 / #35434A`, l'arancio `#C66B42` e adotta i valori funzionali **GO `#55B875`**, **PANIC `#E05A46`**, **WARNING `#E5A52E`**, **INFO `#55AFC5`**. Audio torna all'identità tipo `#8FA8B2`, indipendente dall'Info.
- **Timeline:** il riempimento di ogni clip è derivato dal colore di tipo, con bordo di selezione arancio indipendente e colore del testo adattivo; lo stile Legacy conserva la superficie storica neutra.
- **Cue:** i sette colori personalizzati hanno un riempimento molto più visibile in Industrial Orange; una doppia marcatura chiara/arancio indica la selezione di una cue colorata, senza perdere la tinta assegnata. Il renderer sceglie l'inchiostro chiaro o scuro per contrasto. Il valore salvato resta lo stesso.
- Gli editor e i menu continuano ad usare un unico set di token. Non cambiano routing, playback, geometrie o schema JSON v10.

## 0.9.26 — Fix compilazione test Windows

Corretto il tipo JUCE usato dal test del pulsante Collapse della Timeline: ora è `juce::TextButton*` anziché `juce::Component*`, che non espone `getTooltip()`. Aggiornati anche due test LIVE per eseguire `engine.collectRetired()` tra pubblicazioni consecutive, riproducendo il ciclo del timer principale ed evitando false interruzioni nel motore audio simulato. **Nessuna modifica al codice operativo o al formato .cuelist.** Verificare CTest prima del merge.

## 0.9.25 — Header MIDI iconico e Collapse Timeline

Il Piano Roll MIDI raggruppa **Collapse, Detach, Close** all'estremità destra dell'header: tre pulsanti **28×26 px** con icone vettoriali, coerenti con i controlli esistenti e con tooltip/etichette accessibili. L'editor continua a funzionare sia nel dock che nella finestra separata. La Timeline dispone di un pulsante Collapse direttamente nell'header e di un grip persistente per riaprirla quando è nascosta; la voce View usa la stessa routine e mantiene lo stato persistito nelle preferenze. Non cambiano il playback, i dati dei clip o il formato dei progetti. Test responsive a più larghezze disponibili in CTest.

## 0.9.24 — Modifiche strutturali e MIDI Output senza arrestare il LIVE

L'aggiunta di cue e colonne e il riordino di cue/colonne mantengono il playback, la posizione corrente e il prossimo GO individuato tramite ID stabile, anche se la riga viene spostata. La rinomina e la sola visualizzazione delle Cuelist restano indipendenti dalla LIVE. Il cambio MIDI Output non esegue più PANIC né riavvia il motore audio; un nuovo dispositivo riceve i messaggi dalla posizione corrente e la vecchia stream viene ritirata annullando i pacchetti in coda. La disconnessione o l'assenza di uscita lascia proseguire l'audio, con inoltro MIDI sospeso; il dispositivo selezionato può essere ripristinato per ID esatto. **Delete** della cue/colonna LIVE e modifica del tempo non sono stati alterati: richiedono una politica dedicata di migrazione dello stato prima di rimuovere l'arresto di sicurezza. Test automatici ed esiti nel changelog.

## 0.9.23 — Industrial Orange / contrasti operativi

Rifinitura cromatica conservativa: **verde salvia `#9BB892`** per GO, PLAYING e stati disponibili; **rosso terracotta `#C97870`** per PANIC e segnalazioni di errore; **blu acciaio `#86A6BD`** per indicatori informativi e identificazione Audio. L'obiettivo è aumentare la riconoscibilità sul grafite senza tonalità neon o saturazioni da interfaccia gaming. Arancio identitario, MIDI/OSC/Control, sette colori personali delle cue, layout e comportamenti restano invariati. Aggiunti test sui token e sul contrasto.

## 0.9.22 — Correzione test Windows

La build 0.9.21 compilava correttamente ma un test della vecchia interfaccia continuava a richiedere i pulsanti Undo/Redo invisibili, in conflitto con le icone visibili dopo Guide introdotte nella stessa versione. La 0.9.22 corregge **esclusivamente il test** e verifica icone e menu Edit/More in base allo spazio disponibile, usando le coordinate aggiornate della telemetria audio. Nessuna modifica funzionale o grafica all'applicazione.

## 0.9.21 — OSC Editor e pulsanti Undo/Redo globali

- **OSC**: tasto destro + trascinamento crea una selezione rettangolare multipla, con feedback grafico e operazioni Delete/Duplicate già esistenti; un singolo clic destro apre ancora il menu contestuale. Doppio clic all'interno di una Ramp aggiunge un punto sulla curva, senza duplicare i punti esistenti. Il vecchio dialogo Ramp con Start/End Value è rimosso; i valori vengono modificati trascinando i punti, mentre indirizzo OSC, tipo argomento, indice dell'argomento automatizzato (necessario ai messaggi multiargomento) e range restano nell'intestazione della traccia.
- **Header**: icone vettoriali Undo/Redo immediatamente a destra di Guide quando c'è spazio. In finestre strette si usano Edit/More e le scorciatoie Ctrl+Z, Ctrl+Y e Ctrl+Shift+Z. I comandi condividono lo storico già disponibile e sono disabilitati quando non ci sono azioni.
- Confermate senza riscritture: indipendenza VIEW/LIVE, gestione MIDI Output mancante, strumenti di editing MIDI, menu View, drag&drop Cuelist e palette Industrial Orange. Nessuna modifica al routing, ai file Show o alle dimensioni della Cue Grid. Verifiche descritte nel changelog.

## 0.9.20 — Industrial Orange / gerarchia cromatica

Rifinitura **solo cromatica** della palette approvata: bordo e rail di selezione in arancio bruciato, GO e playback in verde operativo, NEXT/PAUSED e segnalazioni in ambra warning, tipologie Audio/MIDI/OSC/Control nei rispettivi colori indipendenti. Playhead e clip selezionati nella Timeline usano l'arancio, senza alterare i tratti semantici dei clip. Le curve OSC restano viola con outline e punti selezionati arancio; indicatori MIDI/OSC tecnici e meter CPU/RAM non usano più l'accento del brand. Restano inalterati layout, geometria, interazioni, routing, dati dello Show e sette colori assegnabili alle cue.

## 0.9.19 — Undo/Redo condivisi per gli edit dello Show

L'UndoManager già condiviso dagli editor MIDI e OSC include ora le principali modifiche al modello `Show`: struttura e ordinamento di cue, righe, colonne e Cuelist; nomi, colori, Auto Stop, impostazioni clip, Master, assegnazioni e mixer. Gli snapshot conservano i riferimenti ai clip Audio senza duplicare PCM, hanno una cronologia limitata e sono ricostruiti/validati prima del ripristino. Le modifiche a una tab solo VIEW conservano la LIVE; un ripristino che cambia le cue o i parametri live arresta cautelativamente il trasporto. I semplici cambi di visualizzazione e i comandi GO/Stop/PANIC non sono azioni Undo. Le gesture MIDI/OSC mantengono la cronologia locale già integrata nello stesso UndoManager. Test automatici aggiunti per ordine degli interventi, duplicazioni e isolamento della LIVE.

## 0.9.18 — Editor OSC: tracce e selezione multipla

L'editor OSC nativo separa ora le **impostazioni della track** (indirizzo OSC, argomenti tipizzati, indice dell'argomento automatizzato, minimo e massimo) dai **valori della Ramp** (inizio e fine). Il doppio clic sulla testata apre la configurazione condivisa della traccia, il doppio clic su un segmento modifica i suoi estremi. Gli aggiornamenti sono transazionali e preservano la compatibilità con i messaggi multi-argomento e i progetti schema v10. Restano disponibili selezione multipla Ctrl/cornice, spostamento collettivo, Duplicate e Delete; uno spostamento produce una sola transazione Undo. La palette Industrial Orange, il motore e il routing non cambiano.

## 0.9.17 — VIEW e LIVE separati

Cambiare tab seleziona soltanto la Cuelist da visualizzare/modificare. L'indicatore verde sulla tab contrassegna la **LIVE**: il GO globale (incluso Space e comandi mappati) continua ad avanzare la LIVE, senza spostare la VIEW; il controllo mapping e gli snapshot audio/MIDI/OSC mantengono la sequenza live. Un GO esplicito su una cue della VIEW trasferisce invece la riproduzione a quella Cuelist arrestando il trasporto precedente. La gestione delle uscite e il formato dei file non cambiano. Le operazioni distruttive sulla Cuelist live mantengono PANIC di sicurezza; necessaria verifica su hardware reale. Vedi [CHANGELOG](CHANGELOG.md).

## 0.9.16 — Industrial Orange

Nuova identità grafica centralizzata in `src/Theme.h`, con grafite blu-antracite (`#101619`–`#35434A`), arancio bruciato (`#C66B42`) e quattro codici di colonna indipendenti: Audio `#8FA8B2`, MIDI `#B79A72`, OSC `#A18BB9` e Control `#7186A6`. Gli stati GO, PANIC e Warning restano distinti. Preservati i sette colori personalizzati delle cue. La modifica riguarda solo i colori, senza alterare il layout, la logica o il formato dei progetti. Vedi [CHANGELOG](CHANGELOG.md).

## 0.9.15 — Palette Graphite / Copper

L'interfaccia JUCE usa una nuova gerarchia neutra color **grafite** con accenti **rame** per menu, campi, selezioni, cue e mixer. Superfici principali: `#121416`, pannelli `#24272A`, controlli `#34393C`; accento rame accessibile `#E4A77E`, testo caldo `#F2EDE5`. GO (verde), MIDI (ambra), OSC (viola tenue), PANIC ed errori (rosso) restano semanticamente distinti. Non cambiano editor, trasporto, routing, tempi, dimensioni o formato `.cuelist`; maggiori dettagli nel [changelog](CHANGELOG.md). La resa Windows/DPI richiede ancora collaudo visivo.

## 0.9.14 — Primo ciclo di rifinitura / tranche 1

Il GO resta operativo anche quando un MIDI Output non è configurato o viene scollegato; la strip segnala `No MIDI Output` senza popup bloccanti. Aggiunti marker di riordino delle Cuelist, cursore resize del Piano Roll MIDI, Undo/Redo iconici in entrambi gli editor, pulsante freccia di collasso del dock e opzioni indipendenti nel menu View per Clock, CPU, RAM e Timeline. Il router MIDI e il formato `.cuelist` non cambiano; le modifiche alla gestione simultanea di più Cuelist e alla cronologia globale sono rimandate dopo audit architetturale. Verifiche e limiti nel [changelog](CHANGELOG.md).

## 0.9.13 — Fix configurazione CMake

Rimosso il controllo di versione hardcoded che impediva la configurazione con la 0.9.12: la versione attesa viene ora derivata da `project(VERSION ...)`. Aggiunta verifica preventiva della corrispondenza dei file di versione. Nessun cambiamento funzionale all'interfaccia o ai motori. Vedi [CHANGELOG](CHANGELOG.md).

## 0.9.12 — Palette Forest/Teal

Il tema nativo JUCE usa i dodici campioni della palette approvata: sfondi verde scuro, accenti acquamarina e verde, testo avorio/bianco e avvisi terracotta. I colori di Cue Grid, Inspector, mixer, Timeline, editor e menu dipendono dagli stessi token condivisi in `src/Theme.h`. Restano invariati i flussi di lavoro, le assegnazioni delle cue e il formato del progetto. Controlli di contrasto aggiunti; collaudo visivo su Windows ancora richiesto. Dettagli e limitazioni: [CHANGELOG](CHANGELOG.md).

## 0.9.6 — Control Mappings e LTC Audio

Le celle con Control Map assegnata continuano a utilizzare il medesimo ControlPanel della sezione Control Mappings, con ID condiviso e selezione automatica. La generazione LTC da celle Audio vuote ora verifica un'unica regola di non sovrascrittura, gestisce in sicurezza il dialogo asincrono e supporta 29,97 fps sia drop-frame sia non-drop. I test coprono la progressione della vera forma d'onda. Nessuna modifica al timecode globale o al formato JSON v10.

## 0.9.5 — Toolbar e docking responsive

Le toolbar native MIDI e OSC condividono una geometria verificabile. Il MIDI dock compatto mantiene Undo/Redo, Detach/Reattach, Collapse/Close, Delete e Zoom senza sovrapposizioni; il docking OSC conserva i controlli esistenti e i suoi breakpoint. Nessuna modifica alle operazioni su media o motori. Richiesta verifica manuale sui monitor e dispositivi reali.

## 0.9.4 — Editor MIDI/OSC professionali

Il Piano Roll e l'editor OSC N7.1 riducono superfici arrotondate, colori decorativi e messaggi ridondanti; restano invariati i controlli e i dati. La verifica visuale sul PC Windows reale e i gate hardware/licenze rimangono necessari.

## 0.9.3 — Stabilizzazione SemVer e About

La patch corregge il test About che imponeva un suffisso `0.9.0-beta`, preservando l'intera interfaccia e tutti i controlli. Il formato del progetto rimane JSON v10 e la certificazione hardware non è ancora completata.

## 0.9.2 — Correzione CI

La prima patch successiva alla `0.9.1` corregge esclusivamente l'errore di compilazione del test Timeline N6.1, senza cambi al runtime o ai progetti. Restano le verifiche su hardware reale, licenze e DPI prima della distribuzione.

## 0.9.1 — Nuova baseline

La serie delle `0.9.0-beta` si conclude con `0.9.1`, che riunisce il redesign nativo di header, Cue Grid, Inspector, mixer e Timeline, oltre a editor MIDI/OSC, Control Mapping condiviso e generazione WAV LTC. Questo numero identifica l'applicazione: non crea automaticamente una GitHub Release o un pacchetto portable.

Restano da validare su Windows reale docking/editor MIDI/OSC, DPI e 32:9, routing hardware, decoder LTC esterno e condizioni di licenza JUCE. Consulta [stato della 0.9.1](docs/0.9.1-release-status.md), [checklist di rilascio](docs/0.9-release-checklist.md) e [changelog](CHANGELOG.md).

## UI V2 — Implementazione JUCE progressiva

**0.9.0-beta.14 — N3.2 Cue Grid:** disegno JUCE
nativo delle [intestazioni e della colonna Cue/Transport](docs/UI_V2_NATIVE_PHASE_N3_2.md)
con regole per tipo, gerarchia del testo, pulsanti
secondari discreti, tooltip con nomi completi.
Sono conservati tutti gli hit target di GO/Stop,
rinomina, spostamento, editor e collasso. Il Master
Audio originale non viene ridisegnato.

## UI V2 — Implementazione JUCE progressiva

**0.9.0-beta.13 — N3.1 Cue Grid:** iniziata l'integrazione
nativa della matrice con [geometrie condivise JUCE](docs/UI_V2_NATIVE_PHASE_N3_1.md)
per righe, colonne (anche collassate), overlay BPM/Auto Stop,
intestazioni fisse e scorrimento. Test su 13 cue, 11 colonne
e quattro risoluzioni. Le dimensioni e le hitbox originali
restano invariati finché non saranno aggiornati insieme
al rendering nella fase successiva. **Master Audio non ridisegnato**.

## UI V2 — Implementazione JUCE progressiva

**0.9.0-beta.12 — Audit menu N2.1:** tutti i comandi di
configurazione del precedente Settings sono di nuovo
direttamente raggiungibili dai menu JUCE appropriati
(Devices e View). Nessuna doppia voce Pre-flight/Diagnostics,
nessun duplicato MIDI/OSC Learn; conservate impostazioni
di porte MIDI/OSC, intervallo fader e orologio.
Vedi [inventario di parità funzionale](docs/UI_V2_NATIVE_MENU_AUDIT.md).
Il Master Audio originale resta escluso dal redesign.

## UI V2 — Implementazione JUCE progressiva

La **0.9.0-beta.11 (fase N2)** elimina dalla sidebar
i comandi duplicati New/Open/Save, Settings/Preferences
e Control Mapping, già disponibili tramite i menu JUCE
dell'header N1. Rimane un solo nome show editabile e
l'avviso di modifiche non salvate quando necessario.
Nessuna funzione viene rimossa: i callback sono gli
originali, con Settings ancorato al menu Show visibile.
Master Audio, GO/PANIC, cue list, mixer, audio/MIDI/OSC
e formato `.cuelist` restano invariati.
[Specifiche e test N2](docs/UI_V2_NATIVE_PHASE_N2.md).

## UI V2 — Implementazione JUCE progressiva

La **0.9.0-beta.10** inizia l'integrazione reale del redesign
con una barra di navigazione superiore JUCE (File, Edit,
Show, Cuelists, Devices, View, Guide) completamente collegata
ai callback già esistenti e con overflow accessibile a
finestre strette. La matrice, la sidebar operativa, il mixer
e il Master Audio rimangono quelli originali in questa fase.
Il Master è escluso definitivamente dal redesign.
[Specifiche e gate N1](docs/UI_V2_NATIVE_PHASE_N1.md).

## UI V2 — Operator workspace design study

**Vincolo Master Audio:** il Master della GUI JUCE originale NON viene ridisegnato. Fader/meter stereo, pan, mute, Main Out, limiter, gain reduction, Quick Learn, geometria e aspetto originali rimangono invariati. L'immagine del Master nella preview HTML è soltanto un riferimento statico, non un nuovo controllo o una specifica estetica.



**V2.3 / `0.9.0-beta.9`:** rifinito il prototipo di
[Operator Workspace](design/operator-workspace-v2.html):
strip Audio, MIDI, OSC e Control chiaramente differenziate,
testo più leggibile e sidebar senza note provvisorie.
La cue matrix, il Master completo e lo scroll condiviso
del mixer restano invariati. Tutte le funzioni operative
devono continuare a essere disponibili nella GUI nativa:
vedi [parità funzionale](docs/UI_V2_FEATURE_PARITY.md).
Il prototipo resta offline e non operativo.


**V2.2 / `0.9.0-beta.8`:** il Master nel prototipo di
[Operator Workspace](design/operator-workspace-v2.html)
rappresenta tutte le funzioni che il programma ha già in
JUCE (gain/meter stereo, pan, mute, Main Out, limiter,
gain reduction, Quick Learn); i comandi di configurazione
duplicati nella sidebar sono stati raccolti in un
unico menu superiore. **Nessuna funzionalità nativa viene
rimossa nel redesign**: vedi il
[gate di parità funzionale](docs/UI_V2_FEATURE_PARITY.md).
La preview è dimostrativa, non invia audio, MIDI o OSC.


La `0.9.0-beta.7` avvia uno studio di redesign **solo visuale** della
cue matrix, prendendo a riferimento un uso ricco (12 Cuelist, 13 cue,
11 colonne). La [specifica UI V2](docs/UI_V2_OPERATOR_REDESIGN.md)
descrive i vincoli del futuro intervento JUCE e l'[anteprima HTML
offline](design/operator-workspace-v2.html) permette di valutare
gerarchia, densità e collasso dei pannelli. La GUI di produzione,
l'esecuzione di cue e il formato show v10 sono **invariati**.
La preview non invia comandi audio, MIDI o OSC e non costituisce
una nuova build del programma. **V2.1:** dopo il primo feedback,
la parte superiore della demo rimane invariata e il mixer è
stato integrato sotto le stesse 11 colonne della cue matrix,
con scroll orizzontale condiviso, Master allineato alla zona cue
e BPM/Auto Stop ancora fissi. Vedere la
[specifica del mixer integrato](docs/UI_V2_INTEGRATED_MIXER_SPEC.md).

## Capabilities

| Area | Current functionality |
| --- | --- |
| **Cuelists** | Up to 12 named Cuelists within a show, independent cue sequences, tabs with rename/delete, overflow navigation, and independent VIEW/LIVE transport (tab browsing does not stop GO). |
| **Cue control** | Row and cell GO, Stop, Pause/Resume, Panic, overlapping playback, loop, inclusion in Row GO and per-row/per-cell Auto Stop. |
| **Audio** | WAV/MP3; independent audio columns, gain, pan, mute, phase inversion, output routing, Master Send, Direct Out with PRE/POST selection and an optional Master peak limiter. |
| **MIDI** | MIDI note-clip creation/import, built-in Piano Roll, named output selection, cue tempo mapping, editing and transport. |
| **OSC** | Timed UDP events, typed arguments and automation ramps with editable control points; per-column destination. |
| **Control Mapping** | MIDI Learn, global/column/preset/cue mappings and layered Control Sets with priorities, takeover modes, curves and selected audio/MIDI/OSC targets. |
| **Workspace** | Zoomable Timeline; fixed BPM/Auto Stop; collapse/expand of the Inspector and columns; cue-specific notepad; configurable editor dock; Unified Undo/Redo for supported Show and MIDI/OSC edits. |
| **Operations** | Preflight, Diagnostics, crash-recovery checkpoints, transactional project loading, offline tests and an optional hardware inventory utility. |

### Typography

The native interface uses **Exo 2** for compact operational labels (no smaller than **10 pt**) and **Hind** for larger type, headings and titles. **Noto Sans Math** is selected explicitly for mathematical symbols, including the `−∞` fader readout and the **Preferences → Fader Range** menu; **Noto Sans Symbols 2** is selected explicitly for row/column arrows, cue/cell Play–Pause–Stop, the ⋯ actions menu, compact information/caution icons, and the two-state speaker glyphs **🔊 / 🔇** on Master/column/data-output Mute buttons (where supported by the embedded font, with vector alternatives). These Noto fonts are **not general-purpose text fallback fonts**. Operational glyphs such as Delete use vector geometry, and text remains Exo 2/Hind. All four typefaces are embedded from pinned Google Fonts sources under SIL OFL 1.1, independent of fonts installed on the operator's Windows computer. See [NOTICE.md](NOTICE.md) for copyrights and licenses, and the [Unicode symbol audit](docs/unicode-symbol-audit.md) for every supported UI symbol and the deliberate vector exclusions.

### Icon-button visual language

Compact icon controls now share a subdued **Graphite/Copper** surface: neutral low-contrast outlines at rest, slightly brighter on hover, light semantic tint when toggled, muted disabled ink and a distinct keyboard-focus indicator. The symbol remains the visual priority; Play, Stop, Mute, Snap, row/column controls, Inspector grip and control lock retain their operational meanings, tooltip text and hitboxes. Dedicated **GO / PANIC** and ordinary labelled buttons deliberately keep their own treatment. Noto Sans Symbols 2 and Noto Sans Math remain *explicitly selected* for their corresponding glyphs, not font fallbacks. See [Unicode symbol audit](docs/unicode-symbol-audit.md).

### Abstract Control Mapping and Default Map

The controller system supports named project functions with stable IDs and
independent routing: **physical MIDI/OSC input → function → output target**.
A global **Default Map** assigns hardware inputs; a cue's Control Column cell
may override a function's *input* without modifying its destination (for
example Resolume brightness remains MIDI CC 74 on its configured channel).
Overrides do not carry forward between cues unless copied into their Control
Sets. The effective preview identifies inherited vs local assignments.

Global **M Learn / O Learn** controls sit beside the top-right activity
monitors; the Control Set editor provides its own scoped MIDI/OSC Learn.
The OSC receiver starts only on a configured UDP port, accepts bounded
numeric/boolean OSC 1.0 input and updates genuine OSC RX activity. It is
**unauthenticated**: use on a trusted LAN with appropriate firewall rules.
Name-free controller routing presets can be exported and imported without
overwriting project-specific function labels or unrelated local Control Sets.
The old per-column Control Lock is removed from the operator interface.
On project activation, its global bindings migrate transactionally into
named Default Map functions; dormant cue-specific sets are archived as
reusable Control Sets rather than activated unexpectedly. Review and save
the migrated project explicitly: the original file is not overwritten
automatically. Legacy fields remain decoder-compatible, and a separate
global edit lock is reserved for a later explicitly defined policy.

### The workspace

A shared dark editor component is used for cue/column/Cuelist renames, BPM and multiline tempo maps. Control Mapping uses labelled sections, contextual MIDI/OSC target fields and an explicit empty-list state. The left sidebar contains New, Open, Save, Audio devices, Preferences, Control Mapping, the next-cue indicator and GO/Panic. The upper bar displays the current Cuelist tabs, Undo/Redo, a 24-hour clock, CPU/RAM telemetry and MIDI/OSC activity indicators.

The cue grid keeps **Cuelist and Audio/MIDI/OSC/Control column headings pinned while scrolling vertically**. Column headings follow the same horizontal scroll position as their cells, including collapse/reorder controls. BPM and Auto Stop remain fixed at the right. The vertical cue scrollbar now appears on the **left edge** of the Cue List when needed, so the final cells meet the BPM / Auto Stop divider without a dark 9px gutter. Short Cuelists reserve no unnecessary scrollbar space. The Cue List / BPM divider remains continuous across the scrolling grid and mixer. Row values and switches are clipped beneath fixed headers while scrolling vertically, and the row's Play/Pause, Stop and menu actions have separate hit targets. Cell Play/Stop buttons likewise use distinct, larger targets. The row-level Auto Stop switch has a separate **CELLS +/−** disclosure for cell-specific stop settings. **Double-click a Cuelist tab or the Cuelist name in the cue-grid header to rename it**; both always show the same saved name. The `×` action deletes a Cuelist after confirmation, but the last remaining Cuelist cannot be deleted. A separate overflow menu appears only when tabs no longer fit. The Inspector can collapse into a narrow right-side tab and reopen without losing committed edits. Cue notes are saved with their cue.

Undo/Redo share a single bounded history for supported Show operations, clip and row edits, Master/mixer adjustments and MIDI/OSC gestures. Runtime GO, Stop, PANIC and VIEW-only tab changes are never undoable.

## Getting started

1. Obtain a compiled Windows x64 build or build from source using the instructions below.
2. Launch **The Cuelist.exe** (or `Avvia.cmd` from a properly extracted package).
3. Open **Audio devices**, select the intended device and confirm its output configuration.
4. Import a WAV/MP3 clip into an Audio cell, or add MIDI/OSC columns and configure their outputs.
5. Use **GO** (Space) to trigger the next cue, and **Panic** (Esc) to stop playback. Test both before connecting to production equipment.
6. Save the show to a `.cuelist` file; keep its accompanying `_media` folder when transferring the show.

For detailed operation, device behavior, MIDI/OSC editors and the project format, read the **[Complete User Guide](docs/complete-user-guide.md)**.

## Build from source

**Requirements:** Windows x64, Visual Studio 2022 / Build Tools with *Desktop development with C++*, Windows SDK and CMake tools. Git is used when the pinned JUCE dependency is not yet present in `vendor/JUCE`. The build script retrieves **JUCE 8.0.12** if required.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1
```

Or run `Compila.cmd`. The Release executable is written to:

```text
build/TheCuelist_artefacts/Release/The Cuelist.exe
```

The script configures CMake, compiles the application and test targets, then runs CTest. Use `scripts/build.ps1 -TestOnly` for the test binary only, `-Run` to launch after a successful build, and `--offline` when manually starting the executable without opening an audio device.

### Portable Windows package

Package creation is a **separate, explicit release operation**, not part of a standard local build or pull-request CI check:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/package.ps1 -SkipBuild
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/check-package.ps1 -SmokeLaunch
```

The `Windows CI (portable package on demand)` workflow can also be started manually from GitHub Actions. The ordinary `Windows Release and tests` workflow only builds and tests. No GitHub Release is implied by the presence of a source branch or passing CI.

The ZIP contains the executable, launcher, README, **one application `LICENSE`**, `NOTICE.md` and the separate **JUCE third-party license notice**. Packaging does **not** establish that the rights holder has secured a commercial JUCE distribution license.

## Projects, backups and recovery

Projects use JSON schema **v10**, independent of the application's `versione indicata in [VERSION.txt](VERSION.txt)` number. The loader retains compatibility with earlier project versions. Cuelist names, per-cue notes and additional mixer/control data are saved as optional v10 fields.

Save collects referenced audio into the project's sibling `<show-name>_media` directory; keep the project and media together. Overwrites preserve a backup. Project opening prepares and validates media before replacing the running show. Recovery checkpoints periodically protect applied unsaved model changes, but **are not a substitute for manual saves** and may not contain text still being edited.

See the [release acceptance checklist](docs/0.9-release-checklist.md) and [stability and timing notes](docs/stability-and-timing.md) for test scope and remaining limitations.

## Development status and known limitations

- This is **versione indicata in [VERSION.txt](VERSION.txt)**, not a stable release or certified live-show system.
- MIDI/OSC dispatch counters describe local software activity, **not confirmed delivery** or end-to-end latency. OSC UDP offers no acknowledgement by default.
- Hardware MIDI controller feedback, native video playback, LTC and MTC are not implemented.
- File import and project save can still involve blocking work; long-duration, real-device stress tests remain necessary.
- Control Mapping should be rehearsed carefully with the actual controller, especially Pickup/Jump, connection changes and layered priorities.
- JUCE's licensing terms apply independently of the application's proprietary license.

## License

The Cuelist is **proprietary software; all rights reserved**. The single [LICENSE](LICENSE) contains a conventional copyright reservation rather than an improvised end-user licence agreement. No public licence to reproduce, modify, build derivatives from or redistribute the original source is granted by publishing this repository. Any already-granted individual licence agreement remains governed by its own terms. A future commercial end-user licence must be prepared separately and professionally reviewed.

GitHub confirms that **without an open-source licence**, copyright remains with the rights holder; public repository visibility nevertheless permits other users to view and fork through GitHub's functionality. See [GitHub's licensing guidance](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/licensing-a-repository) and [GitHub's Terms of Service](https://docs.github.com/en/site-policy/github-terms/github-terms-of-service). To restrict *future* access to the source, the owner should consider making the repository private. Existing forks and local copies may remain accessible.

Third-party rights, particularly JUCE, remain subject to [NOTICE.md](NOTICE.md) and the respective vendor licences. Commercial binary distribution requires verification of those separate rights.

## Research & Development Lab

[Research & Development Lab](docs/research/README.md) organizza benchmark di software concorrenti, protocolli Audio/MIDI/OSC/LTC, valutazioni UX di operatori, prove riproducibili e decisioni tecniche. È **documentazione di ricerca**, non una certificazione per show dal vivo né una modifica al comportamento dell'app.
