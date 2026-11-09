# RQ-001 — benchmark competitivo e fonti iniziali

**Non è una recensione o un risultato sperimentale.** I riferimenti sono punti di partenza identificati il 9 novembre 2026; confrontare versioni, sistemi operativi e condizioni di licenza prima di trarre conclusioni.

| Fonte di partenza | Da studiare | Precauzione |
|---|---|---|
| [QLab 5 — Cue Lists](https://qlab.app/docs/v5/fundamentals/cue-lists/) | liste attive, selezione, GO, target, trigger | prodotto macOS, terminologia e comportamenti non trasferibili automaticamente |
| [QLab 5 — Manuale](https://qlab.app/docs/v5/) | editing cues, control, timecode, routing, recovery | citare sezioni e versione, distinguere funzione documentata e test diretto |
| [MIDI Association — Core Specifications](https://midi.org/midi-1-0-core-specifications) | MIDI 1.0, MTC, MSC, formato dei messaggi | norma/protocollo, non benchmark UX; verificare accesso alle specifiche integrali |
| [OSC 1.0 — specifica](https://opensoundcontrol.stanford.edu/spec-1_0.html) | messages, type tags, bundles, scheduling | trasporto UDP non garantisce consegna/acknowledgement |
| [JUCE Documentation](https://docs.juce.com/) | API audio/MIDI e best practice di thread | implementazione specifica da confrontare con codice corrente |

**Ricerca da completare:** almeno tre prodotti alternativi al riferimento QLab, includendo un prodotto Windows. Non inserire un competitor come verificato prima di consultare documentazione ufficiale o provarlo davvero.

## Compiti comparabili proposti

- **T1:** creare uno show, quattro cue Audio, una MIDI, una OSC e una Control Map; impostare ordine e nomi.
- **T2:** suonare una cue LIVE, aprire/modificare altra lista VIEW senza cambiare il prossimo GO.
- **T3:** disconnettere MIDI Output durante playback e recuperare il device per identità corretta.
- **T4:** OSC destinazione irraggiungibile: distinguere «inviato» da «ricevuto».
- **T5:** modificare cue e fare Undo/Redo senza fermate inattese; documentare le operazioni volutamente pericolose.
- **T6:** recovery con media assenti e progetto salvato danneggiato, senza perdere uno show valido già caricato.
- **T7:** LTC Audio verso decoder esterno in un banco line-level isolato; no altoparlanti.

## Dataset obbligatorio per ciascun prodotto

Versione, OS, fonte/video/bench reale, operazione esatta, prerequisiti, numero di step, tempo (solo se misurato), errore possibile, feedback visivo/uditivo, ripristino, lacune e note dell'osservatore.

**Criteri:** affidabilità della scelta in LIVE, efficienza operatori, leggibilità dei rischi, recuperabilità, apprendimento, accessibilità, compatibilità con workflow reali. Evitare un «punteggio complessivo» privo di dati.
