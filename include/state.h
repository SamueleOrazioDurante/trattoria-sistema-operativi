#ifndef STATE_H
#define STATE_H

#include "scenario.h"
#include "ipc.h"
#include "strategy.h"   /* snapshot_t */

/**
 * @brief Reset the internal fatigue tracking for all staff members.
 *        Must be called at the start of every new instance.
 */
void state_reset_fatigue(void);

/**
 * @brief Read all shared memories and build a snapshot_t.
 *
 * The snapshot contains pointers to the live SHM regions (read-only for the
 * client) plus a copy of the locally-tracked fatigue levels.
 *
 * @param[out] snap  Pointer to the snapshot structure to fill.
 */
void state_take_snapshot(snapshot_t *snap);

/**
 * @brief Update the internal fatigue tracking for a single staff member.
 *
 * Called when a fatigue notification is received from the server via the
 * FATIGUE message queue.
 *
 * @param staff_id  Index of the staff member (0-based).
 * @param role      The role whose perceived fatigue increased.
 * @param new_level The new fatigue level.
 */
void state_update_fatigue(int staff_id, role_t role, level_t new_level);

/**
 * @brief Get the current fatigue level for a staff member.
 *
 * @param staff_id  Index of the staff member (0-based).
 * @return The current fatigue level.
 */
level_t state_get_fatigue(int staff_id);

#endif /* STATE_H */
