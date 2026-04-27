#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include "defs.h"

struct sembuf sb;

static SharedMemory *shm = NULL;
static int my_row = -1;

int isPerfect(int n) {
    if (n < 2) return 0;
    long sum = 1;
    for (int i = 2; i <= sqrt(n); i++) {
        if (n % i == 0) {
            sum += i;
            if (i != n / i)
                sum += n / i;
        }
    }
    return sum == n;
}

// n/8 should give us the byte and from there we can use %
int tested(int n) {
    return shm->bitmap[n / 8] & ( 1 << (n % 8));
}

// assign bitwise
void mark_tested(int n) {
    shm->bitmap[n/8] |= (1 << (n % 8));
}

// Signal
// if terminated, clear row and just end program immediately
void shutdown_handler(int sig) {
    if (my_row >= 0) {
        memset(&shm->processes[my_row], 0, sizeof(ProcessRow));
    }
    if (shm != NULL) {
        shmdt(shm);
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    int sid;
    int semid;
    int msgid;
 
    if (argc != 2) {
        fprintf(stderr, "Usage: compute [START]\n");
        exit(1);
    }

    int first = atoi(argv[1]);

    // get our shared memory
    if ((sid=shmget(PHONE_KEY,sizeof(SharedMemory),0660))== -1) {
        perror("shmget");
        exit(1);
    }

    if ((shm=((SharedMemory *) shmat(sid,0,0)))== (SharedMemory *)-1) {
        perror("shmat");
        exit(2);
    }

    // get semaphore
    if ((semid=semget(PHONE_KEY,2,0660))== -1) {
        perror("semget");
        exit(1);
    }

    // message queue
    if ((msgid=msgget(PHONE_KEY,0660))== -1) {
        perror("msgget");
        exit(1);
    }

    struct sigaction action;
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGALRM);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGHUP);
    action.sa_flags   = 0;
    action.sa_mask    = mask;
    action.sa_handler = shutdown_handler;
    sigaction(SIGINT,  &action, NULL);
    sigaction(SIGALRM, &action, NULL);
    sigaction(SIGQUIT, &action, NULL);
    sigaction(SIGHUP,  &action, NULL);

    Message msg;
    msg.mtype = MSG_REGISTER;
    msg.pid = getpid();
    msg.value = 0;

    // Register with manage
    if (msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0) < 0) {
        perror("msgsnd");
        exit(1);
    }

    // Block while we wait for our row
    // Found this nice pattern online
    struct sembuf down = { SEM_REGISTER, -1, 0 };
    if (semop(semid, &down, 1) < 0) { perror("semop"); exit(1); }

    // Find row
    pid_t my_pid = getpid();
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (shm->processes[i].pid == my_pid) {
            my_row = i;
            break;
        }
    }

    if (my_row < 0) {
	fprintf(stderr, "Could not find row, pid=%d\n", my_pid);
	exit(1);
    }


    struct sembuf lock = { SEM_MUTEX, -1, 0 };
    struct sembuf unlock = { SEM_MUTEX, 1, 0 };


    // set as unlock to prepare for loop
    semop(semid, &unlock, 1);

    int n = first;

    do {
        // lock
        semop(semid, &lock, 1);

        if (tested(n)) {
            semop(semid, &unlock, 1);
            shm->processes[my_row].skipped++;
        } else {
            mark_tested(n);
            semop(semid, &unlock, 1);

            // I don't think I need to lock for this, only bitmap?
            shm->processes[my_row].candidates++;

            // test for perfect
            if (isPerfect(n)) {
                // lock
                shm->processes[my_row].found++;

                // Create message to send
                Message pmsg;
                pmsg.mtype = MSG_PERFECT;
                pmsg.pid = my_pid;
                pmsg.value = n;
                msgsnd(msgid, &pmsg, sizeof(Message) - sizeof(long), 0); // excluding mtype
            }
        }

        // Wrap around
        n = (n+1) % MAX_NUM;
        if (n < 2) n = 2;
    } while ( n != first); // stop when we've reached original number

    execlp("report", "report", "-k", NULL);
    perror("execlp");
    exit(1);
}
