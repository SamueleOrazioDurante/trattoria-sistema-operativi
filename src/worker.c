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

// ---- Atomic flag: TR_TRUE while the current instance is running ----
static atomic_int g_instance_running = 0;

// ---- Polling interval (microseconds) ----
#define WORKER_POLL_INTERVAL_US  50000   /* 50 ms */

/* ---------- public helpers ------------------------------------------------ */

void worker_start_instance(void) {
    state_reset_fatigue();
    atomic_store(&g_instance_running, 1);
}

void worker_stop_instance(void) {
    atomic_store(&g_instance_running, 0);
}

/* ---------- internal helpers ---------------------------------------------- */

/**
 * @brief Drain all pending fatigue messages for this staff member (non-blocking).
 *        Updates g_staff_fatigue with the latest level received.
 */
static void drain_fatigue(int staff_id) {
    msg_fatigue_t fmsg;
    /*
     * mtype = staff_id + 1 (SysV requires mtype > 0).
     * IPC_NOWAIT so we never block — just consume what's available.
     */
    while (msgrcv(q_fatigue, &fmsg, sizeof(fmsg) - sizeof(long),
                  staff_id + 1, IPC_NOWAIT) != -1) {
        state_update_fatigue(staff_id, fmsg.role, fmsg.new_lvl);
    }
    /* ENOMSG / EAGAIN are expected when the queue is empty — ignore them. */
}

/**
 * @brief Remove any previous assignment of @p staff_id from the blackboard.
 *        Caller must hold the blackboard semaphore.
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
 * @brief Write the new role assignment for @p staff_id onto the blackboard.
 *        For per-table roles (WAITER / HELPER) we pick the best available table.
 *        Caller must hold the blackboard semaphore.
 */
static void write_assignment(int staff_id, role_t role) {
    switch (role) {

    case ROLE_COOK:
        shm_blackboard->cook = staff_id;
        break;

    case ROLE_CASHIER:
        shm_blackboard->cashier = staff_id;
        break;

    case ROLE_DISHWASHER:
        shm_blackboard->dishwasher = staff_id;
        break;

    case ROLE_WAITER: {
        /*
         * Priority: first try tables that have food ready to serve,
         * then tables that are TAKEN (need order-taking).
         */
        int best = -1;

        /* 1. Tables with food ready and no waiter assigned */
        for (int t = 0; t < shm_blackboard->tables_n; t++) {
            if (shm_blackboard->tables[t].waiter == -1 &&
                shm_kitchen->food_ready[t] == TR_TRUE) {
                best = t;
                break;
            }
        }
        /* 2. Fallback: tables in TAKEN state (waiting for order) */
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
        /* Pick the dirtiest freed table that has no cleaner yet. */
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
        /* Nothing to write — the staff member is resting. */
        break;
    }
}

/* ---------- semaphore lock / unlock --------------------------------------- */

static void blackboard_lock(void) {
    struct sembuf op = { .sem_num = SEMIDX_BLACKBOARD, .sem_op = -1, .sem_flg = 0 };
    while (semop(sem_id, &op, 1) == -1) {
        if (errno == EINTR) continue;   /* interrupted by signal — retry */
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

/* ---------- thread entry point -------------------------------------------- */

void *worker_thread(void *arg) {
    worker_args_t *wa = (worker_args_t *)arg;
    const int   sid      = wa->staff_id;
    const strategy_t strat = wa->strategy;
    const staff_member_t *staff_info = wa->staff_info;
    const int   staff_n  = wa->staff_n;

    printf("[WORKER %d] Thread started.\n", sid);

    while (atomic_load(&g_instance_running)) {

        /* 1. Drain pending fatigue messages (non-blocking) */
        drain_fatigue(sid);

        /* 2. Take a snapshot of the current state */
        snapshot_t snap;
        state_take_snapshot(&snap);

        /* 3. Ask the strategy module for the optimal role */
        role_t new_role = strategy_decide_role(sid, strat, &snap,
                                               staff_info, staff_n);

        /* 4. Write the assignment on the blackboard (critical section) */
        blackboard_lock();
        clear_assignment(sid);
        write_assignment(sid, new_role);
        blackboard_unlock();

        /* 5. Sleep to avoid saturating the CPU */
        usleep(WORKER_POLL_INTERVAL_US);
    }

    /* Clean up own assignment before exiting */
    blackboard_lock();
    clear_assignment(sid);
    blackboard_unlock();

    printf("[WORKER %d] Thread exiting.\n", sid);
    return NULL;
}
