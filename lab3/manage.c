// Create IPC resources
// Signal handlers
// msgrcv loop
// PID reg
// find perfects
// shutdown handling
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <string.h>
#include "defs.h"

static SharedMemory *shm = NULL;
static int sid = -1;
static int semid = -1;
static int qid = -1;

void shutdown_handler(int sig) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (shm->processes[i].pid > 0)
            kill(shm->processes[i].pid, SIGINT);
    }
    sleep(5);
    shmdt(shm);
    shmctl(sid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    msgctl(qid, IPC_RMID, NULL);
    exit(0);
}


int main() {
        /* create shared segment if necessary */
        if ((sid=shmget(PHONE_KEY,sizeof(SharedMemory),IPC_CREAT |0660))== -1) {
                perror("shmget");
                exit(1);
        }

        shm = (SharedMemory *)shmat(sid, 0, 0);
        if (shm == (SharedMemory *)-1) {
            { perror("shmat"); exit(2); }
        }

        memset(shm, 0, sizeof(SharedMemory));
        shm->manage_pid = getpid();

        // Semaphore
        if ((semid = semget(PHONE_KEY, 1, IPC_CREAT |0660))==-1) {
            perror("semget");
            exit(1);
        }

        if ((qid=msgget(PHONE_KEY, IPC_CREAT |0660)) == -1) {
            perror("msgget");
            exit(1);
        }

        struct sigaction action;
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGINT);
        sigaddset(&mask, SIGQUIT);
        sigaddset(&mask, SIGHUP);
        action.sa_flags = 0;
        action.sa_mask = mask;
        action.sa_handler = shutdown_handler;
        sigaction(SIGINT,  &action, NULL);
        sigaction(SIGQUIT, &action, NULL);
        sigaction(SIGHUP,  &action, NULL);

        Message msg;
        while (1) {
            if (msgrcv(qid, &msg, sizeof(Message) - sizeof(long),0,0) < 0) {
                perror("msgrcv");
                continue;
            }

            if (msg.mtype == MSG_REGISTER) {
                // register new
                int row = -1;

                // Find free row
                for (int i = 0; i < MAX_PROCESSES; i++) {
                    if (shm->processes[i].pid == 0) { row = i; break; }
                }

                if (row == -1) {
                    fprintf(stderr,"Max processes reached\n");
                    continue;
                }

                shm->processes[row].pid = msg.pid;
                shm->processes[row].found = 0;
                shm->processes[row].candidates = 0;
                shm->processes[row].skipped = 0;
                
                struct sembuf up = { 0, 1, 0 };
                semop(semid, &up, 1);
            } else if (msg.mtype == MSG_PERFECT) {
                // perfect found, update perfects map, other already handled in compute.c
                int num = msg.value;
                for (int i = 0; i < MAX_PERFECT; i++) {
                    if (shm->perfects[i] == 0) {
                        shm->perfects[i] = num;
                        break;
                    }
                    // Ensure sorted
                    if (num < shm->perfects[i]) {
                        int temp = shm->perfects[i];
                        shm->perfects[i] = num;
                        num = temp;
                    }
                }
            }
        }

}
