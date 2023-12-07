#include "clsh.h"

void replaceItem(char *buffer, char target, char replace) {
    for (int i = 0; i < strlen(buffer); i++) {
        if (buffer[i] == target) {
            buffer[i] = replace;
        }
    }
    return;
}

int getNodes(char *nodes, char *nodeList[], char delimiter) {
    int index = 0;
    int nodeSize = 0;
    char tmp[1000];

    for (int i = 0; i < strlen(nodes); ++i) {
        if (nodes[i] != delimiter) {
            tmp[index++] = nodes[i];
        } else {
            tmp[index] = '\0';  // 현재까지의 문자열 끝에 null 문자 추가
            nodeList[nodeSize++] = strdup(tmp);
            // 다음 아이템을 위해 초기화
            memset(tmp, 0, sizeof(tmp));
            index = 0;
        }
    }

    nodeList[nodeSize++] = strdup(tmp);
    // 마지막 아이템 처리
    return nodeSize;
}

int getRemoteCommand(char *command[], char *remoteCommand, int start, int end, int isRedirection) {
    int commandSize = 0;
    for (int i = start; i < end; i++) {
        strcat(remoteCommand, command[i]);
        strcat(remoteCommand, " ");
        commandSize += strlen(command[i]) + 1;
    }

    if (isRedirection) {
        remoteCommand[commandSize - 1] = '\0';
        return commandSize;
    }

    strcat(remoteCommand, "&& echo  ");
    commandSize += 12;
    remoteCommand[commandSize - 1] = '\n';
    return commandSize;
}

int **getOption(int argc, char **argv, int *cnt) {
    extern int optind;

    int **option_list = (int **)malloc(sizeof(int *) * 100);
    int c = 0;
    while (1) {
        int *item = (int *)malloc(sizeof(int) * 2);

        static struct option long_options[] = {
            {HOST_FILE_OPTION, required_argument, 0, HOST_FILE_OPTION_INDEX},
            {OUTPUT_OPTION, required_argument, 0, OUTPUT_OPTION_INDEX},
            {ERROR_OPTION, required_argument, 0, ERROR_OPTION_INDEX},
            {0, 0, 0, 0}};

        int option_index = 0;

        c = getopt_long(argc, argv, "h:b:i", long_options, &option_index);

        if (c == -1) {
            break;
        }

        switch (c) {
            case HOST_FILE_OPTION_INDEX:
                item[0] = HOST_FILE_OPTION_INDEX;
                item[1] = optind - 1;
                break;
            case 'h':
                item[0] = HOST_OPTION_INDEX;
                item[1] = optind - 1;
                break;
            case 'b':
                item[0] = REDIRECTION_OPTION_INDEX;
                item[1] = optind - 1;
                break;
            case OUTPUT_OPTION_INDEX:
                item[0] = OUTPUT_OPTION_INDEX;
                item[1] = optind - 1;
                break;
            case ERROR_OPTION_INDEX:
                item[0] = ERROR_OPTION_INDEX;
                item[1] = optind - 1;
                break;
            case 'i':
                item[0] = INTERACTIVE_OPTION_INDEX;
                item[1] = optind - 1;
                break;
            default:
                break;
        }

        option_list[*cnt] = item;
        *cnt += 1;
    }

    return option_list;
}

char *getFilename(char *command) {
    return command + 6;
}