#ifndef SERVER_COMM_H
#define SERVER_COMM_H

#include "scenario.h"
#include "ipc.h"

/**
 * @brief Inizializza il modulo di comunicazione (imposta le code di messaggi, ecc.)
 */
void server_comm_init();

/**
 * @brief Invia il messaggio HELLO al server
 * @param strategy La strategia scelta dall'utente (o STRATEGY_NONE)
 */
void server_comm_hello(char matricole[STUDENTID_MAX][STUDENTID_MAXLEN], strategy_t strategy);

/**
 * @brief Attende il messaggio WELCOME e analizza le informazioni sullo staff/tavoli
 */
void server_comm_wait_welcome();

/**
 * @brief Entra nel loop principale della simulazione (INSTANCE -> INSTANCE_DONE)
 */
void server_comm_instance_loop();

/**
 * @brief Pulisce le risorse di comunicazione
 */
void server_comm_cleanup();

#endif // SERVER_COMM_H
