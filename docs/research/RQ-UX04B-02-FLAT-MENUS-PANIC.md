# RQ-UX04B-02 — Esc/PANIC nel menu Edit/Show/Cuelists/Guide

**Baseline verificata:** `main@e19901514899a960735f6e5c400bacf16ef84159`, v0.9.90 · **Target:** v0.9.91 · **PR:** [#222 (merged)](https://github.com/Askeladd22/The_Cuelist/pull/222), `main@5539d885e74b244a7fe8f54f0b78948b16a316e7` · **Schema:** `.cuelist` JSON v10 invariato.

## Scenario operativo

Un operatore apre un menu superiore con voci di editing/gestione durante LIVE, poi preme Escape per interrompere immediatamente lo show. In JUCE 8.0.12, `MenuWindow::keyPressed(Escape)` consuma l'evento e chiude il menu, percio' la pressione puo' non raggiungere `MainComponent::keyPressed` / `stopAll`. La PR #221 / v0.9.90 protegge solamente File; il rischio e' identico nella classe di popup semplice Edit, Show, Cuelists e Guide, ma la conferma RED deve provenire dal test native peer e non da una deduzione.

**Operator risk:** alto se la scorciatoia Esc viene intesa come stop immediato; falsi positivi possibili se il popup si chiude ma output continuano. Non aggiungere una seconda via di PANIC su normale cancellazione click/focus. Non modificare comportamenti reali di GO, Undo/Redo, LIVE/VIEW.

## Fonti verificabili / confronto / ipotesi

- Codice letto su baseline: `src/MainComponent.cpp` (costruzione e `showOperatorMenu`, `stopAll`); `src/MainComponent.h` (`FilePopupEscapePanic` esistente e `SafePointer`); `src/OperatorNavigation.h` (indici e categoria).
- [JUCE 8.0.12 PopupMenu.cpp](https://github.com/juce-framework/JUCE/blob/8.0.12/modules/juce_gui_basics/menus/juce_PopupMenu.cpp) righe 612–669: `MenuWindow` consuma Escape; righe 2210–2239: il menu passa a modal state sincronicamente, mentre la scelta arriva in callback asincrona.
- [JUCE 8.0.12 ComponentPeer.cpp](https://github.com/juce-framework/JUCE/blob/8.0.12/modules/juce_gui_basics/windows/juce_ComponentPeer.cpp) righe 196–220: i KeyListener del target vengono interrogati prima di `Component::keyPressed`.
- [Issue #191](https://github.com/Askeladd22/The_Cuelist/issues/191), [PR #221](https://github.com/Askeladd22/The_Cuelist/pull/221) con RED/ GREEN per File. I sistemi QLab/concorrenti non sono trasferibili qui come prova; il contratto deve essere quello delle scorciatoie The CUElist.
- **H1 falsificabile:** `Edit`, `Show`, `Cuelists`, `Guide` senza listener falliscono al primo Esc: evento ricevuto dal peer, PANIC software non raggiunto. **H2:** riutilizzare lo stesso listener RAII (rinominato genericamente) mantiene i 5 menu semplici, senza infrastruttura di input globale.

## Controlli AC e implementazione reversibile

| AC | Criterio | Misura / prova |
|---|---|---|
| AC-1 | RED: almeno un menu nativo semplice non forwarda PANIC sul primo Esc | **RED misurato:** test-only SHA `33b8b4c9`; [Windows run #44775466385](https://github.com/Askeladd22/The_Cuelist/actions/runs/44775466385), 421.024 asserzioni / 1 FAIL atteso (`top-level operator menu consumed Escape without PANIC`) |
| AC-2 | GREEN: File/Edit/Show/Cuelists/Guide forwardano primo Esc via **vero peer** | **GREEN misurato sul codice:** SHA `a2381474`, [Windows run #44775956715](https://github.com/Askeladd22/The_Cuelist/actions/runs/44775956715), UX04B-01 e UX04B-02 PASS, 421.220 asserzioni / 0 FAIL |
| AC-3 | Click fuori/cancel non sono PANIC, riapertura File riattacca listener senza puntatore obsoleto | **PASS** su `019cd87c`: [Windows run 44776093554](https://github.com/Askeladd22/The_Cuelist/actions/runs/44776093554), 421.227 asserzioni / 0 FAIL, entrambe le UX04B-02 PASS |
| AC-4 | LIVE/VIEW, NEXT, snapshot JSON v10 immutati dopo Escape; menu normali riutilizzano i comandi originali | Test e revisione patch mirata |
| AC-5 | Version guard VERSION.txt/CMake 0.9.91, CHANGELOG; CI/CodeQL verificati sull'ultimo SHA | GitHub Actions, ND sino a run conclusi |

**Codice modificato:** `src/MainComponent.h` rinomina la singola `FilePopupEscapePanic` in `FlatOperatorPopupEscapePanic`; `src/MainComponent.cpp` conserva il solo owner callback (stopAll) e il detach in distruzione, includendo indici 0,1,2,3,6. `tests/UiRefinementTests.h` usa reali bottoni e peer JUCE; la seconda regressione verifica cancellazione non-PANIC e riapertura. Nessun nuovo listener per menu, nessun polling o hook globale, nessuna chiamata nel callback audio.

## Red team / review C++ / QA

- **Ownership:** un solo `KeyListener` per `MainComponent`, menu identificato con `SafePointer`, `detach` rimuove il listener prima che `MainComponent` venga distrutto. Il popup nativo possiede il suo ciclo di vita e puo' chiudersi prima dell'owner. Un pointer non sicuro a un `MenuWindow` (o callback `[this]` sopravvissuto) causerebbe UAF; revisionare apertura/chiusura ripetuta.
- **Invocazione:** `getCurrentlyModalComponent` deve identificare il popup *nuovamente creato*, non una dialog sovrapposta; confrontare con `previousModal`. Un test che invochi `MainComponent::keyPressed(Escape)` e' un falso positivo, bypassa il focus owner.
- **UX:** Esc PANIC deve conservare anche la normale dismissione del menu (listener restituisce false e `MenuWindow` gestisce Esc). Una dismissione ordinaria `dismissAllActiveMenus` non deve fermare lo show.
- **Falsificazioni:** il test JUCE peer simula key dispatch ma non genera evento fisico Win32; impossibile attribuire latenze reali o certificazione di output downstream.
- **Ambiti esclusi:** `Devices`, `View`, `More` hanno sottomenu e focus variabile; legacy Preferences, editor MIDI/OSC, righe/celle, control-map, Timeline, `AlertWindow`, `DialogWindow` e input OS/hardware da analizzare in slice indipendenti. Non chiudere issue #191.

## PAUL / SEED / Skill adapters / stato

**PLAN**: definito scenario, ac, hypothesi, rischi; **APPLY**: test-first e patch breve; **UNIFY**: aggiornare questo ledger e PR solo dopo run con SHA esatto. Gli adattamenti PAUL, SEED, Trail of Bits manual C++ Review, Matt Pocock TDD, Simplify focused survey, CCteam QA e Unlazy sono metodi; non dichiarano scan automatico, test fisico o subagenti.

| Gate | SHA/run | Risultato |
|---|---|---|
| Windows test-only RED | `33b8b4c9` / [44775466385](https://github.com/Askeladd22/The_Cuelist/actions/runs/44775466385) | **FAIL atteso:** 421.024 asserzioni / 1 FAIL mirato |
| Windows Release/CTest source | `a2381474` / [44775956715](https://github.com/Askeladd22/The_Cuelist/actions/runs/44775956715); `019cd87c` / [44776093554](https://github.com/Askeladd22/The_Cuelist/actions/runs/44776093554) | **PASS** per 5 menu: 421.220 asserzioni / 0 FAIL; anche negativa cancellazione/riapertura **PASS**: 421.227 asserzioni / 0 FAIL |
| Final PR head `fa1889b9`: Windows [44777424844](https://github.com/Askeladd22/The_Cuelist/actions/runs/44777424844), CodeQL [44777424749](https://github.com/Askeladd22/The_Cuelist/actions/runs/44777424749), version guard [44777424676](https://github.com/Askeladd22/The_Cuelist/actions/runs/44777424676) | **Windows / CodeQL / guard PASS**; benchmark fisico ancora NOT RUN |
| Tastiera fisica + menu e sottomenu con stato LIVE | PC Windows / show di prova | NON ESEGUITO |
| DPI 100/125/150, Narrator, IME | Test fisici Windows | NON ESEGUITO |
| Audio/MIDI/OSC/LTC esterni, latenza PANIC misurata | Banco isolato e hardware reale | NON ESEGUITO |

**Rollback:** revert della sola [PR #222](https://github.com/Askeladd22/The_Cuelist/pull/222), lasciare progetto JSON v10 invariato; nessun merge automatico.

