#include "clsh.h"

#define nodeNum 4
#define MSGSIZE 10000

pid_t pid[nodeNum];
char *nodeList[MSGSIZE] = {0};
bool mainTerminated = false;
int nodeSize = 0;

void terminateAllChildProcess(int sig) {
    // sigchld block
    int childStatus;
    // 모든 자식프로세스에 sigstop 시그널 보내기
    for (int i = 0; i < nodeSize; i++) {
        kill(pid[i], sig);
    }

    // 자식 프로세스가 끝날 때까지 wait
    for (int i = 0; i < nodeSize; i++) {
        pid_t terminatedChild = wait(&childStatus);
        if (terminatedChild <= 0) continue;
        printf("자식 프로세스(%d)가 종료되었습니다.\n", terminatedChild);
    }
    printf("모든 프로세스가 종료되었습니다.\n");
}

void sigTermHandler(int signo) {
    mainTerminated = true;
    psignal(signo, "Received Signal:");
    terminateAllChildProcess(SIGTERM);
    exit(0);
}

void sigQuitHandler(int signo) {
    mainTerminated = true;
    psignal(signo, "Received Signal:");
    terminateAllChildProcess(SIGKILL);
    exit(0);
}

void sigChildHandler(int signo) {
    if (mainTerminated) return;
    pid_t pid_child;
    int node = -1;
    int status;
    while (1) {
        if ((pid_child = waitpid(-1, &status, WNOHANG)) > 0) {
            printf("자식 프로세스(%d)가 종료되었습니다.\n", pid_child);
            for (int i = 0; i < nodeSize; i++) {
                if ((int)pid_child == (int)pid[i]) {
                    node = i;
                }
            }
        } else {
            break;
        }
    }
    printf("ERROR: %s connection lost\n", nodeList[node]);
    terminateAllChildProcess(SIGTERM);
    exit(0);
}

int main(int argc, char *argv[]) {
    int input[nodeNum][2], output[nodeNum][2], err[nodeNum][2];

    char remoteCommand[MSGSIZE] = {0};
    char keyboardBuffer[MSGSIZE] = {0};
    char buff[MSGSIZE] = {0};

    int remoteCommandSize = 0;
    int cnt = 0;
    int **option_list = getOption(argc, argv, &cnt);
    char *outFilename;
    char *errFilename;
    int commandStart = 1;
    int interactive = 0;

    for (int i = 0; i < cnt; i++) {
        int command_start = option_list[i][1];
        int command_end = argc;

        if (i != cnt - 1) {
            command_end = option_list[i + 1][1] - 1;
        }

        if (option_list[i][0] == HOST_OPTION_INDEX) {
            char *nodeString = argv[command_start];
            nodeSize = getNodes(nodeString, nodeList, ',');
            remoteCommandSize = getRemoteCommand(argv, remoteCommand, command_start + 1, command_end, 0);
        }

        if (option_list[i][0] == HOST_FILE_OPTION_INDEX) {
            char nodes[MSGSIZE] = {0};
            int fd = open(argv[command_start], O_RDONLY);

            read(fd, nodes, MSGSIZE);

            nodeSize = getNodes(nodes, nodeList, '\n');
            remoteCommandSize = getRemoteCommand(argv, remoteCommand, command_start + 1, command_end, 0);
        }

        if (option_list[i][0] == REDIRECTION_OPTION_INDEX) {
            read(0, keyboardBuffer, MSGSIZE);

            memset(remoteCommand, 0, sizeof(remoteCommand));

            remoteCommandSize = getRemoteCommand(argv, remoteCommand, command_start, command_end, 1);
            printf("%s\n", remoteCommand);

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

        if (option_list[i][0] == INTERACTIVE_OPTION_INDEX) {
            commandStart = commandStart + 1;
            interactive = 1;
        }
    }

    if (nodeSize == 0) {
        char *clsh_host_env = getenv("CLSH_HOSTS");
        // 없을 경우 default 로 읽기

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
        remoteCommandSize = getRemoteCommand(argv, remoteCommand, commandStart, argc, 0);
    }

    struct sigaction act, chld;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = sigTermHandler;
    if (sigaction(SIGTERM, &act, (struct sigaction *)NULL) < 0) {
        perror("sigaction");
        exit(1);
    }
    if (sigaction(SIGINT, &act, (struct sigaction *)NULL) < 0) {
        perror("sigaction");
        exit(1);
    }
    if (sigaction(SIGTSTP, &act, (struct sigaction *)NULL) < 0) {
        perror("sigaction");
        exit(1);
    }
    act.sa_handler = sigQuitHandler;
    if (sigaction(SIGQUIT, &act, (struct sigaction *)NULL) < 0) {
        perror("sigaction");
        exit(1);
    }

    // sigchld 처리
    chld.sa_handler = sigChildHandler;
    sigfillset(&chld.sa_mask);
    chld.sa_flags = SA_NOCLDSTOP;
    sigaction(SIGCHLD, &chld, NULL);

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

        if (fcntl(err[i][0], F_SETFL, O_NONBLOCK) == -1) {
            perror("fcntl call");
        }

        if (fcntl(0, F_SETFL, O_NONBLOCK) == -1) {
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

    int completeNode[4] = {0};
    int totalBufferSize = 0;

    while (interactive) {
        memset(buff, 0, sizeof(buff));
        for (int i = 0; i < nodeSize; i++) {
            completeNode[i] = 0;
        }

        printf("Enter 'quit' to leave this interactive mode\n");
        printf("Working with nodes:");
        for (int i = 0; i < nodeSize; i++) {
            printf("%s,", nodeList[i]);
        }

        printf("\n");
        write(1, "clsh> ", 6);

        while (1) {
            char interactiveBuff[MSGSIZE] = {0};
            char command[MSGSIZE] = {0};
            switch (read(0, command, sizeof(command))) {
                case -1:
                    // perror("read");
                    break;
                case 0:
                    printf("EOF\n");
                    break;
                default:
                    if (strcmp(command, "quit\n") == 0) {
                        printf("Bye\n");
                        exit(0);
                    }
                    break;
            }

            if (command[0] == '!') {
                printf("Local : ");
                system(command + 1);
                exit(1);
            }

            if (command[0] != '\0') {
                command[strlen(command) - 1] = ' ';
                strcat(command, "&& echo  ");
                strcat(command, "\n");
                for (int i = 0; i < nodeSize; i++) {
                    write(input[i][1], command, sizeof(command));
                }
                break;
            }
        }

        printf("---------------------------\n");

        while (1) {
            for (int i = 0; i < nodeSize; i++) {
                if (completeNode[i] == 1) {
                    continue;
                }
                int n;
                char interactiveBuff[MSGSIZE] = {0};
                switch ((n = read(output[i][0], interactiveBuff, sizeof(interactiveBuff) - 1))) {
                    case -1: /* code */
                        // perror("read");
                        break;
                    case 0:
                        completeNode[i] = 1;
                        break;
                    default:
                        interactiveBuff[n] = '\0';
                        strcat(buff, nodeList[i]);
                        strcat(buff, " : ");
                        strcat(buff, interactiveBuff);
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
    }

    for (int i = 0; i < nodeSize; i++) {
        write(input[i][1], remoteCommand, remoteCommandSize);
    }

    while (1) {
        for (int i = 0; i < nodeSize; i++) {
            if (completeNode[i]) continue;

            int n;
            char readBuff[MSGSIZE] = {0};
            switch ((n = read(output[i][0], readBuff, sizeof(readBuff)))) {
                case -1: /* code */
                    // perror("read");
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
                        FILE *fp = fopen(tmp, "w");
                        fprintf(fp, "%s", readBuff);
                        fclose(fp);
                    }
                    completeNode[i] = 1;
                    break;
            }

            char errBuff[MSGSIZE] = {0};

            switch (read(err[i][0], errBuff, sizeof(errBuff))) {
                case -1: /* code */
                    // perror("read");
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
                        fprintf(fp, "%s", errBuff);
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
