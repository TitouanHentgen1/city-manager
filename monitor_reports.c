#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define PID_FILE ".monitor_pid"

static void handler_sigusr1(int sig) {
    (void)sig;
    const char *msg = "MSG:New report added to the city!\n";
    write(STDOUT_FILENO, msg, strlen(msg));
}

static void handler_sigint(int sig) {
    (void)sig;
    const char *msg = "END:Monitor shutting down (SIGINT received).\n";
    write(STDOUT_FILENO, msg, strlen(msg));
    unlink(PID_FILE);
    _exit(0);
}

int main(void) {
    int existing = open(PID_FILE, O_RDONLY);
    if (existing != -1) {
        char buf[32];
        memset(buf, 0, sizeof(buf));
        ssize_t n = read(existing, buf, sizeof(buf) - 1);
        close(existing);
        if (n > 0) {
            pid_t existing_pid = (pid_t)atoi(buf);
            if (existing_pid > 0 && kill(existing_pid, 0) == 0) {
                char msg[128];
                snprintf(msg, sizeof(msg),
                    "ERR:Monitor already running with PID=%d.\n", (int)existing_pid);
                write(STDOUT_FILENO, msg, strlen(msg));
                const char *end = "END:Exiting — another monitor is active.\n";
                write(STDOUT_FILENO, end, strlen(end));
                _exit(1);
            }
        }
    }

    int fd = open(PID_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        const char *err = "ERR:Cannot create .monitor_pid file.\n";
        write(STDOUT_FILENO, err, strlen(err));
        const char *end = "END:Startup failed.\n";
        write(STDOUT_FILENO, end, strlen(end));
        _exit(1);
    }
    char pidbuf[32];
    int len = snprintf(pidbuf, sizeof(pidbuf), "%d\n", (int)getpid());
    if (write(fd, pidbuf, len) != len) {
        const char *end = "END:Write to .monitor_pid failed.\n";
        write(STDOUT_FILENO, end, strlen(end));
        close(fd);
        _exit(1);
    }
    close(fd);

    char startmsg[64];
    snprintf(startmsg, sizeof(startmsg), "MSG:Monitor started (PID=%d).\n", (int)getpid());
    write(STDOUT_FILENO, startmsg, strlen(startmsg));

    struct sigaction sa;

    sa.sa_handler = handler_sigusr1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        const char *end = "END:sigaction SIGUSR1 failed.\n";
        write(STDOUT_FILENO, end, strlen(end));
        unlink(PID_FILE);
        _exit(1);
    }

    sa.sa_handler = handler_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        const char *end = "END:sigaction SIGINT failed.\n";
        write(STDOUT_FILENO, end, strlen(end));
        unlink(PID_FILE);
        _exit(1);
    }

    while (1) pause();
    return 0;
}
