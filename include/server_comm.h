#ifndef SERVER_COMM_H
#define SERVER_COMM_H

#include "scenario.h"
#include "ipc.h"

/**
 * @brief Initialize the communication module (setup message queues, etc.)
 */
void server_comm_init();

/**
 * @brief Send the HELLO message to the server
 * @param strategy The strategy chosen by the user (or STRATEGY_NONE)
 */
void server_comm_hello(char matricole[STUDENTID_MAX][STUDENTID_MAXLEN], strategy_t strategy);

/**
 * @brief Wait for the WELCOME message and parse staff/table info
 */
void server_comm_wait_welcome();

/**
 * @brief Enter the main simulation loop (INSTANCE -> INSTANCE_DONE)
 */
void server_comm_instance_loop();

/**
 * @brief Clean up communication resources
 */
void server_comm_cleanup();

#endif // SERVER_COMM_H
