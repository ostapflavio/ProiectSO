#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define MANAGER 100007
#define INSPECTOR 2100007

void add(char** line) {
		 
}



void list(char** line) {
	char filename[60]; 
	snprintf(filename, sizeof(filename), "%s/%s", line[6], "reports.dat"); 	
	int fd; 
	if((fd = open(filename, O_RDONLY)) == -1) {
		fprintf(stderr, "ERROR: you couldn't open the file!\n"); 
		exit(EXIT_FAILURE); 
	}

	char buffer[256]; 
	size_t bytes_read;
	while((bytes_read = read(fd, buffer, 256)) > 0) {
		write(STDOUT_FILENO, buffer, bytes_read); 
	}

	close(fd); 
}

void (*detect_command(char** line))(char**) {	
	if(strcmp(line[5], "--list") == 0) {
		return list; 
	}

	return NULL; 
}

int main(int argc, char** argv) {
	/*
	Conventions:
	 -- argv[0] - name of the progrma 
	 -- argv[1] - the --role argument 
	 -- argv[2] - value for --role parameter 
	 -- argv[3] - the --user argument  
	 -- argv[4] - value for --user parameter 
	 -- argv[5] - command
	 -- argv[6] - argument for the command  
	*/

	if(argc < 4) {
		// write(stderr, "ERROR: you have not specified the role and the command!\n", 60); 
		fprintf(stderr, "ERROR: you have not specified the role and the command!\n"); 
		exit(EXIT_FAILURE);
	}			

	list(argv); 
	return 0; 
}
