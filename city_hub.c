//code AI



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


#define Max_line 512


static void run_hub_mon(void) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("[hub_mon] pipe");
        exit(1);
    }

    pid_t monitor_pid = fork();
    if (monitor_pid < 0) {
        perror("[hub_mon] fork monitor");
        exit(1);
    }

    if (monitor_pid == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("[monitor] dup2");
            exit(1);
        }
        close(pipefd[1]);
        execlp("./monitor_reports", "./monitor_reports", (char *)NULL);
        perror("[monitor] execlp");
        exit(1);
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












static void cmd_calculate_scores(char **districts,int count ){
    if(count==0){
        printf("Usgage:calculate_scores <district1> [district2]...\n")
        return;
    }
    printf("===Combined Work loadreport===\n")
    fflush(stdout);




}














calculate_scores<list_of_districts>
dup2()














