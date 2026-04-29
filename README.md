# trattoria-sistema-operativi

## Struttura file

```
src/
├── main.c              ← entry point, parsing argomenti CLI, ciclo istanze
├── server_comm.c/.h    ← protocollo messaggi col server (hello, welcome, instance, end)
├── ipc_manager.c/.h    ← attach/detach SHM, semafori, code messaggi
├── worker.c/.h         ← logica del thread per ogni membro del personale
├── strategy.c/.h       ← strategia profit e reputation
└── state.c/.h          ← struttura dati snapshot stato (sala, cucina, cassa, lavagna, stanchezza)
```

---

## Suddivisione compiti

### Persona 1 — Infrastruttura & Protocollo

#### `main.c`

1. Parsing argomenti CLI (`--strategy <profit|reputation>`)
2. Entry point del programma
3. Chiama `server_comm` per il protocollo e il ciclo di istanze

#### `server_comm.c/.h`

1. Invio messaggio `HELLO` (matricole + strategia)
2. Ricezione messaggio `WELCOME` (parsing info personale: abilità e tratti)
3. Loop istanze:
   - Ricezione `INSTANCE` (speed, strategy, families_n)
   - Creazione di N thread (uno per membro del personale) tramite `worker.c`
   - Attesa `INSTANCE_DONE` (metriche: tempo medio, valutazione media)
   - Join dei thread e reset lavagna prima della prossima istanza
4. Ricezione `END` → terminazione client

#### `ipc_manager.c/.h`

1. `ipc_init()`: `shmget`/`shmat` per dining room, kitchen, blackboard, cashdesk
2. `ipc_init()`: `semget` per il set di semafori (usare `SEMIDX_BLACKBOARD` per la lavagna)
3. `ipc_init()`: `msgget` per le code messaggi (C2S, S2C, FATIGUE)
4. `ipc_cleanup()`: `shmdt` di tutte le memorie condivise
5. Espone puntatori globali alle SHM per gli altri moduli

---

### Persona 2 — Worker Thread & Stato

#### `worker.c/.h`

1. Funzione `worker_thread(staff_id)` — entry point del pthread per ogni membro del personale
2. Loop principale del worker:
   - Legge la stanchezza dalla coda fatigue (`msgrcv` con `mtype = staff_id + 1`)
   - Chiama `state_snapshot()` per ottenere lo stato corrente
   - Chiama `strategy_decide_role()` per decidere il prossimo ruolo
   - Scrive l'assegnazione sulla lavagna (con `semop` su `SEMIDX_BLACKBOARD`)
3. Gestione segnale di fine istanza (flag atomico per uscire dal loop)
4. Gestione sleep/polling per non saturare la CPU

#### `state.c/.h`

1. Struttura `state_snapshot_t` che raccoglie:
   - Stato di ogni tavolo (state, dirt_level, food_qty) da `shm_diningroom_t`
   - Stato cucina (food_ready, pending_orders, piatti) da `shm_kitchen_t`
   - Stato cassa (pending_payments) da `shm_cashdesk_t`
   - Assegnamenti correnti dalla lavagna `shm_blackboard_t`
   - Livello stanchezza corrente per ogni membro del personale (tracciato internamente)
2. `state_take_snapshot()`: legge le SHM e costruisce lo snapshot
3. `state_update_fatigue(staff_id, role, new_level)`: aggiorna il tracking interno della stanchezza

---

### Persona 3 — Strategia

#### `strategy.c/.h`

1. `strategy_decide_role(staff_id, strategy, snapshot, staff_info)` → restituisce il ruolo ottimale (`role_t`)
2. `strategy_profit(...)`: minimizza tempo medio delle famiglie
   - Priorità: assegnare subito pulitori/camerieri ai tavoli in attesa
   - Privilegiare i membri con abilità alta nel ruolo assegnato
   - Cuoco sempre attivo se ci sono ordini pendenti
   - Minimizzare tempi morti
3. `strategy_reputation(...)`: massimizza valutazione media famiglie
   - Assegnare personale con pazienza/socievolezza/professionalità alta ai ruoli di contatto (cameriere, cassiere)
   - Bilanciare qualità del servizio vs velocità
   - Accettare tempi maggiori in favore di personale più qualificato
4. Gestione stanchezza per entrambe le strategie:
   - Rotazione dei ruoli quando la stanchezza è MEDIA per prevenire HIGH
   - Considerare la resistenza del membro per decidere quando ruotare
5. Logica di osservazione delle famiglie:
   - Dopo ordine → osservare `food_qty` per stimare carico cucina
   - Dopo consumo → osservare `dirt_level` per pianificare pulizia

## Relazione

1. Descrizione delle caratteristiche del personale ottenuto;
2. Osservazioni sulle caratteristiche delle famiglie ottenute;
3. Spiegazione delle scelte fatte nella progettazione della strategia, con
   specifico riferimento alle caratteristiche del personale;
4. Spiegazione dei criteri usati per soddisfare i vincoli di tempo di
   completamento minore, di reputazione maggiore ed eventualmente di
   assenza di stanchezza alta;
5. Descrizione delle problematiche incontrate e delle soluzioni adottate;
6. Spiegazione degli apporti specifici di ogni membro del gruppo di
   studenti nello sviluppo del codice e della relazione.
