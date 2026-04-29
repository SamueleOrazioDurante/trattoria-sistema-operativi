#include "server_comm.h"
#include "scenario.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage(const char *progname) {
    printf("Usage: %s [--strategy <profit|reputation>]\n", progname);
}

int main(int argc, char *argv[]) {
    strategy_t chosen_strategy = STRATEGY_NONE;

    // Parsing arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--strategy") == 0) {
            if (i + 1 < argc) {
                if (strcmp(argv[i+1], "profit") == 0) {
                    chosen_strategy = STRATEGY_PROFIT;
                } else if (strcmp(argv[i+1], "reputation") == 0) {
                    chosen_strategy = STRATEGY_REPUTATION;
                } else {
                    fprintf(stderr, "Error: Unknown strategy '%s'\n", argv[i+1]);
                    print_usage(argv[0]);
                    return EXIT_FAILURE;
                }
                i++; // skip next arg
            } else {
                fprintf(stderr, "Error: --strategy requires an argument\n");
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
        } else {
            fprintf(stderr, "Error: Unknown argument '%s'\n", argv[i]);
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if(STRATEGY_NONE == chosen_strategy){
        printf("[CLIENT] No strategy chosen. Exiting.\n");
        return EXIT_FAILURE;
    }

    char matricole[STUDENTID_MAX][STUDENTID_MAXLEN] = {0};
    strncpy(matricole[0], "VR518042", STUDENTID_MAXLEN);
    strncpy(matricole[1], "VR519359", STUDENTID_MAXLEN);
    strncpy(matricole[2], "VR523996", STUDENTID_MAXLEN);

    printf("[CLIENT] Starting Trattoria Client...\n");

    // Initialize communication (connect to message queues)
    server_comm_init();

    // Protocol flow
    server_comm_hello(matricole, chosen_strategy);
    server_comm_wait_welcome();
    
    // Main loop
    server_comm_instance_loop();

    // Cleanup
    server_comm_cleanup();

    printf("[CLIENT] Exiting normally.\n");
    return EXIT_SUCCESS;
}