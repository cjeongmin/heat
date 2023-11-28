#include "handler.h"

extern FILE* logging_file;
extern option* state;

void sigchld_handler(int signo, siginfo_t* info) {
    if (signo != SIGCHLD) {
        return;
    }
    time_t tt;
    time(&tt);

    int status;
    while (waitpid(-1, &status, WNOHANG) > 0) {
    }

    printf("%d: ", (unsigned int)tt);
    if (status == 0) {
        printf("OK\n");
    } else {
        char* str_status = NULL;
        fprintf(logging_file, "[%d](%d): ", (unsigned int)tt, info->si_pid);
        switch (info->si_code) {
        case CLD_CONTINUED:
            str_status = "continued";
            break;
        case CLD_DUMPED:
            str_status = "dumped";
            break;
        case CLD_EXITED:
            str_status = "exited";
            break;
        case CLD_KILLED:
            str_status = "killed";
            break;
        case CLD_STOPPED:
            str_status = "stopped";
            break;
        case CLD_TRAPPED:
            break;
        default:
            str_status = "unknown";
        }
        fprintf(logging_file, "child %s(%d)\n", str_status,
                WEXITSTATUS(status));
        fflush(logging_file);
        printf("Failed: Exit Code %d, details in heat.log\n",
               WEXITSTATUS(status));

        if (kill(state->pid, state->signal) == -1) {
            fprintf(stderr,
                    "시그널을 보낼 프로세스가 없습니다. (pid: %d, %d)\n",
                    state->pid, state->signal);
            return;
        }
    }
}
