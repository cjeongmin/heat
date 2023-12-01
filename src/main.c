#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>

#include "handler.h"
#include "options.h"

extern int errno;
extern State* state;
extern char** environ;

int main(int argc, char** argv) {
    state = parse_optarg(argc, argv);

    if (state == NULL) {
        fprintf(stderr, "[ERROR](heat): 옵션 파싱 실패");
        exit(1);
    }

    int signo;
    siginfo_t info;
    sigset_t block_mask, sigset_mask;
    union sigval payload;

    struct timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = 100000;

    sigfillset(&block_mask);
    sigdelset(&block_mask, SIGQUIT);
    sigdelset(&block_mask, SIGSTOP);
    sigdelset(&block_mask, SIGTERM);
    sigdelset(&block_mask, SIGINT);
    sigprocmask(SIG_SETMASK, &block_mask, NULL);

    sigemptyset(&sigset_mask);

    int status;
    pid_t pid = fork();
    if (pid == -1) {
        perror("[ERROR](heat)");
        exit(1);
    } else if (pid == 0) {
        // HEAT RECEIVER
        prctl(PR_SET_NAME, "heat-receiver", NULL, NULL, NULL);

        sigaddset(&sigset_mask, SIGCHLD);
        sigaddset(&sigset_mask, SIGALRM);
        sigaddset(&sigset_mask, SIGUSR1);

        while (1) {
            if ((signo = sigtimedwait(&sigset_mask, &info, &timeout)) == -1) {
                if (errno == EAGAIN) {
                    continue;
                }
                perror("[ERROR](receiver)");
                exit(1);
            }

            if (signo == SIGCHLD) {

            } else if (signo == SIGALRM) {

            } else if (signo == SIGUSR1) {
                pid = fork();
                if (pid == -1) {
                    perror("[ERROR](receiver)");
                    exit(1);
                } else if (pid == 0) {
                    if (state->script_path != NULL) {
                        if (execle(state->script_path, state->script_path, NULL,
                                   environ) == -1) {
                            perror("[ERROR](wrapper)");
                            exit(1);
                        }
                    } else {
                        if (execvp(state->inspection_command[0],
                                   state->inspection_command) == -1) {
                            perror("[ERROR](wrapper)");
                            exit(1);
                        }
                    }
                }
            }

            waitpid(-1, NULL, WNOHANG);
        }

    } else {
        // HEAT MAIN
        sigaddset(&sigset_mask, SIGCHLD);
        sigaddset(&sigset_mask, SIGALRM);

        // interval 마다 SIGUSR1 알람을 통해 receiver에게 프로세스 생성을 명령
        struct itimerval timer;
        timer.it_value.tv_sec = state->interval;
        timer.it_value.tv_usec = 0;
        timer.it_interval.tv_sec = state->interval;
        timer.it_interval.tv_usec = 0;

        if (setitimer(ITIMER_REAL, &timer, (struct itimerval*)NULL) == -1) {
            perror("[ERROR](heat)");
            exit(1);
        }

        kill(pid, SIGUSR1);

        while (1) {
            if ((signo = sigtimedwait(&sigset_mask, &info, &timeout)) == -1) {
                if (errno == EAGAIN) {
                    continue;
                }
                perror("[ERROR](heat)");
                exit(1);
            }

            if (signo == SIGCHLD && info.si_pid == pid) {
                fprintf(
                    stderr,
                    "[ERROR](heat): 시그널 수신 프로세스가 종료되었습니다.\n");
            } else if (signo == SIGALRM) {
                payload.sival_int = 0;
                payload.sival_ptr = NULL;
                sigqueue(pid, SIGUSR1, payload);
            }

            if (waitpid(pid, &status, WNOHANG) == -1) {
                fprintf(stderr, "[ERROR](heat): 시그널 수신 프로세스에 문제가 "
                                "발생하여 종료합니다.\n");
                exit(1);
            }

            if (WIFSTOPPED(status)) {
                fprintf(
                    stderr,
                    "[ERROR](heat): 시그널 수신 프로세스가 중단되었습니다.\n");
                exit(1);
            }
        }
    }

    return 0;
}
