# RQ-UX04B-01 — PANIC sul primo Escape del menu File nativo JUCE

**Data** 2026-11-20 · **Base** `main@621bbedb9974aa08735bb6435d5959e4581ed795`, app v0.9.89 · **Target** v0.9.90 · **PR** [#221](https://github.com/Askeladd22/The_Cuelist/pull/221) (**merged** in main@e19901514899a960735f6e5c400bacf16ef84159) · **Formato** .cuelist JSON v10 invariato.

## Scenario operativo e impatto

Un operatore con una scena LIVE apre il menu File nativo per salvare/aprire. Durante un'emergenza preme Escape per richiedere PANIC. Il menu `juce::PopupMenu` possiede il focus e tratta Escape come chiusura: l'operatore puo' interpretare la chiusura del menu come arresto quando invece il trasporto continua. Rischio **ALTO** in caso di emergenza, ma **nessuna misura fisica di ritardo o mancato PANIC e' ancora disponibile**. L'indicatore di test `engine.acknowledgedCueId` prova il percorso software, non lo spegnimento del dispositivo remoto.

## Fonti primarie e comportamento corrente

- Codice applicazione: `src/MainComponent.cpp::showOperatorMenu` (`File` indice 0), `MainComponent::keyPressed`, `MainComponent::stopAll`; `tests/UiRefinementTests.h`; `src/MainComponent.h` (ownership); `src/OperatorNavigation.h`. Le correzioni degli inline TextEditor e del progetto Label sono gia' integrate e non vanno duplicate.
- [Sorgente ufficiale esatto JUCE 8.0.12 PopupMenu](https://github.com/juce-framework/JUCE/blob/8.0.12/modules/juce_gui_basics/menus/juce_PopupMenu.cpp): `MenuWindow::keyPressed` alle righe 612–669 gestisce Escape con `dismissMenu(nullptr)`, poi ritorna `true`, senza chiamare root `MainComponent`.
- [Sorgente JUCE 8.0.12 ComponentPeer](https://github.com/juce-framework/JUCE/blob/8.0.12/modules/juce_gui_basics/windows/juce_ComponentPeer.cpp): `handleKeyPress` alle righe 196–220 consulta i `KeyListener` del target prima della relativa `Component::keyPressed`. Questa e' una via pubblica di **simulazione peer**, non una conferma di evento fisico OS.
- [Sorgente JUCE 8.0.12 DialogWindow](https://github.com/juce-framework/JUCE/blob/8.0.12/modules/juce_gui_basics/windows/juce_DialogWindow.cpp) righe 49–65: anche le finestre modali hanno gestione Escape propria; fuori scope.
- [Issue #191](https://github.com/Askeladd22/The_Cuelist/issues/191), [documentazione JUCE PopupMenu](https://docs.juce.com/master/classjuce_1_1PopupMenu.html). Distinguere specifica API da comportamento concreto della versione pin, da discussioni del forum e da test Windows eseguiti.

## Piano PAUL / gate SEED Application Deep

**Ipotesi H1 (falsificabile):** il primo `ComponentPeer::handleKeyPress(Escape)` diretto al vero `PopupMenu` File della UI attuale chiude il menu **senza** resettare `engine.acknowledgedCueId`. Un test RED nativo deve riprodurlo; se fallisce per assenza peer/modal, e' un blocco di strumentazione, non la prova del difetto.

**Ipotesi H2:** un `juce::KeyListener` applicato al solo `MenuWindow` File, con lifetime controllata e inoltro `stopAll()`, puo' ottenere PANIC al primo Escape e lasciare alla Window la normale chiusura, senza spostare il focus, cambiare callback delle voci File o attaccare hook Windows globali.

| ID | Criterio di accettazione | Prova / gate |
|---|---|---|
| AC-1 | Escape tramite vero peer del menu File chiama il PANIC app al primo evento, non soltanto la chiusura | Test CTest reale, prima RED poi GREEN |
| AC-2 | Il test conserva `encodeShow`, `liveCuelistIndex` e `nextCueIndex` | Test medesimo, supporto alle invarianti JSON v10 |
| AC-3 | Menu invocato dal pulsante File reale, look-and-feel e scelta voci invariati; hook svincolato quando menu/owner scompare | code review lifetime, stress apertura/chiusura, CTest |
| AC-4 | VERSION/CMake/CHANGELOG v0.9.90, branch/PR senza merge automatico | GitHub Actions version guard |
| AC-5 | Windows Release/CTest e CodeQL sullo SHA di codice e poi sullo SHA finale, con alert analizzati a parte | GitHub Actions |

**Scope:** solo **menu File di primo livello** (nessun sottomenu); il sistema `PopupMenu` e l'engine non vengono alterati. **Non-obiettivi:** menu Devices, View, Menu overflow, popup cella/riga/Control Map/Timeline/OSC, AlertWindow/DialogWindow, script tastiera OS, certificazione LIVE. Questi restano nella issue #191 per slice successive.

## Red team / qualità

- **Potenziali falsi positivi:** un `peer` assente in CTest, focus su altro peer, test che chiama root Esc invece del menu, menu che si chiude solo su click esterno, stato acknowledged impostato artificialmente senza riproduzione fisica.
- **Concorrenza e ownership:** nessun richiamo a `engine.panic()` dal callback audio; il listener del menu deve esistere quanto il suo riferimento registrato. `SafePointer` e detach durante distruzione, senza dangling callback su `MainComponent` eliminato, senza modificare routing esterno. Il listener restituisce `false` dopo l'inoltro per conservare la chiusura Escape nativa del `MenuWindow`.
- **Alternative scartate:** hook tastiera globale Windows (privilegi e interazione con altri software), inoltro di PANIC per qualsiasi callback menu `choice=0` (un click fuori/Cancel non deve attivare PANIC), riscrittura di tutti i popup o del sistema di input.
- **Semplificazione:** nessuna infrastruttura di cattura globale, nessun nuovo editor sostitutivo. Preferire un unico hook di vita breve con contratto rigoroso.
- **QA:** test peer **simulato** e funzionamento fisico OS e periferiche **separati**. Un job CI verde non implica assenza di alert CodeQL e non prova consegna downstream.

## Evidenze e ledger

| Gate | Riferimento | Esito |
|---|---|---|
| TDD RED test-only `abb209e4` | [Windows CI 44772996671](https://github.com/Askeladd22/The_Cuelist/actions/runs/44772996671) | **FAIL atteso:** 421.018 asserzioni, 1 test fallito: primo Esc chiude File senza PANIC |
| Guard versioni test-only | [run 44772996626](https://github.com/Askeladd22/The_Cuelist/actions/runs/44772996626) | **PASS** |
| CodeQL test-only | [run 44772996539](https://github.com/Askeladd22/The_Cuelist/actions/runs/44772996539) | **CANCELLED** dopo aggiornamenti successivi, non PASS |
| GREEN source + exact-head checks | PR head `f2ffb739`: [Windows CI 44773805428](https://github.com/Askeladd22/The_Cuelist/actions/runs/44773805428), [Version guard 44773805242](https://github.com/Askeladd22/The_Cuelist/actions/runs/44773805242), [CodeQL 44773805376](https://github.com/Askeladd22/The_Cuelist/actions/runs/44773805376) | **Windows/CTest PASS:** 421.196 asserzioni, 0 errori; **guard PASS**; CodeQL in verifica al controllo |
| Windows reale con tasto Esc fisico, focus e menu aperto | UAT con show fittizio, output disconnessi/sicuri | NOT RUN |
| Windows DPI 100/125/150, Narrator e IME | Operatore di test | NOT RUN |
| Audio/MIDI/OSC/LTC su banco hardware isolato e misure latenza | UAT fisica separata | NOT RUN |

**Rollback:** revert della singola PR #221; versione riallineata al successivo main e nessuna migrazione .cuelist.

**Stato:** PLAN/APPLY/UNIFY della PR #221 completati per il codice e CTest; PR #221 successivamente mergiata in main. CodeQL finale verificato separatamente; hardware e focus fisico restano gate aperti in issue #191. Non dichiarare ready-for-LIVE.
