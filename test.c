#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_CMD 1024
#define MAX_DISTRICTS 32

void start_monitor();
void run_hub_mon();
void calculate_scores(char *cmd);

int main() {

    char cmd[MAX_CMD];

    while (1) {

        printf("city_hub> ");
        fflush(stdout);

        if (fgets(cmd, sizeof(cmd), stdin) == NULL)
            break;

        cmd[strcspn(cmd, "\n")] = '\0';

        if (strcmp(cmd, "exit") == 0) {
            break;
        }
        else if (strcmp(cmd, "start_monitor") == 0) {
            start_monitor();
        }
        else if (strncmp(cmd, "calculate_scores", 16) == 0) {
            calculate_scores(cmd);
        }
        else {
            printf("Unknown command\n");
        }
    }

    return 0;
}

void start_monitor() {

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        run_hub_mon();
        exit(0);
    }

    printf("hub_mon started with PID %d\n", pid);
}

void run_hub_mon() {

    int pipefd[2];

    if (pipe(pipefd) < 0) {
        perror("pipe");
        exit(1);
    }

    pid_t mon_pid = fork();

    if (mon_pid < 0) {
        perror("fork");
        exit(1);
    }

    if (mon_pid == 0) {

        close(pipefd[0]);

        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);

        close(pipefd[1]);

        execl("./monitor", "./monitor", NULL);

        perror("exec monitor");
        exit(1);
    }

    close(pipefd[1]);

    char buffer[512];
    int n;

    while ((n = read(pipefd[0], buffer, sizeof(buffer)-1)) > 0) {

        buffer[n] = '\0';

        printf("[MONITOR OUTPUT]\n%s", buffer);
        fflush(stdout);

        if (strstr(buffer, "END|")) {
            printf("[HUB] Monitor terminated.\n");
        }

        if (strstr(buffer, "ERROR|")) {
            printf("[HUB] Monitor startup failed.\n");
        }
    }

    close(pipefd[0]);

    waitpid(mon_pid, NULL, 0);
}

void calculate_scores(char *cmd) {

    char *districts[MAX_DISTRICTS];
    int count = 0;

    char *token = strtok(cmd, " ");

    token = strtok(NULL, " ");

    while (token != NULL && count < MAX_DISTRICTS) {

        districts[count++] = token;

        token = strtok(NULL, " ");
    }

    if (count == 0) {
        printf("No districts provided\n");
        return;
    }

    int pipes[MAX_DISTRICTS][2];
    pid_t pids[MAX_DISTRICTS];

    for (int i = 0; i < count; i++) {

        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return;
        }

        pids[i] = fork();

        if (pids[i] < 0) {
            perror("fork");
            return;
        }

        if (pids[i] == 0) {

            close(pipes[i][0]);

            dup2(pipes[i][1], STDOUT_FILENO);

            close(pipes[i][1]);

            execl("./scorer", "./scorer", districts[i], NULL);

            perror("exec scorer");
            exit(1);
        }

        close(pipes[i][1]);
    }

    printf("\n=== WORKLOAD REPORT ===\n\n");

    char buffer[1024];

    for (int i = 0; i < count; i++) {

        int n = read(pipes[i][0], buffer, sizeof(buffer)-1);

        if (n > 0) {

            buffer[n] = '\0';

            printf("%s\n", buffer);
        }

        close(pipes[i][0]);

        waitpid(pids[i], NULL, 0);
    }
}
