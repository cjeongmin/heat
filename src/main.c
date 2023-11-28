#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>

#include "handler.h"
#include "options.h"

extern option* state;
FILE* logging_file;

void exec_action() {
    pid_t pid;
    char buffer[128];

    if ((pid = fork()) == 0) {
        if (state->script_path == NULL) {
            // 검사 명령 실행
            dup2(fileno(logging_file), STDOUT_FILENO);
            dup2(fileno(logging_file), STDERR_FILENO);

            sprintf(buffer, "/bin/%s", state->inspection_command[0]);
            if (execv(buffer, state->inspection_command) == -1) {
                time_t tt;
                time(&tt);

                fprintf(stderr, "[%d](%d): 검사 명령 실행에 실패했습니다.\n",
                        (unsigned int)tt, getpid());
                exit(1);
            }
        } else {
            // 스크립트 실행
        }
    }
}

int set_timer() {
    struct sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = exec_action;

    if (sigaction(SIGALRM, &action, (struct sigaction*)NULL) < 0) {
        fprintf(stderr, "알람 시그널 설정에 실패했습니다.\n");
        return 1;
    }

    struct itimerval timer;
    timer.it_value.tv_sec = state->interval;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = state->interval;
    timer.it_interval.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &timer, (struct itimerval*)NULL) == -1) {
        fprintf(stderr, "타이머 설정에 실패했습니다.\n");
        return 1;
    }

    return 0;
}

int set_signal_handler() {
    struct sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = sigchld_handler;

    if (sigaction(SIGCHLD, &action, (struct sigaction*)NULL) < 0) {
        fprintf(stderr, "자식 시그널 설정에 실패했습니다.\n");
        return 1;
    }
    return 0;
}

int main(int argc, char** argv) {
    state = parse_optarg(argc, argv);

    if (state == NULL) {
        exit(1);
    }
    state->ppid = getppid();

    // 로깅을 위한 파일 생성,
    logging_file = fopen("heat.log", "w");
    if (logging_file == NULL) {
        fprintf(stderr, "heat.log 파일을 생성하는데 실패했습니다.\n");
        exit(1);
    }

    set_signal_handler();
    if (state->interval > 0) {
        set_timer();
    }
    exec_action();

    while (1) {
    }

    fclose(logging_file);
    return 0;
}