#ifndef __OPTIONS__
#define __OPTIONS__

#ifndef POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define INTERVAL 'i'
#define SCRIPT 's'
#define PID 'p'
#define SIGNAL 'n'
#define FAIL 'f'
#define THRESHOLD 't'
#define RECOVERY 'r'
#define RECOVERY_TIMEOUT 'e'
#define FAULT_SIGNAL 'a'
#define SUCCESS_SIGNAL 'u'

typedef struct HeatOption {
    int end_of_option;

    int interval;

    char* script_path;
    char** inspection_command;

    int pid;
    int signal;
    unsigned int threshold;

    char* failure_script_path;

    char* recovery_script_path;
    int recovery_timeout;

    int fault_signal;
    int success_signal;
} HeatOption;

int find_end_of_option(int, char**);
HeatOption* parse_optarg(int, char**);
int get_signal(char*);

#endif