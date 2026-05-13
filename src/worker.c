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
#include <stdlib.h>

// ---- Flag atomico: TR_TRUE mentre l'istanza corrente è in esecuzione ----
static atomic_int g_instance_running = 0;

// ---- Intervallo di polling (microsecondi) ----
#define REPUTATION_POLL_INTERVAL_US  50000   /* 50 ms */
#define PROFIT_POLL_INTERVAL_US 20000 /* 20 ms */

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
                    shm_diningroom->tables[t].state == TABLE_TAKEN &&
                    shm_diningroom->tables[t].food_qty == LVL_NONE) {
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
        /* Scegli un tavolo libero che ha bisogno di essere pulito e non ha ancora un addetto. */
        int best = -1;
        int candidates[5]; // Assumo massimo 5 tavoli
        int count = 0;
        for (int t = 0; t < shm_blackboard->tables_n; t++) {
            if (shm_blackboard->tables[t].cleaner == -1 &&
                shm_diningroom->tables[t].state == TABLE_FREED &&
                shm_diningroom->tables[t].dirt_level > 0) {
                candidates[count++] = t;
            }
        }
        if (count > 0) {
            best = candidates[rand() % count];
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

static void dump_info_to_file(const staff_member_t *staff_info, int staff_n, strategy_t strategy) {
    FILE *f = fopen("info.txt", "w");
    if (!f) return;

    fprintf(f, "Strategia Attuale: %s\n\n", (strategy == STRATEGY_PROFIT) ? "PROFIT" : (strategy == STRATEGY_REPUTATION) ? "REPUTATION" : "SCONOSCIUTA");


    fprintf(f, "\n--- ABILITÀ E TRATTI ---\n");
    fprintf(f, "%-10s | %-9s | %-9s | %-9s | %-9s | %-9s | %-12s | %-12s | %-10s\n", 
            "Staff", "Cameriere", "Cuoco", "Aiutante", "Cassiere", "Pazienza", "Socievolez", "Profession.", "Resistenza");
    fprintf(f, "---------------------------------------------------------------------------------------------------------\n");
    for (int i = 0; i < staff_n; i++) {
        const char *level_names[] = {"LOW", "MED", "HIGH"};
        fprintf(f, "%-10s | %-9s | %-9s | %-9s | %-9s | %-9s | %-12s | %-12s | %-10s\n", 
                staff_info[i].name, 
                level_names[staff_info[i].skills[0]],
                level_names[staff_info[i].skills[1]],
                level_names[staff_info[i].skills[2]],
                level_names[staff_info[i].skills[3]],
                level_names[staff_info[i].traits[0]],
                level_names[staff_info[i].traits[1]],
                level_names[staff_info[i].traits[2]],
                level_names[staff_info[i].traits[3]]);
    }

    fprintf(f, "\n--- STANCHEZZA ---\n");
    fprintf(f, "%-10s | %-10s\n", "Staff", "Livello");
    fprintf(f, "-----------------------\n");
    for (int i = 0; i < staff_n; i++) {
        level_t fat = state_get_fatigue(i);
        fprintf(f, "%-10s | %d\n", staff_info[i].name, fat);
    }

    fprintf(f, "\n--- BLACKBOARD STATUS ---\n");
    fprintf(f, "%-10s | %-15s\n", "Staff", "Ruolo Attuale");
    fprintf(f, "---------------------------\n");

    for (int i = 0; i < staff_n; i++) {
        const char *name = staff_info[i].name;
        const char *role = "Riposo";
        char details[64] = "";

        if (shm_blackboard->cook == i) {
            role = "Cuoco";
        } else if (shm_blackboard->cashier == i) {
            role = "Cassiere";
        } else if (shm_blackboard->dishwasher == i) {
            role = "Lavapiatti";
        } else {
            for (int t = 0; t < shm_blackboard->tables_n; t++) {
                if (shm_blackboard->tables[t].waiter == i) {
                    role = "Cameriere";
                    snprintf(details, sizeof(details), " (Tavolo %d)", t);
                    break;
                }
                if (shm_blackboard->tables[t].cleaner == i) {
                    role = "Aiutante";
                    snprintf(details, sizeof(details), " (Tavolo %d)", t);
                    break;
                }
            }
        }
        fprintf(f, "%-10s | %s%s\n", name, role, details);
    }

    fprintf(f, "\n--- Ruoli Unici ---\n");
    fprintf(f, "Cuoco: %s\n", (shm_blackboard->cook == -1) ? "Nessuno" : staff_info[shm_blackboard->cook].name);
    fprintf(f, "Cassiere: %s\n", (shm_blackboard->cashier == -1) ? "Nessuno" : staff_info[shm_blackboard->cashier].name);
    fprintf(f, "Lavapiatti: %s\n", (shm_blackboard->dishwasher == -1) ? "Nessuno" : staff_info[shm_blackboard->dishwasher].name);

    fprintf(f, "\n--- SHARED MEMORY: CASH DESK ---\n");
    fprintf(f, "Pending Payments: %d\n", shm_cashdesk->pending_payments);

    fprintf(f, "\n--- SHARED MEMORY: KITCHEN ---\n");
    fprintf(f, "Pending Orders: %d\n", shm_kitchen->pending_orders);
    fprintf(f, "Clean Plates: %d\n", shm_kitchen->clean_plates);
    fprintf(f, "Dirty Plates: %d\n", shm_kitchen->dirty_plates);
    fprintf(f, "Food Ready: ");
    for (int t = 0; t < shm_kitchen->tables_n; t++) {
        fprintf(f, "T%d:%s ", t, shm_kitchen->food_ready[t] ? "SI" : "NO");
    }
    fprintf(f, "\n");

    fprintf(f, "\n--- SHARED MEMORY: DINING ROOM ---\n");
    fprintf(f, "%-6s | %-10s | %-10s | %-10s | %-10s\n", "Tavolo", "Stato", "Famiglia", "Sporco", "Cibo Qty");
    fprintf(f, "---------------------------------------------------------\n");
    for (int t = 0; t < shm_diningroom->tables_n; t++) {
        const char *state_str = "Sconosciuto";
        switch (shm_diningroom->tables[t].state) {
            case TABLE_EMPTY: state_str = "EMPTY"; break;
            case TABLE_TAKEN: state_str = "TAKEN"; break;
            case TABLE_SERVED: state_str = "SERVED"; break;
            case TABLE_FREED: state_str = "FREED"; break;
        }
        fprintf(f, "%-6d | %-10s | %-10s | %-10d | %-10d\n",
                t, state_str,
                shm_diningroom->tables[t].surname,
                shm_diningroom->tables[t].dirt_level,
                shm_diningroom->tables[t].food_qty);
    }

    fclose(f);
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

        /* 3. Scrivi l'assegnazione sulla lavagna (sezione critica) */
        blackboard_lock();
        
        memcpy(snap.blackboard, shm_blackboard, sizeof(shm_blackboard_t));

        /* 4. Chiedi al modulo strategy il ruolo ottimale */
        role_t new_role = strategy_decide_role(sid, strat, &snap, staff_info, staff_n);
        
        clear_assignment(sid);
        write_assignment(sid, new_role);

        extern int g_print_info;
        if (g_print_info) {
            dump_info_to_file(staff_info, staff_n, strat);
        }

        blackboard_unlock();

        /* 5. Sleep per evitare di saturare la CPU */
        usleep(strat == STRATEGY_PROFIT ? PROFIT_POLL_INTERVAL_US : REPUTATION_POLL_INTERVAL_US);
    }

    /* Pulisci la propria assegnazione prima di uscire */
    blackboard_lock();
    clear_assignment(sid);
    blackboard_unlock();

    printf("[WORKER %d] Thread in uscita.\n", sid);
    return NULL;
}
