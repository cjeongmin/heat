#ifndef __OPTIONS__
#define __OPTIONS__

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
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

typedef struct {
    int pid;
    int signal;
    int end_of_option;
    int interval;
    char* script_path;
    char** inspection_command;
    int failure_script_pid;
    char* failure_script_path;
} State;

int find_end_of_option(int, char**);
char* concat(int, char**);
State* parse_optarg(int, char**);
int get_signal(char*);

#endif