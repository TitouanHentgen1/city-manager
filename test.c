/*new command deletes entire district directory and all contents
Create a child process,calls external command rm-rf<district_directory>*/
#include <sys/wait.h>

void remove_district(const*char district){
    if(strcmp(role,"manager")!=0){
        fprintf(stder,"Only Manager");
        return;
    }
    char district_path[256];
    char symlink_path[256];

    pid_t pid=fork();

    if(pid=<0){
        perror("Fork failed");
        return;
    }

    if(pid==0){
        execlp("rm","rm","-rf",district_path,(char*)NULL);
        perror("System Failed")
        exit(EXIT Failure);
    }

    if(pid>0){
        int state;
        wait(pid,&state,0);
        if (WIFEXITED(state) && WEXITSTATUS(state) == 0) {
            printf("District directory '%s' deleted successfully.\n", district_path);
            
            if (unlink(symlink_path) == 0) {
                printf("Symlink '%s' removed successfully.\n", symlink_path);
            } else {
                perror("Warning: Failed to remove symlink");
            }
        } else {
            fprintf(stderr, "Error: Failed to delete directory '%s'.\n", district_path);
        }


    }

}
int execlp(const char*file,const char *arg,.../*(char *) NULL */);

int execlp(const char*city_manager,const char*arg){

}


/*A process a running application that has own code.A Parent process ,works with a child process*/
process Id unique identifier 
Child process wil have the same code of the parent process 

pid=fork()
-1=error
0=child
0>parent
if (pid=0){
    execlp(, )
}
remove the symbolic link in the parent process

System calls 
pid_t getpid(); PID of the calling process
pid_t getppid();PID of the parent of the caling process

open()
close()
read()
write()
unlink()
sigaction()
kill()
fork()
exec*()
do not use signal()

/* monitor reports 
at startup it creates or overwrites a hidden text file called .monitor_pid where it stores its main process ID. The file is situated at the same level of the directory tree as the district directories.
when it ends, it deletes the above file
the program only ends when it receives SIGINT, and writes a message on the standard output when it does
the program responds to SIGUSR1 signals (which informs it that a new report has been added) by writing a message on the standard output
*/


.monitor_pid
SIGINT
SIGUSR1


Example handling structure
// Example in C
FILE *f = fopen(".monitor_pid", "r");
if (f == NULL) {
    fprintf(log_file, "Error: Monitor PID not found. Monitor not informed.\n");
} else {
    pid_t pid;
    fscanf(f, "%d", &pid);
    fclose(f);
    if (kill(pid, SIGUSR1) == 0) {
        fprintf(log_file, "Report added. Monitor notified.\n");
    } else {
        fprintf(log_file, "Error: Could not send SIGUSR1. Monitor not informed.\n");
    }
}
