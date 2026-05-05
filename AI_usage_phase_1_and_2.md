# ai_usage.md
## Phase 1

 AI Tool used: Claude (claude.ai)
 
The Generated functions

Function parse_condition
Prompt: "Generate a C function `int parse_condition(const char *input, char *field, char *op, char *value)` that splits a `field:op:value` string into three parts."

Code:
int parse_condition(const char *input, char *field, char *op, char *value) {

    const char *colon1 = strchr(input, ':');
    if (!colon1) return 0;

    size_t flen = colon1 - input;    
    strcpy(field, input, flen);
    field[flen] = '\0';            

    const char *colon2 = strchr(colon1 + 1, ':');
    if (!colon2) return 0;

    size_t olen = colon2 - colon1 - 1;
    strcpy(op, colon1 + 1, olen);
    op[olen] = '\0';

    strcpy(value, colon2 + 1, 63);
    value[63] = '\0';

    return 1;
}

Changes made:
- Replaced `strcpy` with `strncpy`
- Added operator validation



Function: match_condition
Prompt: "Generate a C function `int match_condition(Report *r, const char *field, const char *op, const char *value)` that returns 1 if the report matches the condition."

Code:
int match_condition(Report *r, const char *field, const char *op, const char *value) {

    if (strcmp(field, "severity") == 0) {
        int rval = r->severity;
        int cval = atoi(value);  

        if (strcmp(op, "==") == 0) return rval == cval;
        if (strcmp(op, "!=") == 0) return rval != cval;
        if (strcmp(op, "<")  == 0) return rval <  cval;
        if (strcmp(op, "<=") == 0) return rval <= cval;
        if (strcmp(op, ">")  == 0) return rval >  cval;
        if (strcmp(op, ">=") == 0) return rval >= cval;
    }

    if (strcmp(field, "timestamp") == 0) {
        time_t rval = r->timestamp;
        time_t cval = (time_t)atol(value);
       
    }
    if (strcmp(field, "category") == 0) {
        int cmp = strcmp(r->category, value);
        if (strcmp(op, "==") == 0) return cmp == 0;
        if (strcmp(op, "!=") == 0) return cmp != 0;
      
    }
    
}

Changes:
- `timestamp` field was missing — added with `atol()`
- `category` and `inspector` only handled `==` — added remaining operators
- Added `atoi()` for `severity` type conversion

## What I learned
The AI generates a functional skeleton but misses edge cases.
You have to check after IA to see if no mistakes were made with a functions and also verifies if the function works with the rest of your code.
##Phase 2
AI tool used :Claude(claude.ai)
The monitor reports :
Functions 

'static void handler_sigusr1(int sig){
    
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

Main 
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
    if (sigaction(SIGINT,&sa,NULL)==-1) { perror("sigaction SIGINT"); unlink(PID_FILE); return 1; }                                                                                                                while (1) pause();
    return 0;
}
