#ifndef STRATEGY_H
#define STRATEGY_H

#include "scenario.h"
#include "ipc.h"

/**
 * Istantanea (snapshot) dello stato attuale del ristorante.
 * Questa struttura aggrega lo stato delle memorie condivise e lo stato
 * tracciato localmente dal client (come la stanchezza).
 */
typedef struct {
    const shm_diningroom_t *diningroom;
    const shm_kitchen_t *kitchen;
    const shm_blackboard_t *blackboard;
    const shm_cashdesk_t *cashdesk;
    
    // Array dei livelli di stanchezza per ciascun membro dello staff, 
    // tracciati dal client tramite MSGTYPE_FATIGUE
    level_t staff_fatigue[MAX_STAFF];
} snapshot_t;

/**
 * Decide il ruolo ottimale per un membro dello staff in base alla strategia scelta.
 * 
 * @param staff_id L'ID del membro dello staff per cui decidere il ruolo.
 * @param strategy La strategia attuale (STRATEGY_PROFIT o STRATEGY_REPUTATION).
 * @param snapshot Lo stato attuale del ristorante e i livelli di stanchezza.
 * @param staff_info Array contenente i parametri (abilità e tratti) di tutti i membri dello staff.
 * @param staff_n Numero di membri dello staff.
 * @return Il ruolo ottimale (role_t) per il membro dello staff, o ROLE_NONE se riposa/attende.
 */
role_t strategy_decide_role(int staff_id, strategy_t strategy, const snapshot_t *snapshot, const staff_member_t *staff_info, int staff_n);

#endif // STRATEGY_H
