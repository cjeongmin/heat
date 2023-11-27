#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "options.h"

int find_end_of_option(int, char **);
char *concat(int, char **);
void parse_optarg(int, char **);
void alarm_handler();
void signal_handler();

int options;
int interval = 1;
char *target = NULL;
char *script_path = NULL;

int main(int argc, char **argv, char **envp) {
    parse_optarg(argc, argv);

    struct sigaction sa;
    pid_t pid;

    switch ((pid = fork())) {
    case 0:;
        switch ((pid = fork())) {
        case 0:;
            char *process = malloc(sizeof(char) * 128);
            sprintf(process, "/bin/%s", argv[options]);
            if (execve(process, argv + options, envp) == -1) {
                fprintf(stderr, "검사 명령 실행에 실패했습니다.\n");
                exit(1);
            }
            break;
        default:;
            sigemptyset(&sa.sa_mask);
            sigaddset(&sa.sa_mask, SIGALRM);
            sa.sa_flags = 0;
            sa.sa_handler = alarm_handler;

            if (sigaction(SIGALRM, &sa, (struct sigaction *)NULL) < 0) {
                fprintf(stderr, "알람 시그널 설정에 실패했습니다.\n");
                exit(1);
            }

            struct itimerval it;
            it.it_value.tv_sec = interval;
            it.it_value.tv_usec = 0;
            it.it_interval.tv_sec = interval;
            it.it_interval.tv_usec = 0;

            if (setitimer(ITIMER_REAL, &it, (struct itimerval *)NULL) == -1) {
                fprintf(stderr, "타이머 설정에 실패했습니다.\n");
                exit(1);
            }

            while (waitpid(pid, NULL, WNOHANG) == 0) {
            }

            break;
        }
        break;
    default:;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sa.sa_handler = signal_handler;

        if (sigaction(SIGUSR1, &sa, (struct sigaction *)NULL) < 0) {
            fprintf(stderr, "HEAT 프로그램의 시그널 설정에 실패했습니다.\n");
            exit(1);
        }

        while (waitpid(pid, NULL, WNOHANG) == 0) {
        }

        break;
    }

    return 0;
}

int find_end_of_option(int argc, char **argv) {
    for (int i = 2; i < argc; i++) {
        if (argv[i - 1][0] != '-' && argv[i][0] != '-') {
            return i;
        }
    }
    return argc;
}

char *concat(int size, char **arr) {
    int length = size - 1;
    for (int i = 0; i < size; i++) {
        length += strlen(arr[i]);
    }

    int p = 0;
    char *res = malloc(sizeof(char) * length + 1);
    for (int i = 0; i < size; i++) {
        for (size_t j = 0; j < strlen(arr[i]); j++) {
            res[p++] = arr[i][j];
        }
        res[p++] = ' ';
    }
    res[length] = '\0';
    return res;
}

void parse_optarg(int argc, char **argv) {
    extern char *optarg;
    extern int optind, opterr, optopt;

    int n;
    options = find_end_of_option(argc, argv);
    while ((n = getopt_long(options, argv, "i:s:", long_options, NULL)) != -1) {
        switch (n) {
        case INTERVAL:
            interval = atoi(optarg);
            if (interval == 0) {
                // 숫자로 변환에 실패하거나 0인 경우
                fprintf(stderr,
                        "Interval의 값은 0보다 큰 숫자이여야 합니다.\n");
                exit(1);
            }
            break;
        case SCRIPT:
            script_path = optarg;
            break;
        }
    }

    if (options != argc) {
        target = concat(argc - options, argv + options);
    }
}

void alarm_handler() {
    pid_t ppid = getppid();
    printf("SEND SIGNAL FROM %d TO %d\n", getpid(), ppid);
    kill(ppid, SIGUSR1);
}

void signal_handler() { printf("OK\n"); }