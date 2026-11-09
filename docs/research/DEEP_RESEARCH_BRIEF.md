# RQ-001 — Deep research: competitività, UX e affidabilità dell'operatore

**Stato:** brief pronto per una sessione **Deep research** ChatGPT; non è una ricerca già eseguita.

## Prompt da copiare in Deep research

Conduci una ricerca multidisciplinare, documentata e **critica**, sulla progettazione di un software professionale Windows per show control chiamato **The CUElist**. Stack attuale: C++17/JUCE 8, Audio + MIDI + OSC + Control Mappings, LTC audio generato, progetti JSON v10; non è una DAW e non riproduce video nativamente. Destinatari: tecnici di teatro, live, installazioni, operatori di cue.

Problema: progettare flussi operatore più sicuri e veloci e un software affidabile **senza imitare visivamente QLab, eliminare feature esistenti o riscrivere l'engine**. Il prodotto ha già VIEW e LIVE separate; il punto da verificare è quanto questa distinzione sia comprensibile in pratica.

**Ricerca obbligatoria:**
1. Reperisci manuali ufficiali e tutorial tecnici su QLab 5 e **almeno altri 3 sistemi pertinenti**, possibilmente includendo show control Windows. Verifica versione/piattaforma e non inventare funzioni non documentate.
2. Confronta operator task flow reali: creare/rinominare/riordinare cues; eseguire GO e PANIC; passare tra list VIEW e LIVE; modificare in prova e durante playback; risolvere media assenti; attivare cue con MIDI/OSC; recovery da fault.
3. Per ogni operazione prepara una matrice con passi minimi, scorciatoie/focus, feedback, conferme per azioni distruttive, modalità di errore e recupero. **ND** quando un dato non è documentato; non dedurre i clic da screenshot statici.
4. Analizza pratiche di progettazione sicura di software audio in tempo reale (callback, bounded queues, device switching, audio underflow); MIDI/OSC delivery/ordering e sincronismo; perdita di clock LTC; riscontri da interviste/issue di operatori.
5. Classifica **solo ipotesi** e opportunità concrete: 3 miglioramenti ad alto impatto e basso rischio, 3 rischi principali, 3 cose che non toccheresti. Considera vincoli Windows/JUCE, schema v10, politica UI di no-placeholder e obiettivo commerciale one-time purchase.
6. Proponi esperimenti falsificabili e test su persone/dispositivi veri: task-based completion, errori, mediane/percentili, scenari di recovery; distinguere test automatici, simulati ed end-to-end hardware.
7. Sottoponi le tre proposte a un **red team**: controargomenti, costi di implementazione, potenziali regressioni e conflitti con l'operatore. Raccomanda una sequenza di prove, **non una nuova UI da implementare immediatamente**.

**Fonti prioritarie:** documentazione ufficiale del software, MIDI Association (MIDI/MTC/MSC), specifica OSC, fonti tecniche JUCE, ricerche HCI/UX disponibili; usare forum/community come esperienze non statisticamente rappresentative. Cita link, versione, data, sezioni e contraddizioni; segnala testi non accessibili. Non spacciare la ricerca online per misurazione di hardware.

**Contesto GitHub:** `Askeladd22/The_Cuelist`, `docs/research/BASELINE.md`, `docs/0.9-release-checklist.md`, `docs/stability-and-timing.md`, `docs/UI_V2_VISUAL_DESIGN_PRINCIPLES.md`, `docs/VERSIONING.md`, `README.md`. Verifica commit esatto; se non accessibile, chiedi/usa file esportati senza dichiararli letti.

**Deliverable:** report con fonti, matrice comparativa, ipotesi di miglioramento motivate, controargomenti, priorità rischio/valore, piano di benchmark e protocollo hardware/UX. Non modificare repo e non promettere certificazione per show dal vivo.

## Dopo il report

Registrare le fonti e il report nel laboratorio; aprire esperimenti separati per ogni raccomandazione. Nessuna funzione viene promossa dal risultato Deep research senza review e validazione.
