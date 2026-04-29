#ifndef STRATEGY_H
#define STRATEGY_H

#include "scenario.h"
#include "ipc.h"

/**
 * Snapshot of the current state of the restaurant.
 * This structure aggregates the state from the shared memories and 
 * the local client-tracked state (like fatigue).
 */
typedef struct {
    const shm_diningroom_t *diningroom;
    const shm_kitchen_t *kitchen;
    const shm_blackboard_t *blackboard;
    const shm_cashdesk_t *cashdesk;
    
    // Array of fatigue levels for each staff member, tracked by the client via MSGTYPE_FATIGUE
    level_t staff_fatigue[MAX_STAFF];
} snapshot_t;

/**
 * Decides the optimal role for a staff member based on the chosen strategy.
 * 
 * @param staff_id The ID of the staff member to decide the role for.
 * @param strategy The current strategy (STRATEGY_PROFIT or STRATEGY_REPUTATION).
 * @param snapshot The current state of the restaurant and fatigue levels.
 * @param staff_info Array containing the parameters (skills and traits) of all staff members.
 * @param staff_n Number of staff members.
 * @return The optimal role_t for the staff member, or ROLE_NONE if resting/waiting.
 */
role_t strategy_decide_role(int staff_id, strategy_t strategy, const snapshot_t *snapshot, const staff_member_t *staff_info, int staff_n);

#endif // STRATEGY_H
