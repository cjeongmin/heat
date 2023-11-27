#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define INTERVAL 'i'
#define SCRIPT 's'
#define PID 'p'
#define SINGAL 'n'
#define FAIL 'f'
#define THRESHOLD 't'
#define RECOVERY 'r'
#define RECOVERY_TIMEOUT 'e'
#define FAULT_SIGNAL 'f'
#define SUCCESS_SIGNAL 'u'

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

int interval = 1;
char *target = NULL;

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
        for (int j = 0; j < strlen(arr[i]); j++) {
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
    int options = find_end_of_option(argc, argv);
    while ((n = getopt_long(options, argv, "i:s:", long_options, NULL)) != -1) {
        switch (n) {
        case INTERVAL:
            interval = atoi(optarg);
            if (interval == 0) {
                // 숫자로 변환에 실패하거나 0인 경우
                write(STDERR_FILENO,
                      "간격은 0보다 큰 숫자만 입력할 수 있습니다.\n", 41);
                exit(1);
            }
            break;
        case SCRIPT:
            break;
        }
    }

    if (options != argc) {
        target = concat(argc - options, argv + options);
        printf("%s\n", target);
    }
}

int main(int argc, char **argv) {
    parse_optarg(argc, argv);

    while (1) {
    }

    return 0;
}
