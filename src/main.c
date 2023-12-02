#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>

#include "options.h"
#include "utils.h"

#define PR_WRAPPER "heat-wrapper"
#define PR_FAILURE "heat-failure"
#define PR_RECOVERY "heat-recovery"

extern int errno;
extern State* state;
extern char** environ;

void heat_main(int receiver_pid);
void heat_receiver();
void make_wrapper(char* type);

int main(int argc, char** argv) {
    state = parse_optarg(argc, argv);

    if (state == NULL) {
        fprintf(stderr, "[ERROR](heat): 옵션 파싱 실패");
        exit(1);
    }

    sigset_t block_mask;

    sigfillset(&block_mask);
    sigdelset(&block_mask, SIGQUIT);
    sigdelset(&block_mask, SIGSTOP);
    sigdelset(&block_mask, SIGTERM);
    sigdelset(&block_mask, SIGINT);
    sigprocmask(SIG_SETMASK, &block_mask, NULL);

    pid_t pid = fork();
    if (pid == -1) {
        perror("[ERROR](heat)");
        exit(1);
    } else if (pid == 0) {
        // HEAT RECEIVER
        heat_receiver();
    } else {
        // HEAT MAIN
        heat_main(pid);
    }

    return 0;
}

void make_wrapper(char* type) {
    pid_t receiver_pid = getppid();
    pid_t pid = fork();
    if (pid == -1) {
        perror("[ERROR](wrapper)");
        exit(1);
    } else if (pid == 0) {
        // INSPECTION PROCESS
        if (strcmp(type, PR_WRAPPER) == 0) {
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
        } else if (strcmp(type, PR_FAILURE) == 0) {
            if (state->failure_script_path != NULL) {
                if (execle(state->failure_script_path,
                           state->failure_script_path, NULL, environ) == -1) {
                    perror("[ERROR](wrapper)");
                    exit(1);
                }
            }
        }
    } else {
        // WRAPPER
        prctl(PR_SET_NAME, type, NULL, NULL, NULL);
        if (strcmp(type, PR_FAILURE) == 0) {
            union sigval payload;
            payload.sival_int = getpid();
            if (sigqueue(receiver_pid, SIGUSR2, payload) == -1) {
                perror("[ERROR](wrapper)");
                exit(2);
            }
        }

        int signo;
        int status;
        siginfo_t info;
        sigset_t sigset_mask;
        union sigval payload;

        struct timespec timeout;
        timeout.tv_sec = 0;
        timeout.tv_nsec = 100000; // 1ms

        sigemptyset(&sigset_mask);
        sigaddset(&sigset_mask, SIGCHLD);

        FILE* heat_log;
        if ((heat_log = fopen("heat.log", "a")) == NULL) {
            perror("[ERROR](wrapper)");
            exit(1);
        }

        while (1) {
            if ((signo = sigtimedwait(&sigset_mask, &info, &timeout)) == -1) {
                if (errno == EAGAIN) {
                    continue;
                }
                perror("[ERROR](wrapper)");
                fclose(heat_log);
                exit(1);
            }

            if (signo == SIGCHLD) {
                time_t tt;
                time(&tt);

                while (waitpid(pid, &status, WNOHANG) != -1) {
                }

                if (status == 0) {
                    if (strcmp(type, PR_FAILURE) != 0) {
                        printf("%d: OK\n", (unsigned int)tt);
                    }
                } else {
                    // write details in heat.log
                    char* str_status = set_str_status(info.si_code);
                    fprintf(heat_log, "[%d](%d): child %s(%d)\n",
                            (unsigned int)tt, info.si_pid, str_status,
                            WEXITSTATUS(status));
                    fflush(heat_log);
                    printf("%d: Failed: Exit Code %d, details in heat.log\n",
                           (unsigned int)tt, WEXITSTATUS(status));

                    if (state->pid != 0 && state->signal != -1) {
                        sigqueue(state->pid, state->signal, payload);
                    }

                    if (state->failure_script_path != NULL) {
                        // FAILURE
                        payload.sival_int = 1;
                        if (sigqueue(receiver_pid, SIGUSR2, payload) == -1) {
                            perror("[ERROR](wrapper)");
                            exit(2);
                        }
                    }
                }

                if (WIFEXITED(status)) {
                    break;
                }
            }
        }

        fclose(heat_log);
        exit(0);
    }
}

void heat_main(int receiver_pid) {
    int signo;
    int status;
    siginfo_t info;
    sigset_t sigset_mask;
    union sigval payload;

    struct timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = 100000; // 1ms

    sigemptyset(&sigset_mask);
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

    kill(receiver_pid, SIGUSR1);

    while (1) {
        if ((signo = sigtimedwait(&sigset_mask, &info, &timeout)) == -1) {
            if (errno == EAGAIN) {
                continue;
            }
            perror("[ERROR](heat)");
            exit(1);
        }

        if (signo == SIGCHLD && info.si_pid == receiver_pid) {
            fprintf(stderr,
                    "[ERROR](heat): 시그널 수신 프로세스가 종료되었습니다.\n");
        } else if (signo == SIGALRM) {
            sigqueue(receiver_pid, SIGUSR1, payload);
        }

        if (waitpid(receiver_pid, &status, WNOHANG) == -1) {
            fprintf(stderr, "[ERROR](heat): 시그널 수신 프로세스에 문제가 "
                            "발생하여 종료합니다.\n");
            exit(1);
        }

        if (WIFSTOPPED(status)) {
            fprintf(stderr,
                    "[ERROR](heat): 시그널 수신 프로세스가 중단되었습니다.\n");
            exit(1);
        }
    }
}

void heat_receiver() {
    int allow_new_wrapper = 0;
    int running_process_pid = 0;

    int signo;
    pid_t pid;
    siginfo_t info;
    sigset_t sigset_mask;

    struct timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = 100000; // 1ms

    prctl(PR_SET_NAME, "heat-receiver", NULL, NULL, NULL);

    sigemptyset(&sigset_mask);
    sigaddset(&sigset_mask, SIGCHLD);
    sigaddset(&sigset_mask, SIGALRM);
    sigaddset(&sigset_mask, SIGUSR1);
    sigaddset(&sigset_mask, SIGUSR2);

    while (1) {
        if ((signo = sigtimedwait(&sigset_mask, &info, &timeout)) == -1) {
            if (errno == EAGAIN) {
                continue;
            }
            perror("[ERROR](receiver)");
            exit(1);
        }

        if (signo == SIGCHLD) {
            waitpid(-1, NULL, WNOHANG);
            if (running_process_pid == info.si_pid) {
                running_process_pid = 0;
                allow_new_wrapper = 0;
            }
        } else if (signo == SIGALRM) {

        } else if (signo == SIGUSR1 && allow_new_wrapper == 0) {
            pid = fork();
            if (pid == -1) {
                perror("[ERROR](receiver)");
                exit(1);
            } else if (pid == 0) {
                make_wrapper(PR_WRAPPER);
            }
        } else if (signo == SIGUSR2) {
            int process_type = info.si_value.sival_int;

            if (process_type < 2 && allow_new_wrapper == 0) {
                allow_new_wrapper = 1;
                pid = fork();
                if (pid == -1) {
                    perror("[ERROR](receiver)");
                    allow_new_wrapper = 0;
                    exit(1);
                } else if (pid == 0) {
                    if (process_type == 1) {
                        // FAILURE SCRIPT
                        make_wrapper(PR_FAILURE);
                    } else if (process_type == 2) {
                        // RECOVERY SCRIPT
                        make_wrapper(PR_RECOVERY);
                    }
                }
            } else {
                running_process_pid = info.si_value.sival_int;
            }
        }
    }
}