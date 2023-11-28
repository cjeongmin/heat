#ifndef __OPTIONS__
#define __OPTIONS__

#include <dirent.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

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
    int ppid;
    int end_of_option;
    int interval;
    char* script_path;
    char** inspection_command;
} option;

int find_end_of_option(int, char**);
char* concat(int, char**);
option* parse_optarg(int, char**);

#endif