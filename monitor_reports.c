#define _POSIX_C_SOURCE 200809L
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

volatile sig_atomic_t running = 1;

void sigint_handler(int signal) {
	if(signal == SIGINT) {
		char message[] = "MONITOR_END Terminating the program!\n";
		write(STDOUT_FILENO, message, sizeof(message) - 1);
		running = 0;
	}
}

void sigusr1_handler(int signal) {
	if(signal == SIGUSR1) {
		char message[] = "MONITOR_NEW_REPORT A new report has been added to the program!\n";
		write(STDOUT_FILENO, message, sizeof(message) - 1);
	}
}

void set_sigusr1_action() {
	struct sigaction act;

	memset(&act, 0, sizeof(act));
	act.sa_handler = &sigusr1_handler;

	if(sigaction(SIGUSR1, &act, NULL) == -1) {
		fprintf(stderr, "ERROR: couldn't set SIGUSR1 handler!\n");
		exit(EXIT_FAILURE);
	}
}

void set_sigint_action() {
	struct sigaction act;

	memset(&act, 0, sizeof(act));
	act.sa_handler = &sigint_handler;

	if(sigaction(SIGINT, &act, NULL) == -1) {
		fprintf(stderr, "ERROR: couldn't set SIGINT handler!\n");
		exit(EXIT_FAILURE);
	}
}

int read_pid_from_fd(int fd, pid_t *pid) {
	char buffer[64];
	ssize_t bytes_read;

	bytes_read = read(fd, buffer, sizeof(buffer) - 1);

	if(bytes_read < 0) {
		return -1;
	}

	if(bytes_read == 0) {
		return -1;
	}

	buffer[bytes_read] = '\0';

	char *endptr;
	errno = 0;

	long value = strtol(buffer, &endptr, 10);

	if(errno != 0 || endptr == buffer || value <= 0) {
		return -1;
	}

	*pid = (pid_t)value;
	return 0;
}

int main() {
	int BUFFER_SIZE = 64;
	char monitor_file[BUFFER_SIZE];

	snprintf(monitor_file, sizeof(monitor_file), ".monitor_pid");

	int fd;

	if((fd = open(monitor_file, O_RDONLY)) != -1) {
		char message[256];
		pid_t old_pid;

		int status_code = read_pid_from_fd(fd, &old_pid);

		if(status_code < 0) {
			char error_message[] = "MONITOR_ERROR invalid value in the PID file!\n";
			write(STDOUT_FILENO, error_message, sizeof(error_message) - 1);
			close(fd);
			exit(EXIT_FAILURE);
		}

		snprintf(message, sizeof(message), "MONITOR_ERROR another process is already running! PID = %d\n", (int)old_pid);
		write(STDOUT_FILENO, message, strlen(message));

		close(fd);
		exit(EXIT_FAILURE);
	}

	fd = open(monitor_file, O_WRONLY | O_TRUNC | O_CREAT, 0644);

	if(fd == -1) {
		fprintf(stderr, "MONITOR_ERROR couldn't create or open the .monitor_pid file!\n");
		exit(EXIT_FAILURE);
	}

	char pid_string[BUFFER_SIZE];

	int bytes_written = snprintf(pid_string, sizeof(pid_string), "%d\n", (int)getpid());

	if(bytes_written < 0 || bytes_written >= BUFFER_SIZE) {
		fprintf(stderr, "MONITOR_ERROR PID string is too long!\n");
		close(fd);
		exit(EXIT_FAILURE);
	}

	if(write(fd, pid_string, bytes_written) != bytes_written) {
		fprintf(stderr, "MONITOR_ERROR couldn't write that pid inside of .monitor_pid file!\n");
		close(fd);
		exit(EXIT_FAILURE);
	}

	close(fd);

	char start_message[128];
	int len = snprintf(start_message, sizeof(start_message), "MONITOR_START pid = %d\n", (int)getpid());

	if(len > 0) {
		write(STDOUT_FILENO, start_message, len);
	}

	set_sigint_action();
	set_sigusr1_action();

	while(running) {
		pause();
	}

	if(unlink(monitor_file) == -1) {
		fprintf(stderr, "MONITOR_ERROR couldn't delete the .monitor_pid file!\n");
		exit(EXIT_FAILURE);
	}

	return 0;
}
