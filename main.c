#include "options.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int find_end_of_option(int, char **);
char *concat(int, char **);
void parse_optarg(int, char **);

int interval = 1;
char *target = NULL;

int main(int argc, char **argv) {
    parse_optarg(argc, argv);

    while (1) {
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
