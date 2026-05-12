#ifndef WORKER_H
#define WORKER_H

#include "scenario.h"
#include "ipc.h"

/**
 * Arguments passed to each worker thread.
 */
typedef struct {
    int staff_id;
    strategy_t strategy;
    const staff_member_t *staff_info;  // Array of all staff members
    int staff_n;                       // Number of staff members
} worker_args_t;

/**
 * @brief Set the instance-running flag to TR_TRUE.
 *        Must be called before creating worker threads.
 */
void worker_start_instance(void);

/**
 * @brief Set the instance-running flag to TR_FALSE.
 *        Signals all worker threads to exit their main loop.
 */
void worker_stop_instance(void);

/**
 * @brief Entry point for each worker pthread.
 *
 * The worker loop:
 *   1. Reads fatigue from the fatigue message queue (msgrcv, non-blocking).
 *   2. Takes a snapshot of the current restaurant state.
 *   3. Calls strategy_decide_role() to pick the optimal role.
 *   4. Writes the assignment to the blackboard (protected by semaphore).
 *   5. Sleeps briefly to avoid CPU saturation.
 *
 * Exits when the instance-running flag is cleared.
 *
 * @param arg  Pointer to a worker_args_t struct.
 * @return NULL
 */
void *worker_thread(void *arg);

#endif // WORKER_H
