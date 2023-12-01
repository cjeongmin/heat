#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>

#include "handler.h"
#include "options.h"

extern State* state;

int main(int argc, char** argv) {
    state = parse_optarg(argc, argv);

    if (state == NULL) {
        fprintf(stderr, "[ERROR](heat): 옵션 파싱 실패");
        exit(1);
    }

    int signo;
    siginfo_t info;
    sigset_t block_mask, sigset_mask;

    sigfillset(&block_mask);
    sigdelset(&block_mask, SIGQUIT);
    sigdelset(&block_mask, SIGSTOP);
    sigdelset(&block_mask, SIGTERM);
    sigprocmask(SIG_SETMASK, &block_mask, NULL);

    sigemptyset(&sigset_mask);

    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "[ERROR](heat): fork error\n");
        exit(1);
    } else if (pid == 0) {
        sigaddset(&sigset_mask, SIGCHLD);
        sigaddset(&sigset_mask, SIGALRM);
        sigaddset(&sigset_mask, SIGUSR1);

        while (1) {
            if ((signo = sigwaitinfo(&sigset_mask, &info)) == -1) {
                fprintf(stderr, "[ERROR](receiver): sigwaitinfo error\n");
                exit(1);
            }

            if (signo == SIGCHLD) {

            } else if (signo == SIGALRM) {

            } else if (signo == SIGUSR1) {
            }
        }

    } else {
        sigaddset(&sigset_mask, SIGCHLD);
        sigaddset(&sigset_mask, SIGVTALRM);

        while (1) {
            if ((signo = sigwaitinfo(&sigset_mask, &info)) == -1) {
                fprintf(stderr, "[ERROR]: heat sigwaitinfo error\n");
                exit(1);
            }

            if (signo == SIGCHLD) {
            } else if (signo == SIGVTALRM) {
            }
        }
    }

    return 0;
}
