#include "worker.h"
#include "ipc_manager.h"
#include "strategy.h"
#include "state.h"

#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <unistd.h>
#include <errno.h>

// ---- Flag atomico: TR_TRUE mentre l'istanza corrente è in esecuzione ----
static atomic_int g_instance_running = 0;

// ---- Intervallo di polling (microsecondi) ----
#define WORKER_POLL_INTERVAL_US  50000   /* 50 ms */

/* ---------- helper pubblici ------------------------------------------------ */

void worker_start_instance(void) {
    state_reset_fatigue();
    atomic_store(&g_instance_running, 1);
}

void worker_stop_instance(void) {
    atomic_store(&g_instance_running, 0);
}

/* ---------- helper interni ---------------------------------------------- */

/**
 * @brief Svuota tutti i messaggi di stanchezza pendenti per questo membro dello staff (non bloccante).
 *        Aggiorna g_staff_fatigue con l'ultimo livello ricevuto.
 */
static void drain_fatigue(int staff_id) {
    msg_fatigue_t fmsg;
    /*
     * mtype = staff_id + 1 (SysV richiede mtype > 0).
     * IPC_NOWAIT così non blocchiamo mai — consumiamo solo quello che è disponibile.
     */
    while (msgrcv(q_fatigue, &fmsg, sizeof(fmsg) - sizeof(long),
                  staff_id + 1, IPC_NOWAIT) != -1) {
        state_update_fatigue(staff_id, fmsg.role, fmsg.new_lvl);
    }
    /* ENOMSG / EAGAIN sono previsti quando la coda è vuota — ignorali. */
}

/**
 * @brief Rimuove qualsiasi assegnazione precedente di @p staff_id dalla lavagna (blackboard).
 *        Il chiamante deve detenere il semaforo della lavagna.
 */
static void clear_assignment(int staff_id) {
    for (int t = 0; t < shm_blackboard->tables_n; t++) {
        if (shm_blackboard->tables[t].waiter == staff_id)
            shm_blackboard->tables[t].waiter = -1;
        if (shm_blackboard->tables[t].cleaner == staff_id)
            shm_blackboard->tables[t].cleaner = -1;
    }
    if (shm_blackboard->cook       == staff_id) shm_blackboard->cook       = -1;
    if (shm_blackboard->cashier    == staff_id) shm_blackboard->cashier    = -1;
    if (shm_blackboard->dishwasher == staff_id) shm_blackboard->dishwasher = -1;
}

/**
 * @brief Scrive la nuova assegnazione del ruolo per @p staff_id sulla lavagna.
 *        Per i ruoli per tavolo (WAITER / HELPER) scegliamo il miglior tavolo disponibile.
 *        Il chiamante deve detenere il semaforo della lavagna.
 */
static void write_assignment(int staff_id, role_t role) {
    switch (role) {

    case ROLE_COOK:
        if (shm_blackboard->cook == -1 || shm_blackboard->cook == staff_id)
            shm_blackboard->cook = staff_id;
        break;

    case ROLE_CASHIER:
        if (shm_blackboard->cashier == -1 || shm_blackboard->cashier == staff_id)
            shm_blackboard->cashier = staff_id;
        break;

    case ROLE_DISHWASHER:
        if (shm_blackboard->dishwasher == -1 || shm_blackboard->dishwasher == staff_id)
            shm_blackboard->dishwasher = staff_id;
        break;

    case ROLE_WAITER: {
        /*
         * Priorità: prova prima i tavoli che hanno cibo pronto da servire,
         * poi i tavoli che sono in stato TAKEN (hanno bisogno di prendere l'ordine).
         */
        int best = -1;

        /* 1. Tavoli con cibo pronto e nessun cameriere assegnato */
        for (int t = 0; t < shm_blackboard->tables_n; t++) {
            if (shm_blackboard->tables[t].waiter == -1 &&
                shm_kitchen->food_ready[t] == TR_TRUE) {
                best = t;
                break;
            }
        }
        /* 2. Fallback: tavoli in stato TAKEN (in attesa di ordine) */
        if (best == -1) {
            for (int t = 0; t < shm_blackboard->tables_n; t++) {
                if (shm_blackboard->tables[t].waiter == -1 &&
                    shm_diningroom->tables[t].state == TABLE_TAKEN) {
                    best = t;
                    break;
                }
            }
        }
        if (best != -1) {
            shm_blackboard->tables[best].waiter = staff_id;
        }
        break;
    }

    case ROLE_HELPER: {
        /* Scegli il tavolo libero più sporco che non ha ancora un addetto alla pulizia. */
        int best = -1;
        level_t worst_dirt = LVL_NONE;

        for (int t = 0; t < shm_blackboard->tables_n; t++) {
            if (shm_blackboard->tables[t].cleaner == -1 &&
                shm_diningroom->tables[t].state == TABLE_FREED) {
                level_t d = shm_diningroom->tables[t].dirt_level;
                if (d > worst_dirt) {
                    worst_dirt = d;
                    best = t;
                }
            }
        }
        if (best != -1) {
            shm_blackboard->tables[best].cleaner = staff_id;
        }
        break;
    }

    case ROLE_NONE:
    default:
        /* Niente da scrivere — il membro dello staff sta riposando. */
        break;
    }
}

/* ---------- blocco / sblocco semaforo --------------------------------------- */

static void blackboard_lock(void) {
    struct sembuf op = { .sem_num = SEMIDX_BLACKBOARD, .sem_op = -1, .sem_flg = 0 };
    while (semop(sem_id, &op, 1) == -1) {
        if (errno == EINTR) continue;   /* interrotto da segnale — riprova */
        perror("semop lock BLACKBOARD");
        return;
    }
}

static void blackboard_unlock(void) {
    struct sembuf op = { .sem_num = SEMIDX_BLACKBOARD, .sem_op = 1, .sem_flg = 0 };
    if (semop(sem_id, &op, 1) == -1) {
        perror("semop unlock BLACKBOARD");
    }
}

/* ---------- entry point del thread -------------------------------------------- */

void *worker_thread(void *arg) {
    worker_args_t *wa = (worker_args_t *)arg;
    const int   sid      = wa->staff_id;
    const strategy_t strat = wa->strategy;
    const staff_member_t *staff_info = wa->staff_info;
    const int   staff_n  = wa->staff_n;

    printf("[WORKER %d] Thread avviato.\n", sid);

    while (atomic_load(&g_instance_running)) {

        /* 1. Svuota messaggi di stanchezza pendenti (non bloccante) */
        drain_fatigue(sid);

        /* 2. Scatta una istantanea (snapshot) dello stato attuale */
        snapshot_t snap;
        state_take_snapshot(&snap);

        /* 3. Chiedi al modulo strategy il ruolo ottimale */
        role_t new_role = strategy_decide_role(sid, strat, &snap,
                                               staff_info, staff_n);

        /* 4. Scrivi l'assegnazione sulla lavagna (sezione critica) */
        blackboard_lock();
        clear_assignment(sid);
        write_assignment(sid, new_role);
        blackboard_unlock();

        /* 5. Sleep per evitare di saturare la CPU */
        usleep(WORKER_POLL_INTERVAL_US);
    }

    /* Pulisci la propria assegnazione prima di uscire */
    blackboard_lock();
    clear_assignment(sid);
    blackboard_unlock();

    printf("[WORKER %d] Thread in uscita.\n", sid);
    return NULL;
}
