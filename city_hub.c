#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define MAX_LINE 512
#define MAX_NAME  64

typedef struct {
    int    report_id;
    char   inspector[64];
    double latitude;
    double longitude;
    char   category[32];
    int    severity;
    time_t timestamp;
    char   description[256];
} __attribute__((packed)) Report;

typedef struct {
    char name[MAX_NAME];
    int  score;
    int  count;
} Inspector;

static void run_scorer(const char *district) {
    char path[256];
    snprintf(path, sizeof(path), "districts/%s/reports.dat", district);

    int fd = open(path, O_RDONLY);
    if (fd == -1) { printf("District '%s': not found.\n", district); return; }

    struct stat st;
    fstat(fd, &st);
    if (st.st_size == 0) { printf("District '%s': no reports.\n", district); close(fd); return; }

    Inspector inspectors[128];
    int count = 0;
    Report r;

    while (read(fd, &r, sizeof(Report)) == (ssize_t)sizeof(Report)) {
        int found = 0;
        for (int i = 0; i < count; i++) {
            if (strcmp(inspectors[i].name, r.inspector) == 0) {
                inspectors[i].score += r.severity;
                inspectors[i].count++;
                found = 1; break;
            }
        }
        if (!found && count < 128) {
            strncpy(inspectors[count].name, r.inspector, MAX_NAME-1);
            inspectors[count].name[MAX_NAME-1] = '\0';
            inspectors[count].score = r.severity;
            inspectors[count].count = 1;
            count++;
        }
    }
    close(fd);

    printf("=== Scores: '%s' ===\n", district);
    for (int i = 0; i < count; i++)
        printf("  %-20s reports: %d   score: %d\n",
               inspectors[i].name, inspectors[i].count, inspectors[i].score);
    printf("\n");
    fflush(stdout);
}

static void run_hub_mon(void) {
    int pipefd[2];
    if (pipe(pipefd) == -1) { perror("pipe"); _exit(1); }

    pid_t mpid = fork();
    if (mpid < 0) { perror("fork"); _exit(1); }

    if (mpid == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) { perror("dup2"); _exit(1); }
        close(pipefd[1]);
        execlp("./monitor_reports", "./monitor_reports", (char*)NULL);
        perror("execlp"); _exit(1);
    }

    close(pipefd[1]);

    char buf[MAX_LINE];
    int done = 0;

    while (!done) {
        int pos = 0;
        ssize_t n;
        while (pos < MAX_LINE-1) {
            n = read(pipefd[0], buf+pos, 1);
            if (n <= 0) { done = 1; break; }
            if (buf[pos] == '\n') { pos++; break; }
            pos++;
        }
        if (pos == 0) break;
        buf[pos] = '\0';

        char display[MAX_LINE];
        strncpy(display, buf, MAX_LINE-1);
        display[MAX_LINE-1] = '\0';
        int dlen = strlen(display);
        if (dlen > 0 && display[dlen-1] == '\n') display[dlen-1] = '\0';

        if      (strncmp(buf,"MSG:",4)==0) { printf("[monitor] %s\n",   display+4); fflush(stdout); }
        else if (strncmp(buf,"ERR:",4)==0) { printf("[monitor] ERR: %s\n", display+4); fflush(stdout); }
        else if (strncmp(buf,"END:",4)==0) { printf("[monitor] %s\n",   display+4); printf("[hub_mon] Monitor ended.\n"); fflush(stdout); done=1; }
        else                               { printf("[monitor] %s\n",   display);    fflush(stdout); }
    }

    close(pipefd[0]);
    waitpid(mpid, NULL, 0);
    _exit(0);
}

static void cmd_start_monitor(void) {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }
    if (pid == 0) { run_hub_mon(); _exit(0); }
    printf("[hub] Monitor started (hub_mon PID=%d).\n", (int)pid);
    fflush(stdout);
}

static void cmd_calculate_scores(char **districts, int count) {
    if (count == 0) { printf("Usage: calculate_scores <district...>\n"); return; }

    printf("=== Workload Report ===\n"); fflush(stdout);

    for (int i = 0; i < count; i++) {
        if (strchr(districts[i],'/')||strchr(districts[i],'.')) {
            printf("Invalid district: '%s'\n", districts[i]); continue;
        }

        int pipefd[2];
        if (pipe(pipefd) == -1) { perror("pipe"); continue; }

        pid_t pid = fork();
        if (pid < 0) { perror("fork"); close(pipefd[0]); close(pipefd[1]); continue; }

        if (pid == 0) {
            close(pipefd[0]);
            if (dup2(pipefd[1], STDOUT_FILENO) == -1) { perror("dup2"); _exit(1); }
            close(pipefd[1]);
            run_scorer(districts[i]);
            fflush(stdout);
            _exit(0);
        }

        close(pipefd[1]);
        char buf[MAX_LINE];
        ssize_t n;
        while ((n = read(pipefd[0], buf, sizeof(buf)-1)) > 0) {
            buf[n] = '\0'; printf("%s", buf); fflush(stdout);
        }
        close(pipefd[0]);
        int status;
        waitpid(pid, &status, 0);
    }

    printf("=== End of Report ===\n"); fflush(stdout);
}

static void print_help(void) {
    printf("Commands:\n"
           "  start_monitor\n"
           "  calculate_scores <d1> [d2] ...\n"
           "  help\n"
           "  exit\n");
}

int main(void) {
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NOCLDWAIT;
    sigaction(SIGCHLD, &sa, NULL);

    printf("=== city_hub === (type 'help')\n\n");
    fflush(stdout);

    char line[1024];
    while (1) {
        printf("hub> "); fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) { printf("\n[hub] EOF.\n"); break; }

        int len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';

        char *tokens[64]; int tc = 0;
        char *tok = strtok(line, " \t");
        while (tok && tc < 63) { tokens[tc++] = tok; tok = strtok(NULL, " \t"); }
        if (tc == 0) continue;

        if      (!strcmp(tokens[0],"exit")||!strcmp(tokens[0],"quit")) { printf("[hub] Goodbye.\n"); break; }
        else if (!strcmp(tokens[0],"help"))              print_help();
        else if (!strcmp(tokens[0],"start_monitor"))     cmd_start_monitor();
        else if (!strcmp(tokens[0],"calculate_scores"))  cmd_calculate_scores(&tokens[1], tc-1);
        else printf("Unknown: '%s'\n", tokens[0]);
    }
    return 0;
}
