# RQ-001 / T03 — Riconoscibilità della Cuelist LIVE

**Data:** 2026-11-09  
**Baseline:** `main@895346a32272efd8fb7db338075187d401df38ac` (The CUElist 0.9.33).  
**Ambito:** osservazione UX e regression test nativi JUCE. Nessuna proposta di modifica alla UI prima della misura.  
**Stato:** protocollo predisposto, osservazioni di operatori **ND**, DPI Windows reale **ND**, banco hardware **ND**.

## 1. Scenario e problema

Un operatore sta visualizzando `Emergency` mentre `Pre-show` è la Cuelist **LIVE**, con la cue n. 02 armata per il prossimo GO globale. Il programma conserva la separazione VIEW/LIVE anche quando l'operatore cambia tab, ma a 1180×820 la tab `Pre-show` può trovarsi tra quelle nascoste nell'overflow.

Nel codice 0.9.33 il LIVE indicator sulla tab è un punto verde da 6 px, visibile solo se la tab è mostrata. Il pannello `ON DECK` rende il numero della prossima cue senza il nome della lista; il menu overflow presenta la spunta per VIEW, non per LIVE. Il footer può riportare messaggi di stato contestuali: non nasconderlo artificialmente durante la prova.

**Ipotesi H-T03:** quando la LIVE è fuori dalla barra tab visibile, gli operatori impiegano più tempo e/o sbagliano più spesso l'identificazione della destinazione di Space rispetto alla condizione con LIVE visibile.

**Controipotesi:** l'operatore può riconoscere la destinazione anche grazie a ON DECK, informazioni di stato o esperienza con il concetto di trasporto; una nuova etichetta permanente potrebbe aggiungere rumore visivo senza beneficio misurabile.

## 2. Fonti e gerarchia dell'evidenza

- **Sorgente applicativo** (evidenza di implementazione, non test operatori): [CuelistTabButton](https://github.com/Askeladd22/The_Cuelist/blob/895346a32272efd8fb7db338075187d401df38ac/src/MainComponent.h#L63-L90), [overflow e refresh tab](https://github.com/Askeladd22/The_Cuelist/blob/895346a32272efd8fb7db338075187d401df38ac/src/MainComponent.cpp#L1009-L1135), [GO globale](https://github.com/Askeladd22/The_Cuelist/blob/895346a32272efd8fb7db338075187d401df38ac/src/MainComponent.cpp#L1969-L1992).
- **API JUCE** (documentazione tecnica): [Component::createComponentSnapshot](https://docs.juce.com/master/classjuce_1_1Component.html) crea un'immagine del componente e dei suoi figli. Lo snapshot off-screen non dimostra scaling nativo di Windows né usabilità.
- **Standard concettuale:** [ISO 9241-11:2018](https://www.iso.org/standard/63500.html) inquadra l'usabilità in efficacia, efficienza e soddisfazione nel contesto d'uso. Non prescrive soglie numeriche per questo prodotto né sostituisce un test sul campo.

## 3. Condizioni sperimentali, dati non simulati

Stesso stato in entrambe le condizioni:
- 9 liste con nomi distinti: `Pre-show`, `Act I`, `Act II`, `Act III`, `Intermission`, `Finale`, `Backup`, `Soundcheck`, `Emergency`.
- LIVE: `Pre-show` (index 0); prossimo GO globale: cue 02.
- VIEW: `Emergency` (index 8).
- **A / compact:** finestra JUCE **1180×820** in unità logiche; LIVE tab nascosta e overflow visibile.
- **B / wide:** finestra JUCE **1920×1080** in unità logiche; LIVE e VIEW entrambe visibili.
- Output MIDI, OSC, LTC e audio esterni scollegati. Usare l'eseguibile in setup non collegato allo show reale.

Screenshot di confronto, ottenuti dai **componenti JUCE reali** nella suite CTest:
- `interface-rq001-t03-live-hidden-1180.png`
- `interface-rq001-t03-live-visible-1920.png`

Questi PNG sono rasterizzati dal test off-screen, **non** screenshot del desktop Windows a 100/150% DPI. Per la prova DPI occorre catturare la finestra reale con scaling di Windows verificato. Le due dimensioni sono un confronto di layout, non una prova di accessibilità.

## 4. Protocollo operatore — pilota esplorativo

**Campione obiettivo:** 12 partecipanti, idealmente 6 con esperienza di show control e 6 con meno esperienza; registrare esperienza auto-riferita senza dati identificativi. Ogni operatore prova A e B, con ordine bilanciato (6 A→B, 6 B→A). Il campione è esplorativo: non giustifica certificazioni statistiche né percentuali di sicurezza per lo show.

1. Spiegare una volta sola che VIEW indica la lista visualizzata e LIVE quella che riceve il GO globale. **Non rivelare quale sia LIVE nella schermata di prova.**
2. Mostrare il layout a schermo intero o nell'app nativa senza indicare con il puntatore la risposta; non mostrare i nomi file degli screenshot (contengono la soluzione).
3. Domanda neutra: «Se premi ora Space, quale Cuelist e quale cue riceveranno GO?». Cronometrare dal momento in cui il layout è leggibile alla prima risposta completa.
4. Registrare risposta esatta, tempo, correttezza, esitazioni, richieste di aiuto e fiducia (1–5). Non suggerire `Pre-show`.
5. Chiedere successivamente come tornare alla LIVE mantenendo il trasporto. Registrare il percorso, senza penalizzare un operatore che prima ha risposto «non so».
6. Ripetere nella condizione alternativa. Su display adatto ripetere il percorso manuale al **150% di Windows**, separando i dati dal confronto A/B al 100%. Non confondere ingrandimento PNG con DPI reali.

**Risposta di controllo del facilitatore:** `Pre-show`, cue 02. La risposta può essere detta in modo equivalente. Nessun GO deve essere inviato a dispositivi reali durante il test.

## 5. Metriche e falsificabilità

- **Efficacia primaria:** identificazione esatta di Cuelist **e** numero cue alla prima risposta, per A e B. Separare errore sulla Cuelist da errore sul numero.
- **Efficienza:** tempo in millisecondi alla prima risposta, mediana e p90 per condizione, e differenza A−B per ogni partecipante.
- **Soddisfazione/fiducia:** voto dichiarato da 1 a 5, senza attribuirlo a un test di accessibilità.
- **Rischio operatore:** ogni previsione di GO sulla lista sbagliata è un potenziale errore critico da investigare, anche con un solo episodio.
- **Misura di supporto:** numero di aperture overflow e richieste di aiuto; annotare se la persona si affida a colore, ON DECK, footer o menu.

**Gate esplorativo predefinito (non norma ISO):** approfondire redesign se almeno 2/12 sbagliano la lista LIVE nella condizione A, oppure se la mediana A è almeno doppia della B. Anche un singolo errore impone analisi qualitativa. Con zero errori non dichiarare affidabilità live senza campione maggiore e hardware.

I record vanno nel file `RQ-001-T03-OBSERVATIONS.csv`: una riga per partecipante/condizione, senza nome, email, audio, video o identificatori privati. Non riempire le righe con dati ipotetici.

## 6. Red team

- Ordine A/B ed esperienza producono effetti d'apprendimento: bilanciare l'ordine e mostrare prima gli screenshot al facilitatore per verificarne l'equivalenza.
- Il nome `Emergency` potrebbe indurre un'interpretazione di panico: annotare esplicitamente se influenza la previsione.
- Uno screenshot statico non mostra tutti i tooltip o il menu overflow aperto: svolgere almeno una prova interattiva nel programma vero.
- Gli indicatori visivi potrebbero risultare leggibili su 1920 px ma non con scaling Windows 150%, distanza operatore o contrasto ridotto; verificare fisicamente.
- La ricerca non misura il transito MIDI/OSC/LTC a valle: niente affermazioni di sincronizzazione esterna.

## 7. QA e gate di uscita

- [ ] Compilazione e CTest Windows della PR T03 superati; registrare SHA, log e numero effettivo di assert.
- [ ] Esaminati entrambi i PNG nativi e confermata la corrispondenza fra stato e immagine.
- [ ] DPI nativo Windows 100% e 150% con screenshot reali; annotare monitor e scaling.
- [ ] Pilota operatori eseguito e dati registrati con autorizzazione.
- [ ] Valutazione dei risultati con potenziali controesempi, senza cambiare UI sulla base di soli screenshot.
- [ ] Hardware MIDI/OSC/LTC: gate separato.
- [ ] Decisione ADR: lasciare UI invariata oppure prototipare un'indicazione LIVE più esplicita e ripetere il test.

**Rollback:** PR sperimentale, solo test/documenti; chiuderla senza merge in caso di incompatibilità. Nessun cambio di transport, UI o schema JSON v10.
