#ifndef IPC_MANAGER_H
#define IPC_MANAGER_H

#include "ipc.h"

// Puntatori globali alle memorie condivise
extern shm_diningroom_t *shm_diningroom;
extern shm_kitchen_t    *shm_kitchen;
extern shm_blackboard_t *shm_blackboard;
extern shm_cashdesk_t   *shm_cashdesk;

// ID globali per le risorse IPC
extern int q_c2s;
extern int q_s2c;
extern int q_fatigue;
extern int sem_id;

/**
 * @brief Inizializza tutte le risorse IPC (collega SHM, ottiene semafori e code)
 */
void ipc_init();

/**
 * @brief Pulisce le risorse IPC (scollega SHM)
 */
void ipc_cleanup();

#endif // IPC_MANAGER_H
