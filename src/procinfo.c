#include "common.h"
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
static void usage(const char *a){fprintf(stderr,"Usage: %s <pid>\n",a); exit(1);}
static int isnum(const char*s){for(;*s;s++) if(!isdigit(*s)) return 0; return 1;}
int main(int c,char**v){
    if(c!=2||!isnum(v[1])) usage(v[0]);

    char path[256];
    snprintf(path, sizeof(path),"/proc/%s/stat", v[1]);
    FILE *f = fopen(path, "r");
    if (!f) {
        if (errno == ENOENT) {
            fprintf(stderr, "PID %s not found\n", v[1]);
        } else if (errno ==EACCES) {
            fprintf(stderr, "Permission denied for PID %s\n", v[1]);
        } else {
            perror("Error opening stat file");
        }
        return 1;
    }

    int pid, ppid;
    char comm[256];
    char state;
    unsigned long utime, stime;
    if (fscanf(f, "%d (%[^)]) %c %d", &pid, comm, &state, &ppid) != 4) {
        fprintf(stderr, "Failed to parse stat header \n");
        fclose(f);
        return 1;
    }

    unsigned long dummy;
    for (int i = 0; i < 9; i++)
        fscanf(f, "%lu", &dummy);

    fscanf(f, "%lu %lu", &utime, &stime);
    fclose(f);

    long ticks = sysconf(_SC_CLK_TCK);
    double cpu_time = (utime + stime) / (double)ticks;



    snprintf(path, sizeof(path), "/proc/%s/cmdline", v[1]);
    f = fopen(path, "r");
    char cmd[4096];
    size_t n = 0;
    if (!f) {
        perror("cmdline");
    } else {
        n = fread(cmd, 1, sizeof(cmd) - 1, f);
        fclose(f);
        cmd[n] = '\0';
        for (size_t i = 0; i < n; i++) {
            if (cmd[i] == '\0')
                cmd[i] = ' ';
        }
    }



    snprintf(path, sizeof(path), "/proc/%s/status", v[1]);
    f = fopen(path, "r");
    char line[256];
    long rss_kb = -1;
    if (!f) {
        perror("status");
    } else {
        while(fgets(line, sizeof(line), f)) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                sscanf(line, "VmRSS: %ld", &rss_kb);
                break;
            }
        }
        fclose(f);
    }



    printf("State: %c\n", state);
    printf("Parent PID: %d\n", ppid);
    if (n == 0)
        printf("Command line: [none]\n");
    else
        printf("Command line: %s\n", cmd);
    printf("CPU time: %.2f seconds\n", cpu_time);
    if (rss_kb == -1)
        printf("Memory: [not available]\n");
    else
        printf("Memory: %ld kB\n", rss_kb);
    return 0;
}
