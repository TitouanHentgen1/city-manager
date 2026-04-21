#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
typedef struct
{
    int id;
    char name[30];
    float latitude;
    float longitude;
    char issue[10];
    int severity;
    time_t timestamp;
    char description[50];
}Report;

void add(char *district,char *user,float latitude,float longitude, char *category,int severity,time_t timestamp, char *description)
{
    struct stat st={0};
    if(stat(district,&st)==-1)
    {
        mkdir(district,0750);
        char path[100]="";
        strcpy(path,district);
        strcat(path,"/reports.dat");
        int f1=open(path,O_RDWR);
        chmod(path,0664);

        strcpy(path,district);
        strcat(path,"/district.cfg");
        int f2=open(path,O_RDWR);
        chmod(path,0640);

        strcpy(path,district);
        strcat(path,"/logged_district");
        int f3=open(path,O_RDWR);
        chmod(path,0644);
    }




}

int main(int argc,char *argv[])
{

}
