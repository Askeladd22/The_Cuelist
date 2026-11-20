# RQ-UX04B-04 — Primo Esc nel sottomenu nativo More

**2026-11-20** · Baseline verificata `main@507e796d13ae97bf4a4daa01edcda6b0b50a923e`, applicazione **0.9.92** · Target **0.9.93** · [PR #224](https://github.com/Askeladd22/The_Cuelist/pull/224) draft · formato **.cuelist JSON v10** invariato.

## Scenario / conseguenza per l'operatore

Su un workspace 1180×820, l'operatore apre il pulsante **More** mentre un altro cue e' potenzialmente LIVE. Seleziona con Down la prima categoria raggruppata e apre il sottomenu con Right; durante un'emergenza preme Escape. Il primo Esc potrebbe chiudere il sottomenu senza inoltrare PANIC. **Rischio operativo ALTO**: l'aspetto di un menu chiuso non dimostra che audio/MIDI/OSC/LTC siano fermati. Non svolgere fault injection su uno show o output reali.

## Fonti primarie effettivamente lette

- Progetto: `src/MainComponent.cpp::showOperatorMenu`, `src/MainComponent.h::RootOperatorPopupEscapePanic`, `src/OperatorNavigation.h::OperatorNavigationLayout`, `src/Theme.h` e `tests/UiRefinementTests.h` sul SHA baseline.
- [JUCE 8.0.12 PopupMenu.cpp](https://github.com/juce-framework/JUCE/blob/8.0.12/modules/juce_gui_basics/menus/juce_PopupMenu.cpp): `showSubMenuFor` righe 1256–1272 crea `MenuWindow` figlio e chiama `enterModalState(false)`; `MenuWindow::keyPressed` righe 612–669 consuma Escape e chiude; nel costruttore del menu riga ~460 il `LookAndFeel` riceve `preparePopupMenuWindow(*this)`.
- [JUCE 8.0.12 ComponentPeer.cpp](https://github.com/juce-framework/JUCE/blob/8.0.12/modules/juce_gui_basics/windows/juce_ComponentPeer.cpp) righe 196–220 interroga i KeyListener del target prima del proprio key handler.
- [JUCE 8.0.12 PopupMenu.h](https://github.com/juce-framework/JUCE/blob/8.0.12/modules/juce_gui_basics/menus/juce_PopupMenu.h) riga ~1031 espone `LookAndFeelMethods::preparePopupMenuWindow`; [LookAndFeel_V2.cpp](https://github.com/juce-framework/JUCE/blob/8.0.12/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel_V2.cpp) ne offre implementazione base vuota. Distinguerne lo scopo dal contratto delle scorciatoie dell'app.
- [Issue #191](https://github.com/Askeladd22/The_Cuelist/issues/191) e [PR #223](https://github.com/Askeladd22/The_Cuelist/pull/223) con root menu RED→GREEN. QLab e prodotti concorrenti non dimostrano il comportamento interno JUCE.

## Ipotesi misurabili / controesempi

**H1 RED:** la pressione di Escape nel vero peer del sottomenu More, raggiunto con un percorso operativo reale (click del bottone More, Down, Right), genera `handleKeyPress` gestito ma `engine.acknowledgedCueId` resta a 923. La mancata creazione del popup o l'assenza peer va classificata come **blocco test**, non come bug confermato.

**H2 GREEN:** si puo' registrare l'unico KeyListener gia' esistente anche sui `MenuWindow` figli usando l'hook di preparazione JUCE, **solo quando il modal owner gia' tracciato e' un popup operatore**. Non usare hook OS globali, polling (rischio di perdere il primo Esc), timer, nuovo stato GO o callback audio.

**Controesempi:** un popup contestuale si apre mentre il menu principale e' aperto: non deve ereditare la scorciatoia senza relazione con il menu operatore. Un figlio chiuso con Left lascia il parent attivo: il parent deve ancora intercettare Esc. La distruzione di un child puo' precedere la distruzione del parent o dell'owner: nessun dangling pointer e nessun doppio PANIC.

## Piano / criteri di accettazione

| ID | Contratto e test | Stato |
|---|---|---|
| AC-1 | More a 1180×820: aprire con reale bottone, Down, Right su peer root, Escape su peer **child**; RED deve provare PANIC assente | Test-only `00343b8961e608f94514db2abef6c67cf1d3491f`; Windows ND |
| AC-2 | Dopo fix minimo, lo stesso scenario manda il primo Esc a PANIC e preserva Show serializzato, LIVE/VIEW e NEXT | Windows CTest ND |
| AC-3 | Root File/Edit/Show/Cuelists/Devices/View/Guide/More rimangono coperti, normale dismiss non attiva PANIC | Test UX04B-01/02/03 e negativo UX04B-02; ND |
| AC-4 | Child Left→parent Esc opera ancora; menu indipendenti e callback di selezione restano invariati | Test negativo / manual review; ND |
| AC-5 | Version guard, Windows Release/CTest e CodeQL C++ su SHA esatto, nessuna migrazione JSON v10 | GitHub Actions, ND |
| UAT | Tasto Esc fisico Windows, focus e mouse, DPI 100/125/150%, Narrator, IME e misura Audio/MIDI/OSC/LTC su banco isolato | **NOT RUN** |

### Piano reversibile e red-team C++

- Ownership: callback `Theme` deve essere spenta prima della distruzione MainComponent; `SafePointer` impedisce l'accesso a finestre eliminate; ogni listener registrato deve essere rimosso prima di distruggere il listener. La stessa istanza puo' essere applicata a piu' finestre senza duplicare callback.
- Controlli manuali Trail of Bits: referenze pendenti, iterator invalidation in rimozione listener, ricorsione/doppio stop e precondizioni di focus; segnare i falsi positivi. Nessun parser automatico Trail of Bits eseguito.
- Matt Pocock: test al vero `ComponentPeer`, non al root MainComponent. CCteam QA: chiusura ordinaria, riapertura, UI/accessibility e hardware **separati**. Simplify: riutilizzare un proprietario, nessuna infrastruttura globale di input.
- PAUL PLAN/APPLY/UNIFY; SEED application deep; Unlazy gate ledger PASS/FAIL/ND. Tutte skill metodologiche, non prova di scanner o test di dispositivo.

**Rollback:** revert della sola PR #224; nessun merge automatico. I sottomenu piu' profondi, menu contestuali, AlertWindow/DialogWindow, e test fisico restano in #191 finche' non sono verificati con evidenza distinta.

**Stato:** test di riproduzione committato; esecuzione Windows e RED/GREEN ancora ND. Non dichiarare ready-for-LIVE.
