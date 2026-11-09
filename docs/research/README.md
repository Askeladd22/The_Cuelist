# The CUElist — Research & Development Lab

**Istituito:** 9 novembre 2026 · **Stato:** protocollo di ricerca; nessuna feature approvata automaticamente.

Obiettivo: migliorare show-control Windows (Audio, MIDI, OSC, Control Mappings, LTC generato), UX operatore, robustezza e qualità commerciale attraverso fonti attendibili, benchmark, test misurabili e collaudi. **Non è un progetto di riscrittura o di imitazione di QLab.**

La fonte ufficiale della versione è [VERSION.txt](../../VERSION.txt); il codice e la build sono la fonte delle funzionalità effettive; le checklist di rilascio indicano i gate ancora aperti. Consultare [docs/VERSIONING.md](../VERSIONING.md) per qualunque PR, anche documentale.

## Guide operative

| Documento | Funzione |
|---|---|
| [PROTOCOL.md](PROTOCOL.md) | Domande, classificazione evidenze, review critica e gate |
| [BASELINE.md](BASELINE.md) | Funzionalità e rischi attuali, verificati sui file di main |
| [DEEP_RESEARCH_BRIEF.md](DEEP_RESEARCH_BRIEF.md) | Brief per prima Deep research su UX e benchmark |
| [BENCHMARKS.md](BENCHMARKS.md) | Metodo comparativo e fonti primarie da esaminare |
| [VALIDATION.md](VALIDATION.md) | Test automatici, misure esterne, collaudi e metriche |
| [DECISIONS.md](DECISIONS.md) | Questioni aperte, proposte e registro ADR/RQ |
| [CHATGPT_PROJECT_INSTRUCTIONS.md](CHATGPT_PROJECT_INSTRUCTIONS.md) | Istruzioni da copiare in un Progetto ChatGPT |
| [templates/SOURCE_CARD.md](templates/SOURCE_CARD.md) | Scheda fonte |
| [templates/EXPERIMENT_CARD.md](templates/EXPERIMENT_CARD.md) | Piano test preregistrato |
| [templates/OPERATOR_TEST.md](templates/OPERATOR_TEST.md) | Osservazioni e compiti dei tecnici/operatori |
| [templates/HARDWARE_REPORT.md](templates/HARDWARE_REPORT.md) | Misure del banco Windows e dispositivi reali |

## Cinque piste

1. **Competitive UX & operator workflow**: selezione VIEW/LIVE, GO/PANIC, cue list, edit durante playback, mapping, preparazione e recupero dello show.
2. **Audio/reliability**: realtime-safe callback, routing multicanale, dropouts, gestione fault, crash recovery, salvataggio.
3. **MIDI/OSC/LTC**: conformità, stato di dispositivo, packet ordering, sincronismo, generatori LTC e receiver esterni.
4. **Qualità e architettura**: JUCE/C++, Undo/Redo, schema JSON v10, threading, regression, profiling.
5. **Distribuzione**: licensing JUCE e terze parti, installazione portable Windows, documentazione, prove hardware, priorità 1.0.

## Regola fondamentale

Una feature è «implementata» soltanto se presente nel codice corrente; «passa CI» soltanto se esiste un run verde; «validata dal vivo» soltanto con risultati di un vero collaudo su dispositivi e configurazioni dichiarati. Il laboratorio non certifica sicurezza di show control o conformità agli standard da solo.

**Primo mandato RQ-001:** benchmark verificabile del flusso di lavoro di un operatore, non solo confronto estetico. **Secondo mandato RQ-002:** contratto di affidabilità di GO/STOP/PANIC, routing e temporizzazione con metriche end-to-end. Entrambi sono ancora da eseguire.

Non modificare `src/`, progetto `.cuelist` v10 o routing in questo bootstrap. Aprire branch/PR autonomi per ogni esperimento approvato.
