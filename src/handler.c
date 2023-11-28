#include "handler.h"

void alarm_handler() {
    pid_t ppid = getppid();
    kill(ppid, SIGUSR1);
}

void signal_handler() { printf("OK\n"); }