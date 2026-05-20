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

void run_modified_monitor() {
	int pipefd[2]; // Stores the pipe's fds:
		       // - pipefd[0]: read only
		       // - pipefd[1]: write only

		       // 1. create a pipe
	if (pipe(pipefd) == -1) {
		fprintf(stderr, "ERROR: pipe has failed!\n");
		exit(EXIT_FAILURE);
	}



	// 2. create a process that will be reponsible for monitor_reports
	pid_t monitor_reports = fork();

	if(monitor_reports < 0) {
		fprintf(stderr, "ERROR: monitor_reports in city_hub has failed to execute!\n");
		close(pipefd[0]);
		close(pipefd[1]);
		exit(EXIT_FAILURE);
	}

	else if(monitor_reports == 0) {
		close(pipefd[0]);

		// 3. redirect monitor_reports OUTPUT from terminal TO THE PIPE;
		// before that we ned to close unued pipe end
		if(dup2(pipefd[1], STDOUT_FILENO) < 0) {
			fprintf(stderr, "ERROR: dup2 has failed!\n");
			exit(EXIT_FAILURE);
		}
		close(pipefd[1]); // now everytime we call STDOUT_FILENO,
				  // output will be pointing to the write end of the pipe
		int fd;
		if((fd = open(".monitor_pid", O_RDONLY)) != -1) {
			writestr(STDOUT_FILENO, "ERROR: this process already runs!\n");
			close(fd);
			exit(EXIT_FAILURE);
		}

		// 4. if monitor_reports isn't running, run it
		execl("./monitor_reports", "./monitor_reports", NULL);

		fprintf(stderr, "ERROR: execl has failed!\n");
		exit(EXIT_FAILURE);
	}

	else {
		// 5. now, hub_mon will be monitoring for potential errors
		close(pipefd[1]); // hub_mopn does not write anything.

		char buffer[256];
		ssize_t n;

		while((n = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
			buffer[n] = '\0';

			writestr(STDOUT_FILENO, buffer);

			if(strstr(buffer, "ERROR") != NULL) {
				writestr(STDERR_FILENO, "hub_mon: monitor reported an error!\n");
			}

			if(strstr(buffer, "MONITOR_END") != NULL) {
				writestr(STDERR_FILENO, "hub_mon: monitor_reports has ended!!\n");
			}
		}

		// 6. the process either finihed,
		// either is alive BUT it doesn't print anything to STDOUT,
		// we have reached the end of the hub_mon
		close(pipefd[0]);
		waitpid(monitor_reports, NULL, 0); // don't end with zombie proceses
		writestr(STDOUT_FILENO, "hub_mon: monitor_reports finished for some reason....\n");
		exit(EXIT_FAILURE);
	}
}

void start_monitor() {
	pid_t hub_mon;

	hub_mon = fork();
	if (hub_mon < 0) {
		fprintf(stderr, "ERROR: couldn't start a hub_mon!\n");
		exit(EXIT_FAILURE);
	}

	else if (hub_mon == 0) {
		run_modified_monitor(); // it is run inside the memory space of hub_mon
	}

	else {
		writestr(STDOUT_FILENO, "Parent: hub_mon has started\n");
	}
}

void calculate_score(int count_of_districts, char* list_of_districts[]) {
	int pipefd[2]; 

	pid_t pid = fork(); 
	if(pid < 0) {
		fprintf(stderr, "ERROR: couldn't start a new process!\n"); 
		exit(EXIT_FAILURE); 
	}	

	else if(pid == 0) {
		int counter = 1; 
		while(counter < count_of_districts) {
			close(pipefd[0]); 
			if(pipe(pipefd) == -1) {
				fprintf(stderr, "ERROR: pipe has failed!\n");
				exit(EXIT_FAILURE); 
			}

			if(dup2(pipefd[1], STDOUT_FILENO) < 0) {
				fprintf(stderr, "ERROR: dup2 has failed!\n");
				exit(EXIT_FAILURE);
			}
			close(pipefd[1]); // now everytime we call STDOUT_FILENO,
					  // output will be pointing to the write end of the pipe

			execl("./scorer", "./scorer", list_of_districts[counter++], NULL);
			fprintf(stderr, "ERROR: exec failed!\n"); 
			exit(EXIT_FAILURE); 
		}
	}

	else {
		close(pipefd[1]); // hub_mopn does not write anything.
		
		writestr(STDOUT_FILENO, "SUMMARY:\n"); 
		char buffer[256];
		ssize_t n;

		while((n = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
			buffer[n] = '\0';

			writestr(STDOUT_FILENO, buffer);
		}

		// 6. the process either finihed,
		// either is alive BUT it doesn't print anything to STDOUT,
		// we have reached the end of the hub_mon
		close(pipefd[0]);
		waitpid(pid, NULL, 0); // don't end with zombie proceses
	}
}

void remove_newline(char *str) {
	str[strcspn(str, "\n")] = '\0';
}

int main(int argc, char** argv) {
	char command[256];

	writestr(STDOUT_FILENO, "city_hub started.\n");
	writestr(STDOUT_FILENO, "Available commands: start_monitor, calculate_score, exit\n");

	while(1) {
		writestr(STDOUT_FILENO, "city_hub> ");

		if(fgets(command, sizeof(command), stdin) == NULL) {
			writestr(STDOUT_FILENO, "\nExiting city_hub.\n");
			break;
		}

		remove_newline(command);

		if(strcmp(command, "start_monitor") == 0) {
			start_monitor();
		}

		if(strcmp(command, "calculate_score") == 0) {
			calculate_score(argc, argv); 
		}

		else if(strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
			writestr(STDOUT_FILENO, "Exiting city_hub.\n");
			break;
		}

		else if(strlen(command) == 0) {
			continue;
		}

		else {
			writestr(STDERR_FILENO, "ERROR: unknown command!\n");
		}
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
