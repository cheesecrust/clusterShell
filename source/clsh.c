#include "clsh.h"

#define nodeNum 4
#define MSGSIZE 10000

pid_t pid[nodeNum];

int main(int argc, char *argv[]) {
    char buff[MSGSIZE] = {0};
    int input[nodeNum][2], output[nodeNum][2], err[nodeNum][2];

    char *nodeList[MSGSIZE] = {0};
    char remoteCommand[MSGSIZE] = {0};
    char keyboardBuffer[MSGSIZE] = {0};

    int nodeSize = 0;
    int remoteCommandSize = 0;
    int cnt = 0;
    int **option_list = getOption(argc, argv, &cnt);
    char *outFilename;
    char *errFilename;
    int commandStart = 1;

    for (int i = 0; i < cnt; i++) {
        int command_start = option_list[i][1];
        int command_end = argc;

        if (i != cnt - 1) {
            command_end = option_list[i + 1][1] - 1;
        }

        if (option_list[i][0] == HOST_OPTION_INDEX) {
            char *nodeString = argv[command_start];
            nodeSize = getNodes(nodeString, nodeList, ',');
            remoteCommandSize = getRemoteCommand(argv, remoteCommand, command_start + 1, command_end);
        }

        if (option_list[i][0] == HOST_FILE_OPTION_INDEX) {
            char nodes[MSGSIZE] = {0};
            int fd = open(argv[command_start], O_RDONLY);

            read(fd, nodes, MSGSIZE);

            nodeSize = getNodes(nodes, nodeList, '\n');
            remoteCommandSize = getRemoteCommand(argv, remoteCommand, command_start + 1, command_end);
        }

        if (option_list[i][0] == REDIRECTION_OPTION_INDEX) {
            read(0, keyboardBuffer, MSGSIZE);

            remoteCommandSize = getRemoteCommand(argv, remoteCommand, command_start, command_end);

            strcat(remoteCommand, "<<< ");
            strcat(remoteCommand, keyboardBuffer);
            replaceItem(remoteCommand, '\n', ' ');
            strcat(remoteCommand, "\n");

            remoteCommandSize = strlen(remoteCommand);
        }

        if (option_list[i][0] == OUTPUT_OPTION_INDEX) {
            outFilename = getFilename(argv[command_start]);
            commandStart = commandStart + 1;
        }

        if (option_list[i][0] == ERROR_OPTION_INDEX) {
            errFilename = getFilename(argv[command_start]);
            commandStart = commandStart + 1;
        }
    }

    if (nodeSize == 0) {
        char *clsh_host_env = getenv("CLSH_HOSTS");
        // 없을 경우 default 로 읽기
        // TODO : 여기 좀 바꿔야 함
        if (clsh_host_env == NULL) {
            int fd = open(argv[2], O_RDONLY);
            read(fd, clsh_host_env, MSGSIZE);
        }

        // 다 없을 경우 에러
        if (clsh_host_env == NULL) {
            printf("CLSH_HOSTS is not set\n");
            exit(1);
        }

        nodeSize = getNodes(clsh_host_env, nodeList, ':');
        remoteCommandSize = getRemoteCommand(argv, remoteCommand, commandStart, argc);
    }

    for (int i = 0; i < nodeSize; i++) {
        int fd = 1;
        // pipe() 함수를 이용하여 파이프를 생성한다.
        if (pipe(input[i]) == -1 || pipe(output[i]) == -1 || pipe(err[i]) == -1) {
            perror("pipe");
            exit(1);
        }

        if (fcntl(output[i][0], F_SETFL, O_NONBLOCK) == -1) {
            perror("fcntl call");
        }

        // fork() 함수를 이용하여 자식 프로세스를 생성한다.
        pid[i] = fork();

        // child process
        if (pid[i] == 0) {
            close(input[i][1]);
            close(output[i][0]);

            dup2(input[i][0], 0);   // file input
            dup2(output[i][1], 1);  // file output
            dup2(err[i][1], 2);     // file error

            // buffer setting 개행 문자를 만날때까지 채웁니다.

            setvbuf(stdin, NULL, _IOLBF, 0);
            setvbuf(stdout, NULL, _IOLBF, 0);

            // execvp() 함수를 이용하여 파이프를 연결한다.
            char *args[] = {"sshpass", "-p", "ubuntu", "ssh", nodeList[i], "-T", "-o",
                            "StrictHostKeyChecking=no", "-l", "ubuntu", NULL};
            if (execv("/bin/sshpass", args) == -1) {
                perror("execv");
                exit(1);
            }

        } else {
            close(input[i][0]);
            close(output[i][1]);
            close(err[i][1]);

            setvbuf(stdin, NULL, _IOLBF, 0);
            setvbuf(stdout, NULL, _IOLBF, 0);
        }
    }

    for (int i = 0; i < nodeSize; i++) {
        write(input[i][1], remoteCommand, remoteCommandSize);
    }

    int totalBufferSize = 0;
    int completeNode[4] = {0};

    while (1) {
        for (int i = 0; i < nodeSize; i++) {
            char readBuff[MSGSIZE] = {0};
            switch (read(output[i][0], readBuff, sizeof(readBuff))) {
                case -1: /* code */
                    perror("read");
                    break;
                case 0:
                    printf("EOF\n");
                    break;
                default:
                    strcat(buff, nodeList[i]);
                    strcat(buff, " : ");
                    strcat(buff, readBuff);

                    if (outFilename != NULL) {
                        char *tmp = strdup(outFilename);
                        strcat(tmp, nodeList[i]);
                        strcat(tmp, ".out");
                        printf("%s\n", tmp);
                        FILE *fp = fopen(tmp, "w");
                        fprintf(fp, "%s", readBuff);
                        fclose(fp);
                    }
                    completeNode[i] = 1;
                    break;
            }

            switch (read(err[i][0], readBuff, sizeof(readBuff))) {
                case -1: /* code */
                    perror("read");
                    break;
                case 0:
                    printf("EOF\n");
                    break;
                default:
                    if (errFilename != NULL) {
                        char *tmp = strdup(errFilename);
                        strcat(tmp, nodeList[i]);
                        strcat(tmp, ".err");

                        FILE *fp = fopen(tmp, "w");
                        fprintf(fp, "%s", readBuff);
                        fclose(fp);
                    }

                    completeNode[i] = 1;
                    break;
            }
        }

        bool isComplete = true;
        for (int i = 0; i < nodeSize; i++) {
            if (completeNode[i] == 0) {
                isComplete = false;
            }
        }

        if (isComplete) {
            break;
        }
    }

    printf("%s", buff);
    // printf("Complete\n");
    return 0;
}
