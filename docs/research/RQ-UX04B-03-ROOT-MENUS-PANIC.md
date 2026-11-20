# RQ-UX04B-03 — First Esc nel livello principale Devices/View/More

**Data:** 2026-11-20 · **Base:** `main@5539d885e74b244a7fe8f54f0b78948b16a316e7` (v0.9.91) · **Release proposta:** v0.9.92 · **PR [#223](https://github.com/Askeladd22/The_Cuelist/pull/223): draft** · **Schema:** JSON .cuelist v10 invariato.

## Scenario / rischio operativo

Durante una scena LIVE l'operatore apre il menu superiore Devices, View o More (quest'ultimo su layout compatti). Un primo Esc inteso come PANIC viene intercettato da `juce::PopupMenu` invece di raggiungere il trasporto. Il menu puo' scomparire, ma audio/MIDI/OSC/LTC possono ancora proseguire. Il problema e' *ipotizzato per queste tre categorie* dalla sorgente upstream e va confermato con RED reale. Le altre cinque categorie superiori sono gia' protette dalla v0.9.91.

**Rischio:** safety UX alto se il primo Esc e' interpretato come arresto globale; il software non deve generare un falso PANIC su un click fuori o un menu chiuso normalmente. Gli output fisici non sono misurati dal test di peer JUCE.

## Fonti verificate, confronto e ipotesi

- Applicazione: `src/MainComponent.cpp::showOperatorMenu` crea tutti gli otto menu superiori via `juce::PopupMenu::showMenuAsync`. Solo gli indici 0,1,2,3,6 si registrano al `FlatOperatorPopupEscapePanic` su main v0.9.91.
- `src/MainComponent.h::FlatOperatorPopupEscapePanic`: un KeyListener con `SafePointer`, rimozione esplicita nel distruttore MainComponent, ritorna `false` per mantenere la chiusura Escape di JUCE.
- `src/OperatorNavigation.h::OperatorNavigationLayout`: `Devices` indice 4, `View` indice 5, `More` indice 7; `More` usa il layout responsive.
- [JUCE 8.0.12 PopupMenu.cpp](https://github.com/juce-framework/JUCE/blob/8.0.12/modules/juce_gui_basics/menus/juce_PopupMenu.cpp) righe 612–669: `MenuWindow::keyPressed` consuma Escape, `dismissMenu(nullptr)`, ritorna true. Righe 2210–2239: creazione ed entrata modal state sincrone; callback asincrono.
- [JUCE 8.0.12 ComponentPeer.cpp](https://github.com/juce-framework/JUCE/blob/8.0.12/modules/juce_gui_basics/windows/juce_ComponentPeer.cpp) righe 196–220: il target invoca prima i `KeyListener`, poi il proprio `keyPressed`.
- [Issue #191](https://github.com/Askeladd22/The_Cuelist/issues/191) e [PR #222](https://github.com/Askeladd22/The_Cuelist/pull/222), v0.9.91: test JUCE peer e bug gia' corretto nelle categorie piatte. Fonti del framework **non sono test end-to-end di Windows** e i comportamenti di altri prodotti show control non sostituiscono questo contratto.

**H1 (falsificabile):** su main v0.9.91, Esc inviato dal *vero peer nativo* dei popup Devices/View/More e' gestito ma non richiama `stopAll()`. Il test **RED** deve riportare esattamente questo, non un errore di compilazione o assenza della finestra.

**H2:** estendere il predicato che abilita l'unico `FlatOperatorPopupEscapePanic` a tutti i menu superiori non legacy elimina il problema **solo al root** senza aggiungere stato o toccare engine, shortcut, look-and-feel o callbacks.

**Controesempio importante:** per un **sottomenu** viene creato un *secondo* `MenuWindow` con proprio peer. La sola correzione del popup root non dimostra inoltro di Esc da un figlio. Non attribuire copertura a Devices/View/More sottomenu o legacy Settings e More nested.

## Acceptance / QA / Unlazy ledger

| Gate | Criterio | Evidenza |
|---|---|---|
| AC-1 RED | Il primo Esc simulato tramite peer sul livello root Devices/View/More non fa PANIC con codice vecchio | `8320875e`, Windows run da verificare |
| AC-2 GREEN | Il medesimo test fa PANIC per tutte e tre le categorie (Devices/View a 1920, More a 1180), con `encodeShow`, LIVE/VIEW, NEXT invariati | Windows CTest commit codice-fix |
| AC-3 regression | I cinque menu preesistenti, la chiusura click/non-Esc e il riarmo File rimangono invariati | test UX04B-01, UX04B-02 e UX04B-03 |
| AC-4 engineering | Un solo `KeyListener`, `SafePointer` per menu, detach al shutdown, nessun hook globale o modifica di threads real-time | Review manuale C++ e CI |
| AC-5 release | VERSION.txt / CMake project VERSION 0.9.92 / CHANGELOG; CodeQL / Windows CTest sull'ultimo SHA | GitHub Actions |

**Rischi red-team:** il menu potrebbe non aver ottenuto il focus sul peer scelto; il test che chiama `MainComponent::keyPressed(Esc)` direttamente e' un falso positivo; la cancellazione per click/focus non deve triggerare PANIC; durante apertura/cancellazione/riapertura non deve rimanere un `KeyListener` appeso alla vecchia finestra. Non sostituire componenti JUCE con HTML, non usare hook Windows globali; verificare anche mancata selezione di voci e nessuna migrazione del file.

### Metodi usati

PAUL (PLAN→APPLY→UNIFY), SEED (app deep quality gate), Trail of Bits C++ Review (review manuale locale di ownership, validita' callback, false positive), Matt Pocock (test-first RED/GREEN), Simplify Codebase (riuso del listener, nessuna astrazione duplicata), CCteam QA (test di regressione reale peer JUCE + gate hardware distinti), Unlazy (evidenze PASS/FAIL/ND). Nessuno di questi adattamenti skill esegue autonomamente parser Trail of Bits, test fisici o agenti invisibili.

## Residual release gates — non certificati

| Gate | Stato |
|---|---|
| Test tastiera fisica Win11, reale focus/modal su dispositivi non di produzione | **NOT RUN** |
| Esc da sottomenu Devices/View/More, popup contestuali e dialoghi | **NOT COVERED** |
| UI DPI 100%,125%,150%, accessibilita' Narrator e IME | **NOT RUN** |
| Misura con oscilloscopio/loopback e dispositivi reali Audio/MIDI/OSC/LTC | **NOT RUN** |
| Test Windows/CTest CodeQL sul PR head esatto | **ND**, da aggiornare dopo i risultati |

**Rollback:** revert della sola PR #223 su feature branch; no merge automatico. Nessun cambiamento di schema JSON v10 o protocolli di output.

**Raccomandazione provvisoria:** integrare solo dopo RED effettivo e GREEN sullo SHA finale. Sviluppare separatamente la copertura dei *child menu* (issue #191) con focus e keyboard delivery veri; non dichiarare mai ready-for-LIVE usando solo CI.
