#include "handler.h"

extern FILE* logging_file;
extern State* state;
extern char** environ;

void recovery_timeout_handler() { kill(getppid(), SIGUSR1); }

int exec_recovery_timeout_timer() {
    pid_t pid;
    time_t tt;
    time(&tt);

    if ((pid = fork()) > 0) {
        return pid;
    }

    if (pid < 0) {
        return 0;
    }

    prctl(PR_SET_NAME, "heat-timer", 0, 0, 0);

    struct sigaction action;
    sigfillset(&action.sa_mask);
    sigdelset(&action.sa_mask, SIGALRM);
    action.sa_flags = 0;
    action.sa_handler = recovery_timeout_handler;

    if (sigaction(SIGALRM, &action, (struct sigaction*)NULL) < 0) {
        fprintf(stderr, "복구 시간초과 알람 시그널 설정에 실패했습니다.\n");
        state->recovery_script_executed = 0;
        exit(1);
    }

    struct itimerval timer;
    timer.it_value.tv_sec = state->recovery_timeout;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &timer, (struct itimerval*)NULL) == -1) {
        fprintf(stderr, "복구 시간초과 타이머 설정에 실패했습니다.\n");
        state->recovery_script_executed = 0;
        exit(1);
    }

    sigsuspend(&action.sa_mask);
    exit(0);
}

void exec_recovery_script(FailState* fail_state) {
    pid_t pid;
    time_t tt;
    time(&tt);

    state->recovery_script_executed = 1;

    pid_t timer_pid = 0;
    if (state->recovery_timeout != 0) {
        timer_pid = exec_recovery_timeout_timer();
        state->recovery_timeout_timer_pid = timer_pid;
    }

    if ((pid = fork()) == 0) {

        char buffer[128];
        sprintf(buffer, "%d", fail_state->exit_code);
        setenv("HEAT_FAIL_CODE", buffer, 1);
        sprintf(buffer, "%d", fail_state->first_time);
        setenv("HEAT_FAIL_TIME", buffer, 1);
        sprintf(buffer, "%d", fail_state->last_time);
        setenv("HEAT_FAIL_TIME_LAST", buffer, 1);
        sprintf(buffer, "%d", fail_state->interval);
        setenv("HEAT_FAIL_INTERVAL", buffer, 1);
        sprintf(buffer, "%d", fail_state->pid);
        setenv("HEAT_FAIL_PID", buffer, 1);
        sprintf(buffer, "%d", fail_state->fail_cnt);
        setenv("HEAT_FAIL_CNT", buffer, 1);

        if (execle(state->recovery_script_path, state->recovery_script_path,
                   NULL, environ) == -1) {
            state->recovery_script_executed = 0;
            fprintf(stderr, "[%d](%d): 복구 스크립트 실행에 실패했습니다.\n",
                    (unsigned int)tt, getpid());

            if (timer_pid > 0) {
                kill(timer_pid, SIGKILL);
            }

            exit(1);
        }
    } else if (pid != -1) {
        state->recovery_script_pid = pid;
        struct sigaction action;
        sigfillset(&action.sa_mask);
        sigdelset(&action.sa_mask, SIGCHLD);
        sigdelset(&action.sa_mask, SIGINT);
        sigdelset(&action.sa_mask, SIGQUIT);
        sigdelset(&action.sa_mask, SIGSTOP);
        sigsuspend(&action.sa_mask);
        waitpid(pid, NULL, 0);
        kill(getpid(), SIGVTALRM);
        state->recovery_script_ended = 1;
    }
}

void exec_failure_script(FailState* fail_state) {
    pid_t pid;
    time_t tt;
    time(&tt);
    if ((pid = fork()) == 0) {
        char buffer[128];
        sprintf(buffer, "%d", fail_state->exit_code);
        setenv("HEAT_FAIL_CODE", buffer, 1);
        sprintf(buffer, "%d", fail_state->first_time);
        setenv("HEAT_FAIL_TIME", buffer, 1);
        sprintf(buffer, "%d", fail_state->interval);
        setenv("HEAT_FAIL_INTERVAL", buffer, 1);
        sprintf(buffer, "%d", fail_state->pid);
        setenv("HEAT_FAIL_PID", buffer, 1);

        if (execle(state->failure_script_path, state->failure_script_path, NULL,
                   environ) == -1) {
            fprintf(stderr, "[%d](%d): 실패 스크립트 실행에 실패했습니다.\n",
                    (unsigned int)tt, getpid());
            exit(1);
        }
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

void timeout_handler() {
    if (state->recovery_timeout_timer_pid > 0 &&
        state->recovery_script_executed == 1 &&
        state->recovery_script_ended == 1) {
        state->failure_count += 1;

        state->recovery_timeout_timer_pid = 0;
        signal(SIGKILL, SIG_IGN);
        kill(getpgid(getpid()) * -1, SIGKILL);
        signal(SIGKILL, SIG_DFL);
        exec_recovery_script(state->fail_state);
    }
}

void sigchld_handler(int signo, siginfo_t* info) {
    if (signo != SIGCHLD || info->si_pid == state->failure_script_pid) {
        return;
    }

    if (info->si_pid == state->recovery_script_pid) {
        if (state->recovery_timeout_timer_pid > 0) {
            kill(state->recovery_timeout_timer_pid, SIGKILL);
            state->recovery_timeout_timer_pid = 0;
        }
        return;
    }

    time_t tt;
    time(&tt);

    int status;
    while (waitpid(-1, &status, WNOHANG) > 0) {
    }

    if (info->si_pid == state->recovery_timeout_timer_pid) {
        return;
    }

    printf("[%d](%d): ", (unsigned int)tt, info->si_pid);
    if (status == 0) { // success
        if (state->recovery_script_executed == 1) {
            state->recovery_script_executed = 0;
        }

        state->failure_count = 0;
        printf("OK\n");
    } else { // failure
        state->failure_count += 1;

        state->fail_state->pid = info->si_pid;
        state->fail_state->exit_code = WEXITSTATUS(status);
        state->fail_state->interval = state->interval;
        state->fail_state->fail_cnt = state->failure_count;

        if (state->fail_state->first_time == 0) {
            state->fail_state->first_time = (unsigned int)tt;
        }
        state->fail_state->last_time = (unsigned int)tt;

        printf("Failed: Exit Code %d, details in heat.log\n",
               WEXITSTATUS(status));

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

        if (state->recovery_script_executed == 0) {
            if (state->failure_count >= state->threshold &&
                state->recovery_script_path != NULL) {
                exec_recovery_script(state->fail_state);
            } else if (state->failure_script_path != NULL) {
                // execute failure script
                exec_failure_script(state->fail_state);
            }
        } else if (state->recovery_script_executed == 1 &&
                   state->recovery_timeout == 0 &&
                   state->failure_count % state->threshold == 0) {
            printf("THRESHOLD\n");
            printf("THRESHOLD\n");
            printf("THRESHOLD\n");
            printf("THRESHOLD\n");
            printf("THRESHOLD\n");
            exec_recovery_script(state->fail_state);
        }
    }
}
