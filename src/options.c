#include "options.h"

option* state = NULL;

static struct option long_options[] = {
    {"pid", required_argument, 0, PID},
    {"signal", required_argument, 0, SINGAL},
    {"fail", required_argument, 0, FAIL},
    {"threshold", required_argument, 0, THRESHOLD},
    {"recovery", required_argument, 0, RECOVERY},
    {"recovery-timeout", required_argument, 0, RECOVERY_TIMEOUT},
    {"fault-signal", required_argument, 0, FAULT_SIGNAL},
    {"success-signal", required_argument, 0, SUCCESS_SIGNAL},
    {0, 0, 0, 0}};

// Option의 끝 index + 1를 반환
int find_end_of_option(int argc, char** argv) {
    for (int i = 2; i < argc; i++) {
        if (argv[i - 1][0] != '-' && argv[i][0] != '-') {
            return i;
        }
    }
    return argc;
}

char* concat(int size, char** arr) {
    int length = size - 1;
    for (int i = 0; i < size; i++) {
        length += strlen(arr[i]);
    }

    int p = 0;
    char* res = malloc(sizeof(char) * length + 1);
    for (int i = 0; i < size; i++) {
        for (size_t j = 0; j < strlen(arr[i]); j++) {
            res[p++] = arr[i][j];
        }
        res[p++] = ' ';
    }
    res[length] = '\0';
    return res;
}

option* parse_optarg(int argc, char** argv) {
    extern char* optarg;
    extern int optind, opterr, optopt;

    option* ret = malloc(sizeof(option));
    ret->interval = 0;
    ret->script_path = NULL;

    int n;
    ret->end_of_option = find_end_of_option(argc, argv);
    while ((n = getopt_long(ret->end_of_option, argv, "i:s:", long_options,
                            NULL)) != -1) {
        switch (n) {
        case INTERVAL:;
            ret->interval = atoi(optarg);
            if (ret->interval == 0) {
                // 숫자로 변환에 실패하거나 0인 경우
                fprintf(stderr,
                        "Interval의 값은 0보다 큰 숫자이여야 합니다.\n");
                return NULL;
            }
            break;
        case SCRIPT:;
            ret->script_path = optarg;
            // path를 읽어서 해당 디렉토리로 이동, 스크립트 권한 확인
            break;
        }
    }

    if (ret->end_of_option != argc) {
        ret->inspection_command = argv + ret->end_of_option;
    }

    return ret;
}
