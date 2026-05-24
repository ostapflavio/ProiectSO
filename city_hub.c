#define _POSIX_C_SOURCE 200809L // avoid unnecessary warning, by specifying what version of POSIX functions we use
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_ARGS 64

void writestr(int fd, const char *str) {
	write(fd, str, strlen(str));
}

void run_modified_monitor() {
	int pipefd[2];

	if(pipe(pipefd) == -1) {
		fprintf(stderr, "ERROR: pipe has failed!\n");
		exit(EXIT_FAILURE);
	}

	pid_t monitor_reports = fork();

	if(monitor_reports < 0) {
		fprintf(stderr, "ERROR: monitor_reports in city_hub has failed to execute!\n");
		close(pipefd[0]);
		close(pipefd[1]);
		exit(EXIT_FAILURE);
	}

	else if(monitor_reports == 0) {
		close(pipefd[0]);

		if(dup2(pipefd[1], STDOUT_FILENO) < 0) {
			fprintf(stderr, "ERROR: dup2 has failed!\n");
			close(pipefd[1]);
			exit(EXIT_FAILURE);
		}

		close(pipefd[1]);

		execl("./monitor_reports", "./monitor_reports", NULL);

		fprintf(stderr, "ERROR: execl has failed!\n");
		exit(EXIT_FAILURE);
	}

	else {
		close(pipefd[1]);

		char buffer[256];
		ssize_t n;

		while((n = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
			buffer[n] = '\0';

			writestr(STDOUT_FILENO, buffer);

			if(strstr(buffer, "MONITOR_ERROR") != NULL) {
				writestr(STDERR_FILENO, "hub_mon: monitor reported an error!\n");
			}

			if(strstr(buffer, "MONITOR_END") != NULL) {
				writestr(STDERR_FILENO, "hub_mon: monitor_reports has ended!\n");
			}
		}

		close(pipefd[0]);

		int status;
		if(waitpid(monitor_reports, &status, 0) == -1) {
			fprintf(stderr, "ERROR: waitpid failed!\n");
			exit(EXIT_FAILURE);
		}

		writestr(STDOUT_FILENO, "hub_mon: monitor_reports finished for some reason.\n");
		exit(EXIT_SUCCESS);
	}
}

void start_monitor() {
	pid_t hub_mon;

	hub_mon = fork();

	if(hub_mon < 0) {
		fprintf(stderr, "ERROR: couldn't start a hub_mon!\n");
		exit(EXIT_FAILURE);
	}

	else if(hub_mon == 0) {
		run_modified_monitor();
	}

	else {
		writestr(STDOUT_FILENO, "Parent: hub_mon has started\n");
	}
}

int district_exists(char *district) {
	struct stat st;

	if(stat(district, &st) == -1) {
		return 0;
	}

	return S_ISDIR(st.st_mode);
}

void calculate_score(int count_of_districts, char* list_of_districts[]) {
	if(count_of_districts < 2) {
		fprintf(stderr, "ERROR: calculate_scores requires at least one district!\n");
		return;
	}

	writestr(STDOUT_FILENO, "COMBINED WORKLOAD REPORT\n");

	int counter = 1;

	while(counter < count_of_districts) {
		if(!district_exists(list_of_districts[counter])) {
			fprintf(stderr, "ERROR: district doesn't exist, skipping: %s\n", list_of_districts[counter]);
			counter++;
			continue;
		}

		int pipefd[2];

		if(pipe(pipefd) == -1) {
			fprintf(stderr, "ERROR: pipe has failed!\n");
			exit(EXIT_FAILURE);
		}

		pid_t pid = fork();

		if(pid < 0) {
			fprintf(stderr, "ERROR: couldn't start a new process!\n");
			close(pipefd[0]);
			close(pipefd[1]);
			exit(EXIT_FAILURE);
		}

		else if(pid == 0) {
			close(pipefd[0]);

			if(dup2(pipefd[1], STDOUT_FILENO) < 0) {
				fprintf(stderr, "ERROR: dup2 failed!\n");
				close(pipefd[1]);
				exit(EXIT_FAILURE);
			}

			close(pipefd[1]);

			execl("./scorer", "./scorer", list_of_districts[counter], NULL);

			fprintf(stderr, "ERROR: exec failed!\n");
			exit(EXIT_FAILURE);
		}

		else {
			close(pipefd[1]);

			writestr(STDOUT_FILENO, "\nDistrict: ");
			writestr(STDOUT_FILENO, list_of_districts[counter]);
			writestr(STDOUT_FILENO, "\n");

			char buffer[256];
			ssize_t n;

			while((n = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
				buffer[n] = '\0';
				writestr(STDOUT_FILENO, buffer);
			}

			close(pipefd[0]);

			int status;

			if(waitpid(pid, &status, 0) == -1) {
				fprintf(stderr, "ERROR: waitpid failed!\n");
				exit(EXIT_FAILURE);
			}

			if(WIFEXITED(status) && WEXITSTATUS(status) != 0) {
				fprintf(stderr, "ERROR: scorer failed for district: %s\n", list_of_districts[counter]);
			}

			counter++;
		}
	}
}

void remove_newline(char *str) {
	str[strcspn(str, "\n")] = '\0';
}

int parse_command(char *command, char *args[]) {
	int count = 0;

	char *token = strtok(command, " ");

	while(token != NULL && count < MAX_ARGS - 1) {
		args[count] = token;
		count++;

		token = strtok(NULL, " ");
	}

	args[count] = NULL;

	return count;
}

int main() {
	char command[512];
	char *args[MAX_ARGS];

	writestr(STDOUT_FILENO, "city_hub started.\n");
	writestr(STDOUT_FILENO, "Available commands: start_monitor, calculate_scores <districts>, exit\n");

	while(1) {
		writestr(STDOUT_FILENO, "city_hub> ");

		if(fgets(command, sizeof(command), stdin) == NULL) {
			writestr(STDOUT_FILENO, "\nExiting city_hub.\n");
			break;
		}

		remove_newline(command);

		int count = parse_command(command, args);

		if(count == 0) {
			continue;
		}

		if(strcmp(args[0], "start_monitor") == 0) {
			start_monitor();
		}

		else if(strcmp(args[0], "calculate_score") == 0) {
			calculate_score(count, args);
		}

		else if(strcmp(args[0], "calculate_scores") == 0) {
			calculate_score(count, args);
		}

		else if(strcmp(args[0], "exit") == 0 || strcmp(args[0], "quit") == 0) {
			writestr(STDOUT_FILENO, "Exiting city_hub.\n");
			break;
		}

		else {
			writestr(STDERR_FILENO, "ERROR: unknown command!\n");
		}
	}

	return 0;
}
