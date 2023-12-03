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
extern HeatOption* option;
extern char** environ;

void heat_main(int receiver_pid);
void heat_receiver();
void make_wrapper(char* type);

int main(int argc, char** argv) {
    option = parse_optarg(argc, argv);

    if (option == NULL) {
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
    pid_t pid = fork();
    if (pid == -1) {
        perror("[ERROR](wrapper)");
        exit(1);
    } else if (pid == 0) {
        // INSPECTION PROCESS
        if (strcmp(type, PR_WRAPPER) == 0) {
            if (option->script_path != NULL) {
                if (execle(option->script_path, option->script_path, NULL,
                           environ) == -1) {
                    perror("[ERROR](wrapper)");
                    exit(1);
                }
            } else {
                if (execvp(option->inspection_command[0],
                           option->inspection_command) == -1) {
                    perror("[ERROR](wrapper)");
                    exit(1);
                }
            }
        } else if (strcmp(type, PR_FAILURE) == 0) {
            if (option->failure_script_path != NULL) {
                if (execle(option->failure_script_path,
                           option->failure_script_path, NULL, environ) == -1) {
                    perror("[ERROR](wrapper)");
                    exit(1);
                }
            }
        } else if (strcmp(type, PR_RECOVERY) == 0) {
            if (option->recovery_script_path != NULL) {
                if (execle(option->recovery_script_path,
                           option->recovery_script_path, NULL, environ) == -1) {
                    perror("[ERROR](wrapper)");
                    exit(1);
                }
            }
        }
    } else {
        // WRAPPER
        prctl(PR_SET_NAME, type, NULL, NULL, NULL);

        int signo;
        int status;
        siginfo_t info;
        sigset_t sigset_mask;

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

                waitpid(pid, &status, 0);

                if (status == 0) {
                    if (strcmp(type, PR_FAILURE) != 0 &&
                        strcmp(type, PR_RECOVERY) != 0) {
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
                }

                if (WIFEXITED(status)) {
                    break;
                }
            }
        }

        fclose(heat_log);
        exit(WEXITSTATUS(status));
    }
}

void heat_receiver() {
    int allow_new_wrapper = 0;
    pid_t failure_wrapper_pid = 0;
    pid_t recovery_wrapper_pid = 0;

    int signo;
    pid_t pid;
    siginfo_t info;
    sigset_t sigset_mask;

    struct timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = 100000; // 1ms

    unsigned int recovery_cnt = 0;
    unsigned int failure_cnt = 0;
    time_t first_failure_time = 0;
    time_t latest_failure_time = 0;

    prctl(PR_SET_NAME, "heat-receiver", NULL, NULL, NULL);

    sigemptyset(&sigset_mask);
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
            waitpid(-1, NULL, 0);

            if (strcmp(set_str_status(info.si_code), "stopped") == 0) {
                if (info.si_pid == recovery_wrapper_pid) {
                    fprintf(stderr, "[ERROR](receiver) 복구 프로세스가 "
                                    "중단되어 종료합니다.\n");
                } else {
                    fprintf(stderr,
                            "[ERROR](receiver) 검사 명령어 또는 프로세스가 "
                            "중단되어 종료합니다.\n");
                }
                kill(getpgid(getpid()) * -1, SIGKILL);
                exit(1);
            }

            if (info.si_status == 0) {
                if (info.si_pid != failure_wrapper_pid &&
                    info.si_pid != recovery_wrapper_pid) {
                    if (recovery_wrapper_pid != 0) {
                        kill(option->pid, option->success_signal);
                    }
                    failure_cnt = 0;
                    failure_wrapper_pid = 0;
                    recovery_wrapper_pid = 0;
                } else if (info.si_pid == recovery_wrapper_pid) {
                    if (info.si_status == 0) {
                        if (option->recovery_timeout != 0) {
                            // SET RECOVERY TIMEOUT TIMER
                            struct itimerval timeout_timer;
                            timeout_timer.it_value.tv_sec =
                                option->recovery_timeout;
                            timeout_timer.it_value.tv_usec = 0;
                            timeout_timer.it_interval.tv_sec = 0;
                            timeout_timer.it_interval.tv_usec = 0;

                            if (setitimer(ITIMER_REAL, &timeout_timer, NULL) ==
                                -1) {
                                perror("[ERROR](receiver)");
                                exit(1);
                            }
                        } else if (option->recovery_timeout == 0 &&
                                   option->threshold != 0) {
                            recovery_cnt = option->threshold;
                        }
                    }
                }
            } else {
                failure_cnt += 1;

                time(&latest_failure_time);
                if (first_failure_time == 0) {
                    first_failure_time = latest_failure_time;
                }

                char buffer[128];
                sprintf(buffer, "%d", WEXITSTATUS(info.si_status));
                setenv("HEAT_FAIL_CODE", buffer, 1);
                sprintf(buffer, "%d", (unsigned int)first_failure_time);
                setenv("HEAT_FAIL_TIME", buffer, 1);
                sprintf(buffer, "%d", (unsigned int)latest_failure_time);
                setenv("HEAT_FAIL_LAST", buffer, 1);
                sprintf(buffer, "%d", option->interval);
                setenv("HEAT_FAIL_INTERVAL", buffer, 1);
                sprintf(buffer, "%d", info.si_pid);
                setenv("HEAT_FAIL_PID", buffer, 1);
                sprintf(buffer, "%d", failure_cnt);
                setenv("HEAT_FAIL_CNT", buffer, 1);

                recovery_cnt -= 1;
                if (recovery_wrapper_pid != 0) {
                    if (recovery_cnt != 0) {
                        continue;
                    }
                }

                if (option->pid != 0 && option->signal != -1) {
                    if (kill(option->pid, option->signal) == -1) {
                        perror("[ERROR](receiver)");
                        exit(1);
                    }
                }

                if (failure_cnt <= option->threshold) {
                    if (option->failure_script_path != NULL) {
                        if ((failure_wrapper_pid = fork()) == -1) {
                            perror("[ERROR](receiver)");
                            exit(1);
                        }

                        if (failure_wrapper_pid == 0) {
                            make_wrapper(PR_FAILURE);
                        } else {
                            waitpid(failure_wrapper_pid, NULL, 0);
                        }
                    }
                } else {
                    if (option->recovery_script_path != NULL) {
                        kill(option->pid, option->fault_signal);
                        if ((recovery_wrapper_pid = fork()) == -1) {
                            perror("[ERROR](receiver)");
                            exit(1);
                        }

                        if (recovery_wrapper_pid == 0) {
                            make_wrapper(PR_RECOVERY);
                        } else {
                            waitpid(recovery_wrapper_pid, NULL, 0);
                        }
                    }
                }
            }
        } else if (signo == SIGALRM) {
            if (recovery_wrapper_pid != 0 &&
                option->recovery_script_path != NULL) {
                if ((recovery_wrapper_pid = fork()) == -1) {
                    perror("[ERROR](receiver)");
                    exit(1);
                }

                if (recovery_wrapper_pid == 0) {
                    make_wrapper(PR_RECOVERY);
                } else {
                    waitpid(recovery_wrapper_pid, NULL, 0);
                }
            }
        } else if (signo == SIGUSR1 && allow_new_wrapper == 0) {
            pid = fork();
            if (pid == -1) {
                perror("[ERROR](receiver)");
                exit(1);
            } else if (pid == 0) {
                make_wrapper(PR_WRAPPER);
            }
        }
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
    timer.it_value.tv_sec = option->interval;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = option->interval;
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
            if (strcmp(set_str_status(info.si_code), "stopped") == 0) {
                fprintf(
                    stderr,
                    "[ERROR](heat): 시그널 수신 프로세스가 중단되었습니다..\n");
                kill(getpgid(getpid()) * -1, SIGKILL);
                exit(1);
            }
        } else if (signo == SIGALRM) {
            sigqueue(receiver_pid, SIGUSR1, payload);
        }

        if (waitpid(receiver_pid, &status, WNOHANG) == -1) {
            fprintf(stderr, "[ERROR](heat): 시그널 수신 프로세스에 문제가 "
                            "발생하여 종료합니다.\n");
            exit(1);
        }
    }
}