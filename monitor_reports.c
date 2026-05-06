#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <dirent.h>

void sigint_handler(int signal) {
    if(signal == SIGINT) {
        printf("\nINTERCEPTED SIGINT!\n"); 
        exit(1);
    }
}

void set_signal_action() {
    struct sigaction act; 

    bzero(&act, sizeof(act)); // put 0 in every member of the sigaction
    act.sa_handler = &sigint_handler; 
    sigaction(SIGINT, &act, NULL);
}

int main() {
    // overwrite or create a hidden text file called .monitor_pid 
    int BUFFER_SIZE = 64;
    struct stat st; 
    char monitor_file[BUFFER_SIZE]; 
    snprintf(monitor_file, sizeof(monitor_file), ".monitor_pid"); 

    int fd = open(monitor_file, O_WRONLY | O_APPEND | O_CREAT, 0777);
    if(fd == -1) {
        fprintf(stderr, "ERROR: couldn't create or open the .monitor_pid file!\n");
        exit(EXIT_FAILURE); 
    }

    close(fd);
    if(unlink(monitor_file) == -1) {
        fprintf(stderr, "ERROR: couldn't delete the .monitor_pid file!\n"); 
        exit(EXIT_FAILURE); 
    }


    set_signal_action();
    while(1){
        continue;
    }

    return 0; 
}
