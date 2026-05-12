


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define MAX_LINE 512

static void run_hub_mon(void) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("[hub_mon] pipe");
        _exit(1);
    }

    pid_t monitor_pid = fork();
    if (monitor_pid < 0) {
        perror("[hub_mon] fork monitor");
        _exit(1);
    }

    if (monitor_pid == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("[monitor] dup2");
            _exit(1);
        }
        close(pipefd[1]);
        execlp("./monitor_reports", "./monitor_reports", (char *)NULL);
        perror("[monitor] execlp");
        _exit(1);
    }

    close(pipefd[1]);

    char buf[MAX_LINE];
    int monitor_done = 0;

    while (!monitor_done) {
        int pos = 0;
        ssize_t n;
        while (pos < MAX_LINE - 1) {
            n = read(pipefd[0], buf + pos, 1);
            if (n <= 0) { monitor_done = 1; break; }
            if (buf[pos] == '\n') { pos++; break; }
            pos++;
        }
        if (pos == 0) break;
        buf[pos] = '\0';

        char display[MAX_LINE];
        strncpy(display, buf, MAX_LINE - 1);
        display[MAX_LINE - 1] = '\0';
        int dlen = strlen(display);
        if (dlen > 0 && display[dlen - 1] == '\n') display[dlen - 1] = '\0';

        if (strncmp(buf, "MSG:", 4) == 0) {
            printf("[monitor] %s\n", display + 4);
            fflush(stdout);
        } else if (strncmp(buf, "ERR:", 4) == 0) {
            printf("[monitor ERROR] %s\n", display + 4);
            fflush(stdout);
        } else if (strncmp(buf, "END:", 4) == 0) {
            printf("[monitor] %s\n", display + 4);
            printf("[hub_mon] Monitor has ended.\n");
            fflush(stdout);
            monitor_done = 1;
        } else {
            printf("[monitor] %s\n", display);
            fflush(stdout);
        }
    }

    close(pipefd[0]);
    waitpid(monitor_pid, NULL, 0);
    printf("[hub_mon] Exiting.\n");
    fflush(stdout);
    _exit(0);
}

static void cmd_start_monitor(void) {
    pid_t hub_mon_pid = fork();
    if (hub_mon_pid < 0) {
        perror("fork hub_mon");
        return;
    }
    if (hub_mon_pid == 0) {
        run_hub_mon();
        _exit(0);
    }
    printf("[city_hub] hub_mon started (PID=%d). Monitor output will appear here.\n",
           (int)hub_mon_pid);
    fflush(stdout);
}

static void cmd_calculate_scores(char **districts, int count) {
    if (count == 0) {
        printf("Usage: calculate_scores <district1> [district2] ...\n");
        return;
    }

    printf("=== Combined Workload Report ===\n");
    fflush(stdout);

    for (int i = 0; i < count; i++) {
        const char *district = districts[i];

        if (strchr(district, '/') || strchr(district, '.')) {
            printf("[scorer] Invalid district name: '%s'\n", district);
            continue;
        }

        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe");
            continue;
        }

        pid_t scorer_pid = fork();
        if (scorer_pid < 0) {
            perror("fork scorer");
            close(pipefd[0]);
            close(pipefd[1]);
            continue;
        }

        if (scorer_pid == 0) {
            close(pipefd[0]);
            if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
                perror("[scorer] dup2");
                _exit(1);
            }
            close(pipefd[1]);
            execlp("./scorer", "./scorer", district, (char *)NULL);
            perror("[scorer] execlp");
            _exit(1);
        }

        close(pipefd[1]);

        char buf[MAX_LINE];
        ssize_t n;
        while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
            buf[n] = '\0';
            printf("%s", buf);
            fflush(stdout);
        }
        close(pipefd[0]);

        int status;
        waitpid(scorer_pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
            printf("[scorer] Process for '%s' exited with error.\n", district);
    }

    printf("=== End of Workload Report ===\n");
    fflush(stdout);
}

static void print_help(void) {
    printf("Commands:\n");
    printf("  start_monitor                     Start background monitor\n");
    printf("  calculate_scores <d1> [d2] ...    Workload report per district\n");
    printf("  help                              Show this message\n");
    printf("  exit                              Quit city_hub\n");
}

int main(void) {
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NOCLDWAIT;
    sigaction(SIGCHLD, &sa, NULL);

    printf("=== city_hub ===\n");
    printf("Type 'help' for available commands.\n\n");
    fflush(stdout);

    char line[1024];
    while (1) {
        printf("hub> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n[city_hub] EOF — exiting.\n");
            break;
        }

        int len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';

        char *tokens[64];
        int token_count = 0;
        char *tok = strtok(line, " \t");
        while (tok && token_count < 63) {
            tokens[token_count++] = tok;
            tok = strtok(NULL, " \t");
        }

        if (token_count == 0) continue;

        if (strcmp(tokens[0], "exit") == 0 || strcmp(tokens[0], "quit") == 0) {
            printf("[city_hub] Goodbye.\n");
            break;
        } else if (strcmp(tokens[0], "help") == 0) {
            print_help();
        } else if (strcmp(tokens[0], "start_monitor") == 0) {
            cmd_start_monitor();
        } else if (strcmp(tokens[0], "calculate_scores") == 0) {
            cmd_calculate_scores(&tokens[1], token_count - 1);
        } else {
            printf("Unknown command: '%s'. Type 'help' for usage.\n", tokens[0]);
        }
    }

    return 0;
}


