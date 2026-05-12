#include "server_comm.h"
#include "ipc_manager.h"
#include "worker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <errno.h>

static msg_welcome_t welcome_info;

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

    printf("[COMM] Inviato messaggio HELLO (Strategia: %d).\n", strategy);
}

void server_comm_wait_welcome() {
    if (msgrcv(q_s2c, &welcome_info, sizeof(welcome_info) - sizeof(long), MSGTYPE_WELCOME, 0) == -1) {
        perror("msgrcv WELCOME");
        exit(EXIT_FAILURE);
    }

    printf("[COMM] Ricevuto WELCOME. Gruppo: %s, Staff: %d, Tavoli: %d\n", 
           welcome_info.group, welcome_info.staff_n, welcome_info.tables_n);
    
    const char* bucket_names[] = {"LOW", "MEDIUM", "HIGH"};
    const char* skill_names[] = {"Cameriere", "Cuoco", "Aiutante", "Cassiere"};
    const char* trait_names[] = {"Pazienza", "Socievolezza", "Professionalità", "Resistenza"};

    for (int i = 0; i < welcome_info.staff_n; i++) {
        staff_member_t *s = &welcome_info.staff[i];
        printf("  Staff #%d: %s\n", i, s->name);
        
        printf("    Abilità: ");
        for (int j = 0; j < NUM_SKILLS; j++) {
            printf("%s=%s%s", skill_names[j], bucket_names[s->skills[j]], (j < NUM_SKILLS - 1) ? ", " : "");
        }
        printf("\n");

        printf("    Tratti: ");
        for (int j = 0; j < NUM_TRAITS; j++) {
            printf("%s=%s%s", trait_names[j], bucket_names[s->traits[j]], (j < NUM_TRAITS - 1) ? ", " : "");
        }
        printf("\n");
    }
    
    if (welcome_info.verify_mode) {
        printf("[COMM] Esecuzione in modalità VERIFY. Strategia imposta: %d\n", welcome_info.imposed_strategy);
    }
}

void server_comm_instance_loop() {
    while (1) {
        // Unione o buffer generico per ricevere diversi tipi di messaggi da S2C
        union {
            long mtype;
            msg_instance_t instance;
            msg_end_t end;
        } msg;

        printf("[COMM] In attesa del prossimo comando dal server...\n");

        if (msgrcv(q_s2c, &msg, sizeof(msg_instance_t) - sizeof(long), 0, 0) == -1) {
            perror("msgrcv instance/end");
            exit(EXIT_FAILURE);
        }

        if (msg.mtype == MSGTYPE_END) {
            msg_end_t *end = (msg_end_t *)&msg;
            printf("[COMM] Ricevuto messaggio END. Motivo: %d. Terminazione.\n", end->reason);
            break;
        }

        if (msg.mtype == MSGTYPE_INSTANCE) {
            msg_instance_t *inst = (msg_instance_t *)&msg;
            printf("[COMM] Avvio Istanza %d (Velocità: %d, Strategia: %d, Famiglie: %d)\n",
                   inst->instance_id, inst->speed, inst->strategy, inst->families_n);

            // --- Avvio dei thread worker ---
            pthread_t threads[MAX_STAFF];
            worker_args_t wargs[MAX_STAFF];

            worker_start_instance();

            for (int i = 0; i < welcome_info.staff_n; i++) {
                wargs[i].staff_id   = i;
                wargs[i].strategy   = inst->strategy;
                wargs[i].staff_info = welcome_info.staff;
                wargs[i].staff_n    = welcome_info.staff_n;

                if (pthread_create(&threads[i], NULL, worker_thread, &wargs[i]) != 0) {
                    perror("pthread_create worker");
                    exit(EXIT_FAILURE);
                }
            }

            // Attesa di INSTANCE_DONE dal server
            msg_instance_done_t done;
            if (msgrcv(q_s2c, &done, sizeof(done) - sizeof(long), MSGTYPE_INSTANCE_DONE, 0) == -1) {
                perror("msgrcv INSTANCE_DONE");
                exit(EXIT_FAILURE);
            }

            printf("[COMM] Istanza %d completata.\n", done.instance_id);
            printf("[COMM] Metriche - Tempo Totale: %.2f, Punteggio Medio: %s\n", 
                   done.total_families_time, done.average_families_score_review);

            // --- Ferma e attendi i thread worker ---
            worker_stop_instance();

            for (int i = 0; i < welcome_info.staff_n; i++) {
                pthread_join(threads[i], NULL);
            }

            // --- Resetta la lavagna (blackboard) per la prossima istanza ---
            struct sembuf lock  = { .sem_num = SEMIDX_BLACKBOARD, .sem_op = -1, .sem_flg = 0 };
            struct sembuf unlock = { .sem_num = SEMIDX_BLACKBOARD, .sem_op =  1, .sem_flg = 0 };
            semop(sem_id, &lock, 1);
            for (int t = 0; t < shm_blackboard->tables_n; t++) {
                shm_blackboard->tables[t].waiter  = -1;
                shm_blackboard->tables[t].cleaner = -1;
            }
            shm_blackboard->cook       = -1;
            shm_blackboard->cashier    = -1;
            shm_blackboard->dishwasher = -1;
            semop(sem_id, &unlock, 1);

            printf("[COMM] Thread worker terminati e lavagna resettata.\n");
        } else if (msg.mtype == MSGTYPE_ERROR) {
            msg_error_t *err = (msg_error_t *)&msg;
            fprintf(stderr, "[ERROR] Il server ha restituito l'errore %d: %s\n", err->code, err->message);
            exit(EXIT_FAILURE);
        }
    }
}
