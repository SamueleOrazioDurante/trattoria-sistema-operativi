#include "server_comm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

static int q_c2s = -1;
static int q_s2c = -1;
static int q_fatigue = -1;

static msg_welcome_t welcome_info;

void server_comm_init() {
    key_t key;

    // Ensure the ftok path exists (minimal check/creation)
    FILE *f = fopen(TRATTORIA_FTOK_PATH, "a");
    if (f) fclose(f);

    key = ftok(TRATTORIA_FTOK_PATH, PROJ_MSG_C2S);
    if ((q_c2s = msgget(key, 0666)) == -1) {
        perror("msgget C2S");
        exit(EXIT_FAILURE);
    }

    key = ftok(TRATTORIA_FTOK_PATH, PROJ_MSG_S2C);
    if ((q_s2c = msgget(key, 0666)) == -1) {
        perror("msgget S2C");
        exit(EXIT_FAILURE);
    }

    key = ftok(TRATTORIA_FTOK_PATH, PROJ_MSG_FATIGUE);
    if ((q_fatigue = msgget(key, 0666)) == -1) {
        perror("msgget FATIGUE");
        exit(EXIT_FAILURE);
    }

    printf("[COMM] Connected to message queues.\n");
}

void server_comm_hello(char matricole[STUDENTID_MAX][STUDENTID_MAXLEN], strategy_t strategy) {
    msg_hello_t hello;
    memset(&hello, 0, sizeof(hello));

    hello.mtype = MSGTYPE_HELLO;
    hello.pid = getpid();
    
    hello.studentid_n = 0;
    for (int i = 0; i < STUDENTID_MAX; i++) {
        if (matricole[i][0] != '\0') {
            strncpy(hello.studentids[i], matricole[i], STUDENTID_MAXLEN);
            hello.studentid_n++;
        }
    }
    
    if (strategy != STRATEGY_NONE) {
        hello.has_strategy = TR_TRUE;
        hello.strategy = strategy;
    } else {
        hello.has_strategy = TR_FALSE;
    }

    if (msgsnd(q_c2s, &hello, sizeof(hello) - sizeof(long), 0) == -1) {
        perror("msgsnd HELLO");
        exit(EXIT_FAILURE);
    }

    printf("[COMM] Sent HELLO message (Strategy: %d).\n", strategy);
}

void server_comm_wait_welcome() {
    if (msgrcv(q_s2c, &welcome_info, sizeof(welcome_info) - sizeof(long), MSGTYPE_WELCOME, 0) == -1) {
        perror("msgrcv WELCOME");
        exit(EXIT_FAILURE);
    }

    printf("[COMM] Received WELCOME. Group: %s, Staff: %d, Tables: %d\n", 
           welcome_info.group, welcome_info.staff_n, welcome_info.tables_n);
    
    const char* bucket_names[] = {"LOW", "MEDIUM", "HIGH"};
    const char* skill_names[] = {"Waiter", "Cook", "Helper", "Cashier"};
    const char* trait_names[] = {"Patience", "Sociability", "Professionality", "Resistance"};

    for (int i = 0; i < welcome_info.staff_n; i++) {
        staff_member_t *s = &welcome_info.staff[i];
        printf("  Staff #%d: %s\n", i, s->name);
        
        printf("    Skills: ");
        for (int j = 0; j < NUM_SKILLS; j++) {
            printf("%s=%s%s", skill_names[j], bucket_names[s->skills[j]], (j < NUM_SKILLS - 1) ? ", " : "");
        }
        printf("\n");

        printf("    Traits: ");
        for (int j = 0; j < NUM_TRAITS; j++) {
            printf("%s=%s%s", trait_names[j], bucket_names[s->traits[j]], (j < NUM_TRAITS - 1) ? ", " : "");
        }
        printf("\n");
    }
    
    if (welcome_info.verify_mode) {
        printf("[COMM] Running in VERIFY mode. Strategy imposed: %d\n", welcome_info.imposed_strategy);
    }
}

void server_comm_instance_loop() {
    while (1) {
        // We need a union or a generic buffer to receive different message types from S2C
        union {
            long mtype;
            msg_instance_t instance;
            msg_end_t end;
        } msg;

        printf("[COMM] Waiting for next command from server...\n");

        if (msgrcv(q_s2c, &msg, sizeof(msg_instance_t) - sizeof(long), 0, 0) == -1) {
            perror("msgrcv instance/end");
            exit(EXIT_FAILURE);
        }

        if (msg.mtype == MSGTYPE_END) {
            msg_end_t *end = (msg_end_t *)&msg;
            printf("[COMM] Received END message. Reason: %d. Terminating.\n", end->reason);
            break;
        }

        if (msg.mtype == MSGTYPE_INSTANCE) {
            msg_instance_t *inst = (msg_instance_t *)&msg;
            printf("[COMM] Starting Instance %d (Speed: %d, Strategy: %d, Families: %d)\n",
                   inst->instance_id, inst->speed, inst->strategy, inst->families_n);

            // [DEBUG] Initializing workers for each staff member
            for (int i = 0; i < welcome_info.staff_n; i++) {
                printf("[DEBUG] Initializing worker thread for staff member %d (%s)\n", 
                       i, welcome_info.staff[i].name);
            }

            // Wait for INSTANCE_DONE
            msg_instance_done_t done;
            if (msgrcv(q_s2c, &done, sizeof(done) - sizeof(long), MSGTYPE_INSTANCE_DONE, 0) == -1) {
                perror("msgrcv INSTANCE_DONE");
                exit(EXIT_FAILURE);
            }

            printf("[COMM] Instance %d completed.\n", done.instance_id);
            printf("[COMM] Metrics - Total Time: %.2f, Avg Score: %s\n", 
                   done.total_families_time, done.average_families_score_review);
            
            printf("[DEBUG] Joining worker threads and resetting blackboard.\n");
        } else if (msg.mtype == MSGTYPE_ERROR) {
            msg_error_t *err = (msg_error_t *)&msg;
            fprintf(stderr, "[ERROR] Server returned error %d: %s\n", err->code, err->message);
            exit(EXIT_FAILURE);
        }
    }
}

void server_comm_cleanup() {
    printf("[COMM] Cleaning up communication module.\n");
}
