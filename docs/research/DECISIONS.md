# The CUElist — RQ e registro delle decisioni R&D

**Stato:** domande da ricercare. Nessun nuovo comportamento approvato automaticamente dal bootstrap.

| ID | Domanda | Priorità | Stato | Primo esperimento |
|---|---|---|---|---|
| RQ-001 | Quali workflow dell'operatore sono meno sicuri/efficienti di quanto necessario? | Alta | da ricercare | Deep research, task benchmark competitor |
| RQ-002 | Quali latenze e failure mode reali distinguono audio, MIDI, OSC e LTC? | Critica | da misurare | banco con timestamp esterni |
| RQ-003 | VIEW/LIVE, GO/PANIC e Undo/Redo sono pienamente comprensibili e protetti? | Critica | da provare | test operatori e regression |
| RQ-004 | La UI è coerente a DPI e geometrie Windows differenti? | Alta | da provare | screenshot nativi e task focus |
| RQ-005 | Dove intervenire nell'architettura JUCE senza regressioni live? | Alta | da valutare | audit code/test, benchmark isolati |
| RQ-006 | Quali verifiche licenze, packaging e qualità mancano prima della vendita? | Alta | da documentare | matrice license/dependency/gate |

## Modello ADR/RQ

**ADR-### — Titolo**

- Data, decisori, commit/PR, RQ:
- Sintomo e rischio sull'operatore:
- Evidenze primarie / test / output:
- Evidenze contrarie, compromessi e lacune:
- Alternative comparate:
- Decisione (**APPROVED / ITERATE / DEFER / REJECT**):
- Invarianti da preservare:
- Gate tecnici e hardware mancanti:
- Piano di rollback:
- Data/condizione di riesame:

**ADR-000 — Istituzione del laboratorio:** documentazione e metodologia soltanto. VIEW/LIVE, Control Maps, routing, engine e schema .cuelist v10 rimangono invariati.
