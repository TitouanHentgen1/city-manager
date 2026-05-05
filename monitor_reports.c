#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define PID_FILE ".monitor_pid"

static void handler_sigusr1(int sig){
    (void)sig;
    const char*msg="[monitor]Report added /n";
    write(STDOUT_FILENO,msg,strlen(msg));
}
static void handler_sigint(int sig ){
    (void)sig;
    const char*msg="[monitor]SIGINT received — shutting down./n"
    write(STDOUT_FILENO,msg,strlen(msg));
    unlink(PID_FILE);
    exit(0);
}
int main(void) {
    int fd =open(PID_FILE,O_WRONLY|O_CREAT|O_TRUNC,0644);
    if(fd==-1){perror("open .monitor_pid");return 1;}
    char pidbuf[32];
    int len=snprintf(pidbuf,sizeof(pidbuf),"%d\n",(int)getpid());
    if (write(fd,pidbuf,len)!=len) { perror("write .monitor_pid"); close(fd); return 1; }
    close(fd);

    printf("[monitor] Started. PID = %d\n",(int)getpid());
    printf("[monitor] Waiting for signals (SIGUSR1 = new report, SIGINT = stop)...\n");
    fflush(stdout);

    struct sigaction sa;

    sa.sa_handler=handler_sigusr1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags=SA_RESTART;
    if (sigaction(SIGUSR1,&sa,NULL)==-1) { perror("sigaction SIGUSR1"); unlink(PID_FILE); return 1; }

    sa.sa_handler=handler_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags=0;
    if (sigaction(SIGINT,&sa,NULL)==-1) { perror("sigaction SIGINT"); unlink(PID_FILE); return 1; }
    
    while (1) pause();
    return 0;
}
