# RQ-UX03G — Operatore: scorciatoie di sicurezza nella testata colonna compatta

**Data:** 2026-11-20. **Base:** `main@02cce883ea510562b5af46d0139975c2481f2ef3` (0.9.88). **Target:** v0.9.89, draft [PR #220](https://github.com/Askeladd22/The_Cuelist/pull/220). **Formato show:** .cuelist JSON schema v10 immutato.

## Scenario e priorità

Nel pannello nativo JUCE `PinnedColumnHeaders`, aperto tramite Shift+F10 durante una prova o uno spettacolo, la riga inferiore mostra tre comandi in un'unica `label()` con elisione automatica: `ESC  PANIC    SPACE  GO    SHIFT+F10  EXIT`. A 1180×820 la larghezza della barra e' inferiore al testo completo. Il rischio non e' un errore di dispatch dimostrato, ma **nascondere una porzione della guida visibile a GO/PANIC**; questa UI controlla anche l'armamento Delete col doppio tasto `5` → `D`.

**Ipotesi falsificabile:** sostituendo soltanto la stringa grafica con una selezione in base alla larghezza dei glifi JUCE, la testata compatta mostra integralmente `ESC PANIC   SPACE GO` e quella ampia conserva il testo originale con Shift+F10; il testo di accessibilità e la semantica dei comandi restano identici.

## Sorgenti verificati e confronto

- Codice reale: `src/CueViews.cpp` (`PinnedColumnHeaders::paint`, `armedDeleteHintForWidth`), `src/MainComponent.h`, `src/CueOperations.cpp` (descrizione testuale e 5→D), `src/Theme.h` (`label` con `drawText(..., true)`), `tests/UiRefinementTests.h` (UX03F/UX03G).
- [JUCE Graphics::drawText (documentazione ufficiale)](https://docs.juce.com/master/classjuce_1_1Graphics.html): testo troppo largo puo' essere ellissato se `useEllipsesIfTooBig` e' true.
- [JUCE GlyphArrangement (documentazione ufficiale)](https://docs.juce.com/master/classjuce_1_1GlyphArrangement.html): `getStringWidth` misura l'ingombro dei glifi formati.
- Issue [#188](https://github.com/Askeladd22/The_Cuelist/issues/188) e [PR #189](https://github.com/Askeladd22/The_Cuelist/pull/189) riguardavano **la riga centrale Delete**, gia' corretta e integrata nella v0.9.67. La presente RQ riguarda **solo il footer**; non duplicare UX03F.

## PLAN → APPLY → UNIFY (PAUL), SEED Application/Deep, Unlazy AC

- **AC-1:** a 1180×820, screenshot reale `PinnedColumnHeaders::createComponentSnapshot` in modalita' Shift+F10 contiene la grafica completa `ESC PANIC   SPACE GO` nell'area y=49..65; confronto pixel-per-pixel col rendering JUCE atteso.
- **AC-2:** a 1920×1080 mantiene la stringa originale completa `ESC  PANIC    SPACE  GO    SHIFT+F10  EXIT`.
- **AC-3:** `getDescription()` continua a spiegare anche Shift+F10; la bitmap o il focus non mutano `encodeShow`, `liveCuelistIndex()` o `nextCueIndex()`.
- **AC-4:** `5` ripetuto non conferma Delete, soltanto `D` cancella la colonna con ID catturato; ESC continua a invocare PANIC e Space il global GO. Nessun cambiamento ai callback.
- **AC-5:** `VERSION.txt`, CMake, CHANGELOG concordano; branch separato; Windows Release/CTest/CodeQL sullo SHA del codice finale. No merge automatico.

## Red team e revisione manuale Trail of Bits C++ (focus)

**Threat model:** errore operativo locale, non vulnerabilità REMOTE. La nuova funzione e' un selettore di testo per width, senza accesso a file/rete, heap nel callback audio, thread o porte OSC/MIDI. La bitmap testata proviene dal vero oggetto JUCE, non da un mockup HTML. Possibili controesempi: caratteri/font e DPI fisici diversi dai pixel logici; screenshot nativi offscreen non equivalgono a un tasto su hardware Windows; sotto larghezze patologiche un footer completo non puo' fisicamente entrare. Fallback `ESC PANIC` e `ESC` conservano la priorita' al comando di emergenza; la guida accessibile resta completa.

**Ambito manuale C++:** lunghezza/aritmetica larghezza (clamp `jmax`), vita delle stringhe statiche, bounds del rettangolo `{10,49,w-20,17}`, assenza di mutazioni del modello e delle funzioni di routing; nessun scanner automatico Trail of Bits eseguito e nessuna garanzia di codebase coverage. **Simplify Codebase:** nessuna nuova classe, astrazione o modifica del contratto. **Matt Pocock:** test visivo reale RED→GREEN richiesto, senza considerare un guard/version failure un test RED. **CCteam QA:** distinguere Windows CI da DPI fisico/board esterno.

## Ledger di verifiche

| Gate | Evidenza | Esito |
| --- | --- | --- |
| Test-only `976b13f5` | [Windows run 44759807800](https://github.com/Askeladd22/The_Cuelist/actions/runs/44759807800): il versioning guard ha fermato il job prima di CTest | BLOCKED (non RED) |
| Pre-fix con versione `c4a24409` | [Windows run 44760054171](https://github.com/Askeladd22/The_Cuelist/actions/runs/44760054171); test UX03G aggiunto, sorgente paint originale | PENDING (RED da verificare su log) |
| Version guard pre-fix | [run 44760054225](https://github.com/Askeladd22/The_Cuelist/actions/runs/44760054225) | PASS |
| C++ source post-fix `ac12a7e0` | selettore messaggi con metriche JUCE | Windows CTest/CodeQL da verificare |
| Screenshot nativi compact/wide, PDF o manuali | confronto del test offscreen con immagini generiche CI | NOT YET VERIFIED |
| Windows fisico 100/125/150 DPI, Narrator, IME, tastiera, periferiche Audio/MIDI/OSC/LTC e timing | banco non collegato a questa sessione | ND / NOT RUN |

## Rischio e rollback

Nessun cambio al mapping shortcut, trasporto, UI gerarchia o JSON v10: il rollback e' il revert della PR #220. La dicitura accorciata nel compact nasconde `SHIFT+F10 EXIT` **soltanto visivamente**, senza rimuoverne il tasto o la spiegazione accessibile; se test operatori mostrano difficolta', considerare una quarta indicazione non distruttiva in un intervento UX separato.

**Non certifica LIVE:** CI verde non dimostra comportamento degli output fisici o sincronismo a valle di UDP/MIDI; PANIC non e' un comando di sicurezza hardware garantito.
