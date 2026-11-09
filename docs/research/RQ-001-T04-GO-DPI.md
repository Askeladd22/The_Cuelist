# RQ-001 / T04 — Ambiguità GO e nomi LIVE simili: protocollo di sicurezza operatore

**Data:** 2026-11-09  
**Baseline verificata:** `main@8702fbe7db05d6e5cec7609313f15a19a6b58240`, `VERSION.txt` e CMake = **0.9.36**, schema `.cuelist` JSON **v10**.  
**Iterazione proposta:** 0.9.37, branch `research/rq001-t04-go-target-long-names`; PR draft, senza merge automatico.

## 1. Scenario operativo

L'operatore visualizza **Emergency (VIEW)** mentre **Act I (LIVE)** ha un'altra cue armata. Il pulsante laterale **GO / SPACE** agisce sulla LIVE, ma il pulsante verde **GO** della riga visualizzata avvia intenzionalmente la cue di Emergency e può trasferire la LIVE dopo aver fermato la precedente. La stessa differenza funzionale si osserva con nomi di Cuelist di 64 caratteri aventi prefissi identici: il pannello ON DECK offre solo 94 px alla visualizzazione del nome, perciò il solo testo visibile potrebbe non disambiguare la destinazione.

### Evidenza, non inferenza

- **Codice 0.9.36:** `MainComponent::goNext()` distingue VIEW/LIVE; `MainComponent::triggerRow()` trasferisce LIVE quando un GO di riga è lanciato dalla VIEW. `PinnedCueHeaders::mouseMove()` dava soltanto il tooltip generico «GO — trigger this cue».
- **Screenshot nativi inviati dall'utente:** T03-B a 1180×820 e 1920×1080: `Pre-show` e `GO 02` leggibili nel pannello, `VIEW NEXT` chiaro. **Non** sono screenshot Windows al 150% DPI.
- **Windows CI precedente:** PR #157, run `42691760123` sul commit `421db1229297a9db14c76e05de3cb848a5f70803`: compilazione Windows/CTest e upload `native-workspace-previews` **PASS**, 420.039 asserzioni e zero test falliti. La PR è stata poi integrata in `main`; nessuna deduzione su hardware esterno o errori di operatori.

## 2. Ipotesi misurabili, alternative, rischio

**H-T04A — distinzione dei GO:** un tooltip nativo esplicito che specifichi «Row GO ... stop LIVE / switch LIVE» riduce la confusione tra GO di riga e GO globale rispetto al tooltip generico. **Controipotesi:** sotto pressione l'operatore non passa il mouse sui pulsanti; il tooltip è tardivo e dunque insufficientemente visibile, anche se correttamente associato.

**H-T04B — nomi lunghi simili:** visualizzare un piccolo identificatore **L01/L02** (posizione corrente della Cuelist nell'ordine delle tab) sul pannello ON DECK consente di distinguere due LIVE con prefisso visivo identico senza cambiare il nome completo, il trasporto o le dimensioni. **Controipotesi:** il numero della lista non è un'identità persistente; un riordino delle tab cambia Lxx e un operatore può confonderlo con il numero della cue. In assenza di prova operatore, Lxx non è un segnale di sicurezza certificato.

**Rischio:** un GO alla Cuelist sbagliata può interrompere playback Audio/MIDI/OSC in corso o anticipare una cue e compromettere lo show. Qualsiasi fraintendimento registrato deve essere trattato come segnale di allarme qualitativo, non ignorato perché la media è buona.

## 3. Cambiamenti minimi JUCE

- Nel pannello ON DECK (156×51 px) compare **Lxx** in alto a destra: è la *posizione corrente della Cuelist LIVE*, non un ID serializzato. Rimangono il nome LIVE, `GO NN` / `END`, la geometria e le hitbox di GO/PANIC.
- Al passaggio del mouse sul GO di riga nel componente nativo `PinnedCueHeaders`, il tooltip distingue:
  - VIEW diversa da LIVE: indica espressamente nome LIVE precedente, nome VIEW e trasferimento intenzionale della LIVE;
  - VIEW uguale a LIVE: indica GO sulla stessa lista e il numero di cue.
- Il sottotitolo `VIEW / 1 cue / 0 clips` usa singolare/plurale corretto senza mutare il modello.
- Nessuna modifica a `goNext`, `triggerRow`, engine, code di trasporto, Control Maps, Undo/Redo, serialization v10.

**Trasparenza:** Lxx non è ancora una soluzione approvata agli errori umani. Il nome integrale rimane verificabile nel tooltip GO globale e nel menu overflow; lo screen reader richiede collaudo distinto.

## 4. Test automatici, screenshot e risultati

**CTest JUCE** aggiunge:
1. GO globale dalla VIEW: rimane sulla LIVE originaria e riconosce la cue armata; click **nativo** sul GO della riga della VIEW: trasferisce intenzionalmente la LIVE alla VIEW e verifica la cue riconosciuta dal motore; il tooltip si aggiorna secondo il nuovo ruolo. Non simulare un click di riga con chiamata diretta a `triggerRow`.
2. Nomi massimi di 64 caratteri con prefisso comune: l'identità completa è preservata nel modello/overflow e il piccolo identificatore pittorico L01 vs L02 rende diversi i due snapshot.
3. Tre immagini native JUCE, se la suite passa: `interface-rq001-t04-long-live-1-1180.png`, `interface-rq001-t04-long-live-2-1180.png`, `interface-rq001-t04-long-live-2-1920.png`.

**Gate 0.9.37:** CMake/MSBuild/CTest Windows, 0 test falliti, generazione dei PNG, review visiva degli stessi, controesempio names/ordinal. La cattura off-screen usa la pipeline JUCE ma **non** misura DPI effettivi né funzioni di Windows fuori dal processo.

## 5. Banco manuale Windows: 100% e 150% DPI

Eseguire su **Windows desktop reale**, con output show non collegati, copia del progetto di test e audio in condizioni sicure. Non modificare la configurazione del monitor durante un concerto.

1. In **Impostazioni → Sistema → Schermo → Ridimensionamento**, impostare **100%**; annotare risoluzione fisica, display, modalità e versione OS. Chiudere e riaprire l'eseguibile della build identificata dallo SHA.
2. A 1180×820 (o alla massima dimensione sostenibile senza andare fuori schermo), creare 9 Cuelist e rendere VISIBILE Emergency, con LIVE su una delle prime due; fotografare **l'intera finestra Windows** tramite Strumento di cattura, annotando timestamp e scaling.
3. Evidenziare col puntatore il **GO globale**, leggere il tooltip e registrare che corrisponda alla LIVE ancora attiva. Passare sul GO della riga di Emergency, leggere il tooltip, poi fare click **solo su banco non connesso** e controllare che lo stato LIVE si trasferisca come annunciato.
4. Ripetere al **150% DPI** riaprendo l'applicazione; verificare leggibilità dell'ordinale, collisioni tra etichette, ritagli e dimensioni dei target, oltre a tooltip e overflow. Ripetere per un nome di 64 caratteri con prefisso simile e un caso `END`.
5. Se disponibile, prova ad alto contrasto e navigazione tastiera; non dichiarare automaticamente accessibile lo stato finché lo screen reader non è testato.

**Segnalare separatamente:** (a) risultato automatico, (b) screenshot off-screen, (c) screenshot del desktop Windows 100%, (d) 150%, (e) test con operatori, (f) dispositivi Audio/MIDI/OSC/LTC esterni.

## 6. Pilota con operatori e gate di giudizio

Proporre almeno 12 partecipanti volontari, con esperienza eterogenea, e controbilanciare gli ordini A/B. Evitare che tutti vedano prima la versione nuova. Gli scenari dovranno essere equivalenti ma con nomi, posizioni e numeri cue controbilanciati per non affidarsi alla memoria.

- Q1: «Se premi ora **Space**, quale Cuelist e quale cue riceveranno GO?»
- Q2: «Se premi invece il **GO della riga** qui visibile, cosa succederà alla LIVE?»
- Q3: «Tra due Cuelist con nome lungo e prefisso simile, quale risulta LIVE?»
- Registrare prima risposta, correttezza, tempo in millisecondi, incertezza 1–5 e uso dei tooltip; un clic su GO vero è consentito solo sul banco scollegato dagli output dello show.
- Tracciare su CSV senza dati identificativi `RQ-001-T04-OBSERVATIONS.csv`; nessun dato sintetico.

**Gate esplorativo:** anche un solo operatore che prevede l'invio alla lista sbagliata impone revisione prima della produzione; zero errori in un pilota piccolo non certifica l'assenza di rischio. Latenza e sincronismo a valle dei protocolli esterni **ND** fino a banco hardware strumentato.

## 7. Red team e rollback

- L01/L02 cambia al riordino delle tab: va verificato al test e segnalato all'operatore. Se crea confusione, **rimuovere solo l'ordinale** in una PR mirata, mantenendo gli altri miglioramenti.
- Il testo del tooltip non sostituisce un avviso visibile prima di un GO ad alto rischio: se la prova Q2 fallisce anche con tooltip, discutere un indicatore persistente dedicato in nuova decisione UX, non aggiungere un dialogo modale di conferma che ritardi GO.
- I nomi identici sono proibiti dalla validazione del modello; i prefissi identici **no**. Non modificare i nomi utente automaticamente.
- Un test che controlla la cue ACK nel motore non dimostra che un dispositivo MIDI/OSC/LTC esterno abbia ricevuto l'evento con timing certificato.
- **Rollback:** branch isolato, file di progetto v10 invariati; chiudere la PR non integrata o revertire le modifiche visuali/documentali, poi ricompilare e rieseguire CTest. **Nessun merge automatico.**

## Risultati dichiarabili

- **0.9.36 baseline:** Windows CI e screenshot acquisiti, PASS.
- **0.9.37 candidato:** codice e test predisposti; CI, screenshot, DPI reali e prova operatori **ND** fino a esecuzione.
- **Pronto per spettacolo live / hardware esterno:** **NO — non verificato**.
