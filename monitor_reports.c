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

volatile sig_atomic_t running = 1; // this is how global variables, that change their values in signal handlers are defined

void sigint_handler(int signal) {
    if(signal == SIGINT) {
        // don't use printf here, as if it would be unsafe 
        // due to the fact that program can finish in unexpected moments and 
        // thus leaving us with corrupted output 
        char message[] = "\nINTERCEPTED SIGINT! Terminating the program!\n";
        write(STDOUT_FILENO, message, sizeof(message) - 1); 
        running = 0; 
    }
}

void sigusr1_handler(int signal) {
    if(signal == SIGUSR1) {
        char message[] = "INTERCEPTED SIGUSR1! A new report has been added to the program!\n";
        write(STDOUT_FILENO, message, sizeof(message) - 1);
    }
}

void set_sigusr1_action() {
    struct sigaction act; 

    memset(&act, 0, sizeof(act)); // put 0 in every member of the sigaction
    act.sa_handler = &sigusr1_handler; 
    if(sigaction(SIGUSR1, &act, NULL) == -1){
        fprintf(stderr, "ERROR: couldn't set SIGUSR1 handler!\n");
        exit(EXIT_FAILURE);
    }
}

void set_sigint_action() {
    struct sigaction act; 

    memset(&act, 0, sizeof(act)); // put 0 in every member of the sigaction
    act.sa_handler = &sigint_handler; 
    if(sigaction(SIGINT, &act, NULL) == -1){
        fprintf(stderr, "ERROR: couldn't set SIGNINT handler!\n");
        exit(EXIT_FAILURE);
    }
}

int main() {
    // overwrite or create a hidden text file called .monitor_pid 
    int BUFFER_SIZE = 64;
    char monitor_file[BUFFER_SIZE]; 
    snprintf(monitor_file, sizeof(monitor_file), ".monitor_pid"); 

    int fd = open(monitor_file, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if(fd == -1) {
        fprintf(stderr, "ERROR: couldn't create or open the .monitor_pid file!\n");
        exit(EXIT_FAILURE); 
    }
    
    char pid_string[BUFFER_SIZE];
    int bytes_written = snprintf(pid_string, sizeof(pid_string), "%d\n", getpid());
    if(write(fd, pid_string, bytes_written) != bytes_written) {
        fprintf(stderr, "ERROR: couldn't write that pid inside of .monitor_pid file!\n");
        close(fd);
        exit(EXIT_FAILURE);
    }


    close(fd);

    set_sigint_action();
    set_sigusr1_action();
    while(running){
        pause();
    }

    if(unlink(monitor_file) == -1) {
        fprintf(stderr, "ERROR: couldn't delete the .monitor_pid file!\n"); 
        exit(EXIT_FAILURE); 
    }
    return 0; 
}
