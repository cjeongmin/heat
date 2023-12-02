#ifndef __UTILS__
#define __UTILS__

#ifndef POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

char* set_str_status(int status);

#endif
