#ifndef WORKER_H
#define WORKER_H

#include "scenario.h"
#include "ipc.h"

/**
 * Argomenti passati a ciascun thread worker.
 */
typedef struct {
    int staff_id;
    strategy_t strategy;
    const staff_member_t *staff_info;  // Array di tutti i membri dello staff
    int staff_n;                       // Numero di membri dello staff
} worker_args_t;

/**
 * @brief Imposta il flag di esecuzione dell'istanza a TR_TRUE.
 *        Deve essere chiamato prima di creare i thread worker.
 */
void worker_start_instance(void);

/**
 * @brief Imposta il flag di esecuzione dell'istanza a TR_FALSE.
 *        Segnala a tutti i thread worker di uscire dal loro loop principale.
 */
void worker_stop_instance(void);

/**
 * @brief Entry point per ciascun thread worker (pthread).
 *
 * Il loop del worker:
 *   1. Legge la stanchezza dalla coda dei messaggi di stanchezza (msgrcv, non bloccante).
 *   2. Scatta una istantanea dello stato attuale del ristorante.
 *   3. Chiama strategy_decide_role() per scegliere il ruolo ottimale.
 *   4. Scrive l'assegnazione sulla lavagna (protetta da semaforo).
 *   5. Dorme brevemente per evitare la saturazione della CPU.
 *
 * Esce quando il flag di esecuzione dell'istanza viene azzerato.
 *
 * @param arg  Puntatore a una struttura worker_args_t.
 * @return NULL
 */
void *worker_thread(void *arg);

#endif // WORKER_H
