#ifndef __HANDLER__
#define __HANDLER__

#define _POSIX_C_SOURCE 200809L
#include "options.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

void sigchld_handler();

#endif