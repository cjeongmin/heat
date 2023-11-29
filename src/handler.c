#include "handler.h"

extern FILE* logging_file;
extern State* state;
extern char** environ;

void exec_failure_script(pid_t fail_pid, int exit_code, unsigned int unixtime,
                         int interval) {
    pid_t pid;
    time_t tt;
    if ((pid = fork()) == 0) {
        char buffer[128];
        sprintf(buffer, "%d", exit_code);
        setenv("HEAT_FAIL_CODE", buffer, 1);
        sprintf(buffer, "%d", unixtime);
        setenv("HEAT_FAIL_TIME", buffer, 1);
        sprintf(buffer, "%d", interval);
        setenv("HEAT_FAIL_INTERVAL", buffer, 1);
        sprintf(buffer, "%d", fail_pid);
        setenv("HEAT_FAIL_PID", buffer, 1);

        if (execle(state->failure_script_path, state->failure_script_path, NULL,
                   environ) == -1) {
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
        sigdelset(&action.sa_mask, SIGQUIT);
        sigdelset(&action.sa_mask, SIGSTOP);
        sigsuspend(&action.sa_mask);
        waitpid(pid, NULL, 0);
    }
}

char* set_str_status(int si_code) {
    switch (si_code) {
    case CLD_CONTINUED:
        return "continued";
    case CLD_DUMPED:
        return "dumped";
    case CLD_EXITED:
        return "exited";
    case CLD_KILLED:
        return "killed";
    case CLD_STOPPED:
        return "stopped";
    case CLD_TRAPPED:
        return "trapped";
    }
    return "unknown";
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
    printf("Failed: Exit Code %d, details in heat.log\n", WEXITSTATUS(status));

    // write details in heat.log
    char* str_status = set_str_status(info->si_code);
    fprintf(logging_file, "[%d](%d): child %s(%d)\n", (unsigned int)tt,
            info->si_pid, str_status, WEXITSTATUS(status));
    fflush(logging_file);

    // send signal
    if (state->pid != 0 && state->signal != -1) {
        if (kill(state->pid, state->signal) == -1) {
            fprintf(stderr,
                    "시그널을 보낼 프로세스가 없습니다. (pid: %d, %d)\n",
                    state->pid, state->signal);
            return;
        }
    }

    // execute failiure script
    if (state->failure_script_path != NULL) {
        exec_failure_script(info->si_pid, WEXITSTATUS(status), (unsigned int)tt,
                            state->interval);
    }
}
