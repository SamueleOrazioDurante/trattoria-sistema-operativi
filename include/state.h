#ifndef STATE_H
#define STATE_H

#include "scenario.h"
#include "ipc.h"
#include "strategy.h"   /* snapshot_t */

/**
 * @brief Resetta il tracciamento interno della stanchezza per tutti i membri dello staff.
 *        Deve essere chiamato all'inizio di ogni nuova istanza.
 */
void state_reset_fatigue(void);

/**
 * @brief Legge tutte le memorie condivise e costruisce uno snapshot_t.
 *
 * Lo snapshot contiene puntatori alle regioni SHM live (sola lettura per il
 * client) più una copia dei livelli di stanchezza tracciati localmente.
 *
 * @param[out] snap  Puntatore alla struttura snapshot da riempire.
 */
void state_take_snapshot(snapshot_t *snap);

/**
 * @brief Aggiorna il tracciamento interno della stanchezza per un singolo membro dello staff.
 *
 * Chiamato quando viene ricevuta una notifica di stanchezza dal server tramite la
 * coda dei messaggi FATIGUE.
 *
 * @param staff_id  Indice del membro dello staff (basato su 0).
 * @param role      Il ruolo la cui stanchezza percepita è aumentata.
 * @param new_level Il nuovo livello di stanchezza.
 */
void state_update_fatigue(int staff_id, role_t role, level_t new_level);

/**
 * @brief Ottiene il livello di stanchezza attuale per un membro dello staff.
 *
 * @param staff_id  Indice del membro dello staff (basato su 0).
 * @return Il livello di stanchezza attuale.
 */
level_t state_get_fatigue(int staff_id);

#endif /* STATE_H */
