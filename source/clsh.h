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

void replaceItem(char *item, char newItem, char path);

int getNodes(char *, char **, char);

int getRemoteCommand(char **, char *, int, int);

void getOption(int, char **);

#endif