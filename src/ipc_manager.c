#include "ipc_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <errno.h>

// Initialize global pointers
shm_diningroom_t *shm_diningroom = NULL;
shm_kitchen_t    *shm_kitchen = NULL;
shm_blackboard_t *shm_blackboard = NULL;
shm_cashdesk_t   *shm_cashdesk = NULL;

int q_c2s = -1;
int q_s2c = -1;
int q_fatigue = -1;
int sem_id = -1;

static int shm_diningroom_id = -1;
static int shm_kitchen_id = -1;
static int shm_blackboard_id = -1;
static int shm_cashdesk_id = -1;

void ipc_init() {
    key_t key;

    // -- MESSAGE QUEUES
    key = ftok(TRATTORIA_FTOK_PATH, PROJ_MSG_C2S);
    if ((q_c2s = msgget(key, 0666)) == -1) { perror("msgget C2S"); exit(EXIT_FAILURE); }

    key = ftok(TRATTORIA_FTOK_PATH, PROJ_MSG_S2C);
    if ((q_s2c = msgget(key, 0666)) == -1) { perror("msgget S2C"); exit(EXIT_FAILURE); }

    key = ftok(TRATTORIA_FTOK_PATH, PROJ_MSG_FATIGUE);
    if ((q_fatigue = msgget(key, 0666)) == -1) { perror("msgget FATIGUE"); exit(EXIT_FAILURE); }

    // -- SEMAPHORES
    key = ftok(TRATTORIA_FTOK_PATH, PROJ_SEM);
    if ((sem_id = semget(key, SEM_NSEMS, 0666)) == -1) { perror("semget"); exit(EXIT_FAILURE); }

    // -- SHARED MEMORIES
    // Dining Room
    key = ftok(TRATTORIA_FTOK_PATH, PROJ_DININGROOM);
    if ((shm_diningroom_id = shmget(key, sizeof(shm_diningroom_t), 0666)) == -1) { perror("shmget DiningRoom"); exit(EXIT_FAILURE); }
    if ((shm_diningroom = shmat(shm_diningroom_id, NULL, 0)) == (void*)-1) { perror("shmat DiningRoom"); exit(EXIT_FAILURE); }

    // Kitchen
    key = ftok(TRATTORIA_FTOK_PATH, PROJ_KITCHEN);
    if ((shm_kitchen_id = shmget(key, sizeof(shm_kitchen_t), 0666)) == -1) { perror("shmget Kitchen"); exit(EXIT_FAILURE); }
    if ((shm_kitchen = shmat(shm_kitchen_id, NULL, 0)) == (void*)-1) { perror("shmat Kitchen"); exit(EXIT_FAILURE); }

    // Blackboard
    key = ftok(TRATTORIA_FTOK_PATH, PROJ_BLACKBOARD);
    if ((shm_blackboard_id = shmget(key, sizeof(shm_blackboard_t), 0666)) == -1) { perror("shmget Blackboard"); exit(EXIT_FAILURE); }
    if ((shm_blackboard = shmat(shm_blackboard_id, NULL, 0)) == (void*)-1) { perror("shmat Blackboard"); exit(EXIT_FAILURE); }

    // Cash Desk
    key = ftok(TRATTORIA_FTOK_PATH, PROJ_CASHDESK);
    if ((shm_cashdesk_id = shmget(key, sizeof(shm_cashdesk_t), 0666)) == -1) { perror("shmget CashDesk"); exit(EXIT_FAILURE); }
    if ((shm_cashdesk = shmat(shm_cashdesk_id, NULL, 0)) == (void*)-1) { perror("shmat CashDesk"); exit(EXIT_FAILURE); }

    printf("[IPC] All resources initialized and attached.\n");
}

void ipc_cleanup() {
    if (shm_diningroom)  shmdt(shm_diningroom);
    if (shm_kitchen)     shmdt(shm_kitchen);
    if (shm_blackboard)  shmdt(shm_blackboard);
    if (shm_cashdesk)    shmdt(shm_cashdesk);
    
    printf("[IPC] All shared memories detached.\n");
}
