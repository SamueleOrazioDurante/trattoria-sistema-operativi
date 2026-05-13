#include "state.h"
#include "ipc_manager.h"

#include <stdio.h>
#include <string.h>

/* ---------- Tracciamento interno della stanchezza ------------------------- */

/*
 * Ogni thread worker scrive solo nel proprio indice, quindi non è necessario
 * un mutex per l'accesso per elemento. L'array viene copiato in ogni snapshot.
 */
static level_t g_fatigue[MAX_STAFF];

/* ---------- API Pubblica ---------------------------------------------------- */

void state_reset_fatigue(void) {
    memset(g_fatigue, 0, sizeof(g_fatigue));
    printf("[STATE] Tracciamento stanchezza resettato.\n");
}

void state_take_snapshot(snapshot_t *snap) {
    /* Copia i dati dalle memorie condivise nello snapshot */
    snap->diningroom_data = *shm_diningroom;
    snap->kitchen_data    = *shm_kitchen;
    snap->blackboard_data = *shm_blackboard;
    snap->cashdesk_data   = *shm_cashdesk;

    /* Imposta i puntatori alle copie interne. */
    snap->diningroom = &snap->diningroom_data;
    snap->kitchen    = &snap->kitchen_data;
    snap->blackboard = &snap->blackboard_data;
    snap->cashdesk   = &snap->cashdesk_data;

    /* Copia i livelli di stanchezza tracciati localmente nello snapshot. */
    memcpy(snap->staff_fatigue, g_fatigue, sizeof(g_fatigue));
}

void state_update_fatigue(int staff_id, role_t role, level_t new_level) {
    if (staff_id < 0 || staff_id >= MAX_STAFF) {
        fprintf(stderr, "[STATE] WARNING: staff_id %d non valido nell'aggiornamento stanchezza\n",
                staff_id);
        return;
    }
    g_fatigue[staff_id] = new_level;
    printf("[STATE] Stanchezza Staff %d aggiornata: ruolo=%d, livello=%d\n",
           staff_id, role, new_level);
}

level_t state_get_fatigue(int staff_id) {
    if (staff_id < 0 || staff_id >= MAX_STAFF) {
        return LVL_NONE;
    }
    return g_fatigue[staff_id];
}
