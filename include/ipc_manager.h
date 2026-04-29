#ifndef IPC_MANAGER_H
#define IPC_MANAGER_H

#include "ipc.h"

// Global pointers to shared memories
extern shm_diningroom_t *shm_diningroom;
extern shm_kitchen_t    *shm_kitchen;
extern shm_blackboard_t *shm_blackboard;
extern shm_cashdesk_t   *shm_cashdesk;

// Global IDs for IPC resources
extern int q_c2s;
extern int q_s2c;
extern int q_fatigue;
extern int sem_id;

/**
 * @brief Initialize all IPC resources (attach SHM, get semaphores and queues)
 */
void ipc_init();

/**
 * @brief Cleanup IPC resources (detach SHM)
 */
void ipc_cleanup();

#endif // IPC_MANAGER_H
