


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


City MANAGER.C

#include "city_manager.h"

void mode_to_str(mode_t mode, char *buf) {
    buf[0]=(mode&S_IRUSR)?'r':'-'; buf[1]=(mode&S_IWUSR)?'w':'-'; buf[2]=(mode&S_IXUSR)?'x':'-';
    buf[3]=(mode&S_IRGRP)?'r':'-'; buf[4]=(mode&S_IWGRP)?'w':'-'; buf[5]=(mode&S_IXGRP)?'x':'-';
    buf[6]=(mode&S_IROTH)?'r':'-'; buf[7]=(mode&S_IWOTH)?'w':'-'; buf[8]=(mode&S_IXOTH)?'x':'-';
    buf[9]='\0';
}

int check_access(const char *path, const char *role, int need_write) {
    struct stat st;
    if (stat(path,&st)==-1) return 1;
    mode_t b=st.st_mode;
    if (strcmp(role,"manager")==0) {
        if (need_write && !(b&S_IWUSR)) return 0;
        if (!need_write && !(b&S_IRUSR)) return 0;
    } else {
        if (need_write && !(b&S_IWGRP)) return 0;
        if (!need_write && !(b&S_IRGRP)) return 0;
    }
    return 1;
}

void log_action(const char *district, const char *role, const char *user, const char *action) {
    char path[256];
    snprintf(path,sizeof(path),"districts/%s/logged_district",district);
    int fd=open(path,O_WRONLY|O_APPEND);
    if (fd==-1) return;
    time_t now=time(NULL);
    char ts[32], line[512];
    strftime(ts,sizeof(ts),"%Y-%m-%d %H:%M:%S",localtime(&now));
    int len=snprintf(line,sizeof(line),"[%s] role=%s user=%s action=%s\n",ts,role,user,action);
    write(fd,line,len);
    close(fd);
}

void notify_monitor(const char *district, const char *role, const char *user) {
    int fd=open(MONITOR_PID_FILE,O_RDONLY);
    if (fd==-1) {
        log_action(district,role,user,"add - WARNING: monitor not found (no .monitor_pid)");
        return;
    }
    char buf[32];
    memset(buf,0,sizeof(buf));
    ssize_t n=read(fd,buf,sizeof(buf)-1);
    close(fd);
    if (n<=0) {
        log_action(district,role,user,"add - ERROR: .monitor_pid is empty or unreadable");
        return;
    }
    pid_t monitor_pid=(pid_t)atoi(buf);
    if (monitor_pid<=0) {
        log_action(district,role,user,"add - ERROR: invalid PID in .monitor_pid");
        return;
    }
    if (kill(monitor_pid,SIGUSR1)==-1) {
        char msg[128];
        snprintf(msg,sizeof(msg),"add - ERROR: kill(SIGUSR1, pid=%d) failed: %s",
                 (int)monitor_pid,strerror(errno));
        log_action(district,role,user,msg);
    } else {
        char msg[128];
        snprintf(msg,sizeof(msg),"add - monitor notified via SIGUSR1 (pid=%d)",(int)monitor_pid);
        log_action(district,role,user,msg);
    }
}

void create_district(const char *id) {
    char dir[256],reports[256],cfg[256],logf[256],link[256];
    snprintf(dir,    sizeof(dir),    "districts/%s",id);
    snprintf(reports,sizeof(reports),"districts/%s/reports.dat",id);
    snprintf(cfg,    sizeof(cfg),    "districts/%s/district.cfg",id);
    snprintf(logf,   sizeof(logf),   "districts/%s/logged_district",id);
    snprintf(link,   sizeof(link),   "active_reports-%s",id);

    mkdir("districts",0750);
    if (mkdir(dir,0750)==-1 && errno!=EEXIST) { perror("mkdir"); return; }
    chmod(dir,0750);

    int fd;
    fd=open(reports,O_WRONLY|O_CREAT|O_EXCL,0664);
    if (fd!=-1) { chmod(reports,0664); close(fd); }

    fd=open(cfg,O_WRONLY|O_CREAT|O_EXCL,0640);
    if (fd!=-1) { chmod(cfg,0640); write(fd,"threshold=1\n",12); close(fd); }

    fd=open(logf,O_WRONLY|O_CREAT|O_EXCL,0644);
    if (fd!=-1) { chmod(logf,0644); close(fd); }

    unlink(link);
    symlink(reports,link);
}

void cmd_add(const char *district, const char *role, const char *user,
             double lat, double lon, const char *category, int severity, const char *description) {
    char path[256];
    snprintf(path,sizeof(path),"districts/%s/reports.dat",district);
    create_district(district);
    if (!check_access(path,role,1)) { fprintf(stderr,"Access denied\n"); return; }

    struct stat st;
    int next_id=1;
    if (stat(path,&st)==0 && st.st_size>0)
        next_id=(int)(st.st_size/sizeof(Report))+1;

    Report r;
    memset(&r,0,sizeof(r));
    r.report_id=next_id;
    strncpy(r.inspector,  user,       sizeof(r.inspector)-1);
    strncpy(r.category,   category,   sizeof(r.category)-1);
    strncpy(r.description,description,sizeof(r.description)-1);
    r.latitude=lat; r.longitude=lon;
    r.severity=severity; r.timestamp=time(NULL);

    int fd=open(path,O_WRONLY|O_APPEND);
    if (fd==-1) { perror("open"); return; }
    if (write(fd,&r,sizeof(r))!=(ssize_t)sizeof(r)) perror("write");
    else printf("Report #%d added to '%s'\n",next_id,district);
    close(fd);

    log_action(district,role,user,"add");
    notify_monitor(district,role,user);
}

void cmd_list(const char *district, const char *role) {
    char path[256];
    snprintf(path,sizeof(path),"districts/%s/reports.dat",district);
    if (!check_access(path,role,0)) { fprintf(stderr,"Access denied\n"); return; }

    struct stat st;
    if (stat(path,&st)==-1) { fprintf(stderr,"District not found\n"); return; }

    char perm[10], mtime[32];
    mode_to_str(st.st_mode,perm);
    strftime(mtime,sizeof(mtime),"%Y-%m-%d %H:%M",localtime(&st.st_mtime));
    printf("=== %s ===\nPermissions: %s | Size: %lld bytes | Modified: %s\n",
           district,perm,(long long)st.st_size,mtime);

    char link[256];
    snprintf(link,sizeof(link),"active_reports-%s",district);
    struct stat lst;
    if (lstat(link,&lst)==0 && S_ISLNK(lst.st_mode)) {
        if (stat(link,&st)==-1 && errno==ENOENT) fprintf(stderr,"Warning: broken symlink: %s\n",link);
        else printf("Symlink: %s -> OK\n",link);
    }
    printf("---\n");

    int fd=open(path,O_RDONLY);
    if (fd==-1) { perror("open"); return; }
    Report r; int count=0;
    while (read(fd,&r,sizeof(r))==(ssize_t)sizeof(r)) {
        char ts[32]; time_t tmp=r.timestamp;
        strftime(ts,sizeof(ts),"%Y-%m-%d %H:%M",localtime(&tmp));
        printf("[#%d] %s | %s | sev:%d | %s\n      %s\n      GPS: %.6f, %.6f\n\n",
               r.report_id,r.inspector,r.category,r.severity,ts,r.description,r.latitude,r.longitude);
        count++;
    }
    close(fd);
    if (count==0) printf("(No reports)\n");
    else printf("Total: %d report(s)\n",count);
    log_action(district,"manager",role,"list");
}

void cmd_view(const char *district, int report_id, const char *role) {
    char path[256];
    snprintf(path,sizeof(path),"districts/%s/reports.dat",district);
    if (!check_access(path,role,0)) { fprintf(stderr,"Access denied\n"); return; }

    int fd=open(path,O_RDONLY);
    if (fd==-1) { perror("open"); return; }
    Report r; int found=0;
    while (read(fd,&r,sizeof(r))==(ssize_t)sizeof(r))
        if (r.report_id==report_id) { found=1; break; }
    close(fd);

    if (!found) { fprintf(stderr,"Report #%d not found\n",report_id); return; }
    char ts[32]; time_t tmp=r.timestamp;
    strftime(ts,sizeof(ts),"%Y-%m-%d %H:%M:%S",localtime(&tmp));
    printf("=== Report #%d ===\nInspector  : %s\nCategory   : %s\nSeverity   : %d/3\nGPS        : %.6f, %.6f\nDate       : %s\nDescription: %s\n",
           r.report_id,r.inspector,r.category,r.severity,r.latitude,r.longitude,ts,r.description);
}

void cmd_remove_report(const char *district, int report_id, const char *role, const char *user) {
    if (strcmp(role,"manager")!=0) { fprintf(stderr,"Manager only\n"); return; }
    char path[256];
    snprintf(path,sizeof(path),"districts/%s/reports.dat",district);

    int fd=open(path,O_RDWR);
    if (fd==-1) { perror("open"); return; }
    struct stat st; fstat(fd,&st);
    int total=(int)(st.st_size/sizeof(Report));

    int idx=-1;
    for (int i=0;i<total;i++) {
        Report r;
        lseek(fd,(off_t)i*sizeof(Report),SEEK_SET);
        read(fd,&r,sizeof(r));
        if (r.report_id==report_id) { idx=i; break; }
    }
    if (idx==-1) { fprintf(stderr,"Report #%d not found\n",report_id); close(fd); return; }

    for (int i=idx+1;i<total;i++) {
        Report r;
        lseek(fd,(off_t)i*sizeof(Report),SEEK_SET);
        read(fd,&r,sizeof(r));
        lseek(fd,(off_t)(i-1)*sizeof(Report),SEEK_SET);
        write(fd,&r,sizeof(r));
    }
    ftruncate(fd,(off_t)(total-1)*sizeof(Report));
    close(fd);
    printf("Report #%d removed (%d -> %d)\n",report_id,total,total-1);
    log_action(district,role,user,"remove_report");
}

void cmd_update_threshold(const char *district, int value, const char *role, const char *user) {
    if (strcmp(role,"manager")!=0) { fprintf(stderr,"Manager only\n"); return; }
    char path[256];
    snprintf(path,sizeof(path),"districts/%s/district.cfg",district);

    struct stat st;
    if (stat(path,&st)==-1) { perror("stat"); return; }
    if ((st.st_mode&0777)!=0640) {
        char p[10]; mode_to_str(st.st_mode,p);
        fprintf(stderr,"Wrong permissions on district.cfg (%s)\n",p); return;
    }
    if (value<1||value>3) { fprintf(stderr,"Invalid threshold (1-3)\n"); return; }

    int fd=open(path,O_WRONLY|O_TRUNC);
    if (fd==-1) { perror("open"); return; }
    char buf[32]; int len=snprintf(buf,sizeof(buf),"threshold=%d\n",value);
    write(fd,buf,len); close(fd);
    printf("Threshold set to %d for '%s'\n",value,district);
    log_action(district,role,user,"update_threshold");
}

int parse_condition(const char *input, char *field, char *op, char *value) {
    const char *p1=strchr(input,':');
    if (!p1) return 0;
    int fl=(int)(p1-input);
    if (fl<=0||fl>=32) return 0;
    strncpy(field,input,fl); field[fl]='\0';

    const char *p2=strchr(p1+1,':');
    if (!p2) return 0;
    int ol=(int)(p2-p1-1);
    if (ol<=0||ol>=4) return 0;
    strncpy(op,p1+1,ol); op[ol]='\0';

    const char *v=p2+1;
    if (!*v||strlen(v)>=64) return 0;
    strncpy(value,v,63); value[63]='\0';

    if (strcmp(op,"==")==0||strcmp(op,"!=")==0||strcmp(op,"<")==0||
        strcmp(op,"<=")==0||strcmp(op,">")==0||strcmp(op,">=")==0) return 1;
    fprintf(stderr,"Unknown operator: '%s'\n",op); return 0;
}

static int cmp_int(int a,const char *op,int b) {
    if (strcmp(op,"==")==0) return a==b;
    if (strcmp(op,"!=")==0) return a!=b;
    if (strcmp(op,"<") ==0) return a<b;
    if (strcmp(op,"<=")==0) return a<=b;
    if (strcmp(op,">") ==0) return a>b;
    if (strcmp(op,">=")==0) return a>=b;
    return 0;
}

static int cmp_str(const char *a,const char *op,const char *b) {
    int c=strcmp(a,b);
    if (strcmp(op,"==")==0) return c==0;
    if (strcmp(op,"!=")==0) return c!=0;
    if (strcmp(op,"<") ==0) return c<0;
    if (strcmp(op,"<=")==0) return c<=0;
    if (strcmp(op,">") ==0) return c>0;
    if (strcmp(op,">=")==0) return c>=0;
    return 0;
}

int match_condition(Report *r, const char *field, const char *op, const char *value) {
    if (strcmp(field,"severity") ==0) return cmp_int(r->severity,op,atoi(value));
    if (strcmp(field,"timestamp")==0) return cmp_int((int)r->timestamp,op,(int)atol(value));
    if (strcmp(field,"category") ==0) return cmp_str(r->category,op,value);
    if (strcmp(field,"inspector")==0) return cmp_str(r->inspector,op,value);
    fprintf(stderr,"Unknown field: '%s'\n",field); return 0;
}

void cmd_filter(const char *district, const char *role, int cond_count, char **conditions) {
    char path[256];
    snprintf(path,sizeof(path),"districts/%s/reports.dat",district);
    if (!check_access(path,role,0)) { fprintf(stderr,"Access denied\n"); return; }

    int fd=open(path,O_RDONLY);
    if (fd==-1) { perror("open"); return; }
    printf("=== Filter on '%s' ===\n",district);
    Report r; int found=0;

    while (read(fd,&r,sizeof(r))==(ssize_t)sizeof(r)) {
        int match=1;
        for (int i=0;i<cond_count&&match;i++) {
            char field[32],op[4],value[64];
            if (!parse_condition(conditions[i],field,op,value)) { match=0; break; }
            if (!match_condition(&r,field,op,value)) match=0;
        }
        if (match) {
            char ts[32]; time_t tmp=r.timestamp;
            strftime(ts,sizeof(ts),"%Y-%m-%d %H:%M",localtime(&tmp));
            printf("[#%d] %s | %s | sev:%d | %s\n      %s\n\n",
                   r.report_id,r.inspector,r.category,r.severity,ts,r.description);
            found++;
        }
    }
    close(fd);
    if (found==0) printf("(No results)\n");
    else printf("Total: %d report(s)\n",found);
}

void cmd_remove_district(const char *district, const char *role, const char *user) {
    if (strcmp(role,"manager")!=0) {
        fprintf(stderr,"Error: remove_district is manager only\n");
        return;
    }
    if (!district||*district=='\0'||strchr(district,'/')!=NULL||strchr(district,'.')!=NULL) {
        fprintf(stderr,"Error: invalid district name '%s'\n",district?district:"");
        return;
    }

    char dir_path[300];
    snprintf(dir_path,sizeof(dir_path),"districts/%s",district);

    struct stat st;
    if (stat(dir_path,&st)==-1||!S_ISDIR(st.st_mode)) {
        fprintf(stderr,"Error: district '%s' does not exist\n",district);
        return;
    }

    pid_t pid=fork();
    if (pid<0) { perror("fork"); return; }

    if (pid==0) {
        execlp("rm","rm","-rf",dir_path,(char *)NULL);
        perror("execlp");
        _exit(1);
    }

    int status;
    if (waitpid(pid,&status,0)==-1) { perror("waitpid"); return; }

    if (WIFEXITED(status)&&WEXITSTATUS(status)==0) {
        char link[256];
        snprintf(link,sizeof(link),"active_reports-%s",district);
        if (unlink(link)==-1&&errno!=ENOENT) perror("unlink symlink");
        printf("District '%s' and its symlink removed\n",district);
    } else {
        fprintf(stderr,"Error: rm -rf failed (code=%d)\n",
                WIFEXITED(status)?WEXITSTATUS(status):-1);
    }
}

static void usage(void) {
    fprintf(stderr,
        "Usage:\n"
        "  city_manager --role <r> --user <u> --add <district> [--lat f] [--lon f] [--category s] [--severity n] [--desc s]\n"
        "  city_manager --role <r> --user <u> --list <district>\n"
        "  city_manager --role <r> --user <u> --view <district> <id>\n"
        "  city_manager --role manager --user <u> --remove_report <district> <id>\n"
        "  city_manager --role manager --user <u> --update_threshold <district> <n>\n"
        "  city_manager --role <r> --user <u> --filter <district> <cond...>\n"
        "  city_manager --role manager --user <u> --remove_district <district>\n"
    );
}

int main(int argc, char *argv[]) {
    char role[32]="",user[64]="",command[32]="",district[64]="";
    int report_id=-1,threshold=-1,severity=1;
    double lat=0.0,lon=0.0;
    char category[32]="unknown",description[256]="No description";
    char **conditions=NULL; int cond_count=0;

    if (argc<2) { usage(); return 1; }

    for (int i=1;i<argc;i++) {
        if      (!strcmp(argv[i],"--role")             &&i+1<argc) strncpy(role,    argv[++i],sizeof(role)-1);
        else if (!strcmp(argv[i],"--user")             &&i+1<argc) strncpy(user,    argv[++i],sizeof(user)-1);
        else if (!strcmp(argv[i],"--add")              &&i+1<argc) { strcpy(command,"add");              strncpy(district,argv[++i],sizeof(district)-1); }
        else if (!strcmp(argv[i],"--list")             &&i+1<argc) { strcpy(command,"list");             strncpy(district,argv[++i],sizeof(district)-1); }
        else if (!strcmp(argv[i],"--view")             &&i+2<argc) { strcpy(command,"view");             strncpy(district,argv[++i],sizeof(district)-1); report_id=atoi(argv[++i]); }
        else if (!strcmp(argv[i],"--remove_report")    &&i+2<argc) { strcpy(command,"remove_report");    strncpy(district,argv[++i],sizeof(district)-1); report_id=atoi(argv[++i]); }
        else if (!strcmp(argv[i],"--update_threshold") &&i+2<argc) { strcpy(command,"update_threshold"); strncpy(district,argv[++i],sizeof(district)-1); threshold=atoi(argv[++i]); }
        else if (!strcmp(argv[i],"--remove_district")  &&i+1<argc) { strcpy(command,"remove_district");  strncpy(district,argv[++i],sizeof(district)-1); }
        else if (!strcmp(argv[i],"--filter")           &&i+1<argc) { strcpy(command,"filter");           strncpy(district,argv[++i],sizeof(district)-1); conditions=&argv[i+1]; cond_count=argc-i-1; break; }
        else if (!strcmp(argv[i],"--lat")              &&i+1<argc) lat=atof(argv[++i]);
        else if (!strcmp(argv[i],"--lon")              &&i+1<argc) lon=atof(argv[++i]);
        else if (!strcmp(argv[i],"--category")         &&i+1<argc) strncpy(category,argv[++i],sizeof(category)-1);
        else if (!strcmp(argv[i],"--severity")         &&i+1<argc) severity=atoi(argv[++i]);
        else if (!strcmp(argv[i],"--desc")             &&i+1<argc) strncpy(description,argv[++i],sizeof(description)-1);
    }

    if (!*role) { fprintf(stderr,"--role required\n"); usage(); return 1; }
    if (!*command) { fprintf(stderr,"Missing command\n"); usage(); return 1; }

    if      (!strcmp(command,"add"))              cmd_add(district,role,user,lat,lon,category,severity,description);
    else if (!strcmp(command,"list"))             cmd_list(district,role);
    else if (!strcmp(command,"view"))             cmd_view(district,report_id,role);
    else if (!strcmp(command,"remove_report"))    cmd_remove_report(district,report_id,role,user);
    else if (!strcmp(command,"update_threshold")) cmd_update_threshold(district,threshold,role,user);
    else if (!strcmp(command,"filter"))           cmd_filter(district,role,cond_count,conditions);
    else if (!strcmp(command,"remove_district"))  cmd_remove_district(district,role,user);
    else { fprintf(stderr,"Unknown command\n"); usage(); return 1; }

    return 0;
}


CityManager.h
#ifndef CITY_MANAGER_H
#define CITY_MANAGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define MONITOR_PID_FILE ".monitor_pid"

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

void mode_to_str(mode_t mode, char *buf);
int  check_access(const char *path, const char *role, int need_write);
void log_action(const char *district, const char *role, const char *user, const char *action);
void notify_monitor(const char *district, const char *role, const char *user);
void create_district(const char *district_id);
void cmd_add(const char *district, const char *role, const char *user,
             double lat, double lon, const char *category,
             int severity, const char *description);
void cmd_list(const char *district, const char *role);
void cmd_view(const char *district, int report_id, const char *role);
void cmd_remove_report(const char *district, int report_id, const char *role, const char *user);
void cmd_update_threshold(const char *district, int value, const char *role, const char *user);
void cmd_filter(const char *district, const char *role, int cond_count, char **conditions);
void cmd_remove_district(const char *district, const char *role, const char *user);
int  parse_condition(const char *input, char *field, char *op, char *value);
int  match_condition(Report *r, const char *field, const char *op, const char *value);

#endif
