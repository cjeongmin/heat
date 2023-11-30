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

typedef struct FailState {
    pid_t pid;
    int exit_code;
    unsigned int first_time;
    unsigned int last_time;
    int interval;
    int fail_cnt;
} FailState;

typedef struct State {
    int end_of_option;

    int interval;

    char* script_path;
    char** inspection_command;

    int pid;
    int signal;

    FailState* fail_state;

    char* failure_script_path;
    pid_t failure_script_pid;
    int failure_count;

    int threshold;
    char* recovery_script_path;
    pid_t recovery_script_pid;
    pid_t recovery_timeout_timer_pid;
    int recovery_timeout;
    int recovery_script_executed;
    int recovery_script_ended;
} State;

int find_end_of_option(int, char**);
char* concat(int, char**);
State* parse_optarg(int, char**);
int get_signal(char*);

#endif