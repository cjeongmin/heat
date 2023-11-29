#include "handler.h"

extern FILE* logging_file;
extern State* state;

void exec_failure_script() {
    pid_t pid;
    time_t tt;
    if ((pid = fork()) == 0) {
        if (execl(state->failure_script_path, state->failure_script_path,
                  NULL) == -1) {
            time(&tt);
            fprintf(stderr, "[%d](%d): 실패 스크립트 실행에 실패했습니다.\n",
                    (unsigned int)tt, getpid());
        }
        exit(1);
    } else {
        state->failure_script_pid = pid;
        struct sigaction action;
        sigfillset(&action.sa_mask);
        sigdelset(&action.sa_mask, SIGCHLD);
        sigdelset(&action.sa_mask, SIGINT);
        sigsuspend(&action.sa_mask);
    }
}

void sigchld_handler(int signo, siginfo_t* info) {
    if (signo != SIGCHLD || info->si_pid == state->failure_script_pid) {
        return;
    }
    time_t tt;
    time(&tt);

    int status;
    while (waitpid(-1, &status, WNOHANG) > 0) {
    }

    printf("[%d](%d): ", (unsigned int)tt, info->si_pid);

    if (status == 0) { // success
        printf("OK\n");
        return;
    }
    // failure
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
    fprintf(logging_file, "child %s(%d)\n", str_status, WEXITSTATUS(status));
    fflush(logging_file);
    printf("Failed: Exit Code %d, details in heat.log\n", WEXITSTATUS(status));

    if (state->pid != 0 && state->signal != -1) {
        if (kill(state->pid, state->signal) == -1) {
            fprintf(stderr,
                    "시그널을 보낼 프로세스가 없습니다. (pid: %d, %d)\n",
                    state->pid, state->signal);
            return;
        }
    }

    if (state->failure_script_path != NULL) {
        exec_failure_script();
    }
}
