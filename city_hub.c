#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


void writestr(int fd, const char *str) {
	write(fd, str, strlen(str));
}

void start_monitor() {
	pid_t hub_mon;	
	char buf;	

	hub_mon = fork();
	if (hub_mon < 0) {
		fprintf(stderr, "ERROR: couldn't start a hub_mon!\n");
		exit(EXIT_FAILURE);
	}

	else if (hub_mon == 0) {
		int pipefd[2];      // Stores the pipe's fds:
				    //	- pipefd[0]: read only
				    //	- pipefd[1]: write only

		if (pipe(pipefd) == -1) {
			fprintf(stderr, "ERROR: pipe has failed!\n");  
			exit(EXIT_FAILURE);
		}		
		
		close(pipefd[1]); 
			

		pid_t monitor_reports = fork(); 

		if(monitor_reports < 0) {
			fprintf(stderr, "ERROR: monitor_reports in city_hub has failed to execute!\n"); 
		}

		else if(monitor_reports == 0) {
			if(dup2(pipefd[0], STDOUT_FILENO) < 0) {
				fprintf(stderr, "ERROR: dup2 has failed!\n"); 	
				exit(EXIT_FAILURE); 
			}	

			close(pipefd[1]); 
			if(open(".monitor_pid", O_RONLY) == -1) {
				writestr(STDOUT_FILENO, "ERROR: this process already runs!\n"); 
				exit(EXIT_FAILURE); 
			}
			execl("./monitor_reports", "./monitor_reports", NULL); 

		}

		else {
			close(pipefd[0]); 
			close(pipefd[1]); 
		}

		// execute steps 6 and 7 
		char* ERROR = "ERROR"; 
		char buffer[10]; 

		while(read(pipefd[0], &buffer, 5) == 5) {
			if(strcmp(buffer, ERROR) == 0) {
				fprintf(stderr, "ERROR: unexpected finish has occured!\n"); 
				exit(EXIT_FAILURE); 
			}
		}
	}

	else {
		write(STDOUT_FILENO, "Parent: hub_mon has started\n"); 
		wait(NULL); 
		writestr(STDOUT_FILENO, "Parent: we have finished executing monitor_reports!\n"); 
	}
}

/*
 start_monitor()
	 1. Create background childprocess - let's call it hub_mon 
	 2. in hub_mon, create a pipe - it reads the output from stdout  
	 3. in hub_mon, fork the monitor_reports (it must execute it)
	 4. when executed the monitor_reports, first we verify if another monitor_reports is already running 
	 5. if running, we send thorugh the pipe and error message that specifies the existing monitor's id then ends 
	 6. if it was running, the hub_mon reads from the read end, displays the message to the user as soon as available. 
	 7. if we haven't encountered an error at startup - but stil execution of the monitor has ended for any reason, print a specific message to the user 

	 https://www.youtube.com/watch?v=hi4Yitv-M28
*/
