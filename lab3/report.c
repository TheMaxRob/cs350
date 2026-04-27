#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include "defs.h"

int main(int argc, char *argv[]) {
    int sid;
    int kill_flag = 0;
    SharedMemory *shm;

    if (argc == 2) {
        // -k flag
        if (strcmp(argv[1], "-k") == 0) {
            kill_flag = 1;
        } else {
            fprintf(stderr, "Usage: report [-k]\n");
            exit(1);
        }
    }

    // attach to memory
    if ((sid=shmget(PHONE_KEY,sizeof(SharedMemory),0660))== -1) {
        perror("shmget");
        exit(1);
    }

    if ((shm=((SharedMemory *) shmat(sid,0,0)))== (SharedMemory *)-1) {
        perror("shmat");
        exit(2);
    }

    // Perfect numbers line
    printf("Perfect Numbers Found: ");
    for (int i = 0; i < MAX_PERFECT; i++) {
        if (shm->perfects[i] > 0) {
            printf("%d ", shm->perfects[i]);
        }
    }
    printf("\n");

    // pids lines
    int perfects = 0;
    int candidates = 0;
    int skipped = 0;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (shm->processes[i].pid != 0) {
            printf("pid(%d): found: %d, tested: %d, skipped: %d\n", shm->processes[i].pid, shm->processes[i].found, shm->processes[i].candidates, shm->processes[i].skipped);
            perfects += shm->processes[i].found;
            candidates += shm->processes[i].candidates;
            skipped += shm->processes[i].skipped;
        }
    }

    // total stats lines
    printf("Total found: %d\n", perfects);
    printf("Total tested: %d\n", candidates);
    printf("Total skipped: %d\n", skipped);

    if (kill_flag) {
        if (shm->manage_pid > 0) {
            kill(shm->manage_pid, SIGINT);
        } else {
            fprintf(stderr, "Couldn't find manage pid\n");
        }
    }
    shmdt(shm);
    exit(0);
}