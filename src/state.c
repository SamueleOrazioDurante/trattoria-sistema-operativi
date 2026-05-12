#include "state.h"
#include "ipc_manager.h"

#include <stdio.h>
#include <string.h>

/* ---------- Internal fatigue tracking ------------------------------------- */

/*
 * Each worker thread writes only to its own index, so no mutex is needed
 * for per-element access.  The array is copied into every snapshot.
 */
static level_t g_fatigue[MAX_STAFF];

/* ---------- Public API ---------------------------------------------------- */

void state_reset_fatigue(void) {
    memset(g_fatigue, 0, sizeof(g_fatigue));
    printf("[STATE] Fatigue tracking reset.\n");
}

void state_take_snapshot(snapshot_t *snap) {
    /* Point directly to the live SHM regions (read-only for the client). */
    snap->diningroom = shm_diningroom;
    snap->kitchen    = shm_kitchen;
    snap->blackboard = shm_blackboard;
    snap->cashdesk   = shm_cashdesk;

    /* Copy the locally-tracked fatigue levels into the snapshot. */
    memcpy(snap->staff_fatigue, g_fatigue, sizeof(g_fatigue));
}

void state_update_fatigue(int staff_id, role_t role, level_t new_level) {
    if (staff_id < 0 || staff_id >= MAX_STAFF) {
        fprintf(stderr, "[STATE] WARNING: invalid staff_id %d in fatigue update\n",
                staff_id);
        return;
    }
    g_fatigue[staff_id] = new_level;
    printf("[STATE] Staff %d fatigue updated: role=%d, level=%d\n",
           staff_id, role, new_level);
}

level_t state_get_fatigue(int staff_id) {
    if (staff_id < 0 || staff_id >= MAX_STAFF) {
        return LVL_NONE;
    }
    return g_fatigue[staff_id];
}
