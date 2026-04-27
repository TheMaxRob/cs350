#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>

#define BITMAP_BYTES (1 << 22)
#define PHONE_KEY 50302
#define MAX_NUM ( 1 << 25)
#define MSG_REGISTER 1
#define MSG_PERFECT 2 // perfect found
#define MAX_PROCESSES 20
#define MAX_PERFECT 20

typedef struct {
    pid_t pid;
    int found;
    int candidates;
    int skipped;
} ProcessRow;

typedef struct {
    char bitmap[BITMAP_BYTES];
    int perfects[20];
    ProcessRow processes[20];
    pid_t manage_pid;
    ProcessRow totals;
} SharedMemory;

typedef struct {
    long mtype;
    pid_t pid;
    int value;
} Message;