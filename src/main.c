#define _POSIX_C_SOURCE 200809L
#include <sys/time.h>
#include <sys/wait.h>

#include "handler.h"
#include "options.h"

int main(int argc, char **argv, char **envp) {
    option *option = parse_optarg(argc, argv);

    struct sigaction sa;
    pid_t pid;

    switch ((pid = fork())) {
    case 0:;
        switch ((pid = fork())) {
        case 0:;
            char *process = malloc(sizeof(char) * 128);
            sprintf(process, "/bin/%s", argv[option->end_of_option]);
            if (execve(process, argv + option->end_of_option, envp) == -1) {
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
            it.it_value.tv_sec = option->interval;
            it.it_value.tv_usec = 0;
            it.it_interval.tv_sec = option->interval;
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