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

int getRemoteCommand(char *command[], char *remoteCommand, int start, int end) {
    int commandSize = 0;

    for (int i = start; i < end; i++) {
        strcat(remoteCommand, command[i]);
        strcat(remoteCommand, " ");
        commandSize += strlen(command[i]) + 1;
    }

    remoteCommand[commandSize - 1] = '\n';
    return commandSize;
}

void getOption(int argc, char **argv) {
    extern int optind;

    int c = 0;
    while (1) {
        static struct option long_options[] = {
            {HOST_FILE_OPTION, required_argument, 0, 1},
            {0, 0, 0, 0}};

        int option_index = 0;

        c = getopt_long(argc, argv, "h:b:", long_options, &option_index);

        if (c == -1) {
            break;
        }

        switch (c) {
            case 1:
                printf("%d \n", optind);
                break;
            default:
                printf("%d \n", optind);
                break;
        }
    }
}
