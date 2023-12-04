#include "options.h"

HeatOption* option = NULL;

static struct option long_options[] = {
    {"pid", required_argument, 0, PID},
    {"signal", required_argument, 0, SIGNAL},
    {"fail", required_argument, 0, FAIL},
    {"threshold", required_argument, 0, THRESHOLD},
    {"recovery", required_argument, 0, RECOVERY},
    {"recovery-timeout", required_argument, 0, RECOVERY_TIMEOUT},
    {"fault-signal", required_argument, 0, FAULT_SIGNAL},
    {"success-signal", required_argument, 0, SUCCESS_SIGNAL},
    {0, 0, 0, 0}};

static char* signals[] = {
    "SIGHUP",      "SIGINT",      "SIGQUIT",     "SIGILL",      "SIGTRAP",
    "SIGABRT",     "SIGBUS",      "SIGFPE",      "SIGKILL",     "SIGUSR1",
    "SIGSEGV",     "SIGUSR2",     "SIGPIPE",     "SIGALRM",     "SIGTERM",
    "SIGSTKFLT",   "SIGCHLD",     "SIGCONT",     "SIGSTOP",     "SIGTSTP",
    "SIGTTIN",     "SIGTTOU",     "SIGURG",      "SIGXCPU",     "SIGXFSZ",
    "SIGVTALRM",   "SIGPROF",     "SIGWINCH",    "SIGIO",       "SIGPWR",
    "SIGSYS",      "SIGRTMIN",    "SIGRTMIN+1",  "SIGRTMIN+2",  "SIGRTMIN+3",
    "SIGRTMIN+4",  "SIGRTMIN+5",  "SIGRTMIN+6",  "SIGRTMIN+7",  "SIGRTMIN+8",
    "SIGRTMIN+9",  "SIGRTMIN+10", "SIGRTMIN+11", "SIGRTMIN+12", "SIGRTMIN+13",
    "SIGRTMIN+14", "SIGRTMIN+15", "SIGRTMAX-14", "SIGRTMAX-13", "SIGRTMAX-12",
    "SIGRTMAX-11", "SIGRTMAX-10", "SIGRTMAX-9",  "SIGRTMAX-8",  "SIGRTMAX-7",
    "SIGRTMAX-6",  "SIGRTMAX-5",  "SIGRTMAX-4",  "SIGRTMAX-3",  "SIGRTMAX-2",
    "SIGRTMAX-1",  "SIGRTMAX"};

// Option의 끝 index + 1를 반환
int find_end_of_option(int argc, char** argv) {
    for (int i = 2; i < argc; i++) {
        if (argv[i - 1][0] != '-' && argv[i][0] != '-') {
            return i;
        }
    }
    return argc;
}

HeatOption* parse_optarg(int argc, char** argv) {
    extern char* optarg;
    extern int optind, opterr, optopt;

    HeatOption* ret = (HeatOption*)malloc(sizeof(HeatOption));

    ret->end_of_option = find_end_of_option(argc, argv);

    ret->interval = 0;

    ret->script_path = NULL;
    ret->inspection_command = NULL;

    ret->pid = 0;
    ret->signal = SIGHUP;

    ret->threshold = 0;
    ret->recovery_script_path = NULL;
    ret->recovery_timeout = 0;

    ret->fault_signal = 0;
    ret->success_signal = 0;

    int n;
    time_t tt;
    time(&tt);
    FILE* check;
    while ((n = getopt_long(ret->end_of_option, argv, "i:s:", long_options,
                            NULL)) != -1) {
        switch (n) {
        case INTERVAL:
            ret->interval = atoi(optarg);
            if (ret->interval <= 0) {
                // 숫자로 변환에 실패하거나 0인 경우
                fprintf(stderr,
                        "Interval의 값은 0보다 큰 숫자이여야 합니다.\n");
                return NULL;
            }
            break;
        case SCRIPT:
            ret->script_path = optarg;
            if ((check = fopen(ret->script_path, "r")) == NULL) {
                fprintf(stderr,
                        "[%d](%d): 스크립트의 경로가 올바른지 확인해주세요.\n",
                        (unsigned int)tt, getpid());
                return NULL;
            }
            fclose(check);

            if (access(ret->script_path, X_OK | F_OK)) {
                fprintf(stderr, "[%d](%d): 스크립트 실행 권한이 없습니다.\n",
                        (unsigned int)tt, getpid());
                return NULL;
            }
            break;
        case PID:
            ret->pid = atoi(optarg);
            if (ret->pid <= 0) {
                fprintf(stderr, "pid의 값은 0보다 커야합니다.\n");
                return NULL;
            }
            break;
        case SIGNAL:
            ret->signal = get_signal(optarg);
            if (ret->signal == -1) {
                fprintf(stderr, "알 수 없는 시그널입니다.\n");
                return NULL;
            }
            break;
        case FAIL:
            ret->failure_script_path = optarg;
            if ((check = fopen(ret->failure_script_path, "r")) == NULL) {
                fprintf(
                    stderr,
                    "[%d](%d): 실패 스크립트의 경로가 올바른지 확인해주세요.\n",
                    (unsigned int)tt, getpid());
                return NULL;
            }
            fclose(check);

            if (access(ret->failure_script_path, X_OK | F_OK)) {
                fprintf(stderr,
                        "[%d](%d): 실패 스크립트 실행 권한이 없습니다.\n",
                        (unsigned int)tt, getpid());
                return NULL;
            }
            break;
        case RECOVERY:
            ret->recovery_script_path = optarg;
            if ((check = fopen(ret->recovery_script_path, "r")) == NULL) {
                fprintf(
                    stderr,
                    "[%d](%d): 복구 스크립트의 경로가 올바른지 확인해주세요.\n",
                    (unsigned int)tt, getpid());
                return NULL;
            }
            fclose(check);

            if (access(ret->recovery_script_path, X_OK | F_OK)) {
                fprintf(stderr,
                        "[%d](%d): 복구 스크립트 실행 권한이 없습니다.\n",
                        (unsigned int)tt, getpid());
                return NULL;
            }
            break;
        case THRESHOLD:
            ret->threshold = atoi(optarg);
            if (ret->threshold <= 0) {
                fprintf(stderr, "threshold 값은 0보다 큰 숫자이여야 합니다.\n");
                return NULL;
            }
            break;
        case RECOVERY_TIMEOUT:
            ret->recovery_timeout = atoi(optarg);
            if (ret->recovery_timeout <= 0) {
                fprintf(stderr,
                        "recovery timeout 값은 0보다 큰 숫자이여야 합니다.\n");
                return NULL;
            }
            break;
        case FAULT_SIGNAL:
            ret->fault_signal = get_signal(optarg);
            if (ret->fault_signal == -1) {
                fprintf(stderr, "알 수 없는 시그널입니다.\n");
                return NULL;
            }
            break;
        case SUCCESS_SIGNAL:
            ret->success_signal = get_signal(optarg);
            if (ret->success_signal == -1) {
                fprintf(stderr, "알 수 없는 시그널입니다.\n");
                return NULL;
            }
            break;
        }
    }

    if (ret->end_of_option != argc) {
        ret->inspection_command = argv + ret->end_of_option;
    }

    return ret;
}

int get_signal(char* signal) {
    int cnt = 1;
    char** pointer = signals;
    while (*pointer != NULL) {
        if (strcmp(*pointer, signal) == 0) {
            return cnt;
        } else if (strcmp(*pointer + 3, signal) == 0) {
            return cnt;
        }
        pointer++;
        cnt++;
    }
    return -1;
}
