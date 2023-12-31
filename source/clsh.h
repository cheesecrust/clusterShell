#ifndef CLSH_H
#define CLSH_H

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define HOST_OPTION "h"
#define HOST_FILE_OPTION "hostfile"
#define REDIRECTION_OPTION "b"
#define OUTPUT_OPTION "out"
#define ERROR_OPTION "err"
#define INTERACTIVE_OPTION "i"

#define HOST_OPTION_INDEX 0
#define HOST_FILE_OPTION_INDEX 1
#define REDIRECTION_OPTION_INDEX 2
#define OUTPUT_OPTION_INDEX 3
#define ERROR_OPTION_INDEX 4
#define INTERACTIVE_OPTION_INDEX 5

void replaceItem(char *item, char newItem, char path);

int getNodes(char *, char **, char);

int getRemoteCommand(char **, char *, int, int, int);

int **getOption(int, char **, int *);

char *getFilename(char *);
#endif