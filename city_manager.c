#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define MANAGER 100007
#define INSPECTOR 2100007

typedef struct {
    int id; 
    char inspector[64];
    double latitude; 
    double longitude; 
    char category[32];
    int severity; 
    time_t timestamp; 
    char description[256];
} Report; 


typedef struct {
    
} Command; 
void add(char** line) {
	return; 
}

void view(char** line) {
    return; 
}

void remove_report(char** line) {
    return; 
}

void update_threshold(char** line) {
    return; 
}

void filter(char** line) {
    return; 
}

void list(char** line) {
    size_t BUFFER_SIZE = 256;

	char filename[BUFFER_SIZE]; 
    size_t bytes_written = snprintf(filename, sizeof(filename), "%s/%s", line[6], "reports.dat"); 	
    if(bytes_written >= BUFFER_SIZE) {

    }
    int fd; 

	if((fd = open(filename, O_RDONLY)) == -1) {
		fprintf(stderr, "ERROR: you couldn't open the file!\n"); 
		exit(EXIT_FAILURE); 
	}

	char buffer[BUFFER_SIZE]; 
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
    if(strcmp(line[5], "--add") == 0) {
        return add; 
    }
    if(strcmp(line[5], "--remove_report") == 0) {
        return remove_report; 
    }
    if(strcmp(line[5], "--update_threshold") == 0) {
        return update_threshold; 
    }
    if(strcmp(line[5], "--filter") == 0) {
        return filter; 
    }
	return NULL; 
}

int main(int argc, char** argv) {
	/*
	Conventions:
	 -- argv[0] - name of the progrma 
     -- argv[2] - value for --role parameter 
     -- argv[3] - the --user argument  
     -- argv[4] - value for --user parameter 
     -- argv[5] - command
     -- argv[6] - argument for the command  
	 -- argv[1] - the --role argument 
	*/

	if(argc < 7) {
		// write(stderr, "ERROR: you have not specified the role and the command!\n", 60); 
		fprintf(stderr, "ERROR: not enough arguments!");  
		exit(EXIT_FAILURE);
	}


    void (*command)(char**) = detect_command(argv);
    if(command == NULL) {
        fprintf(stderr, "ERROR: unknown command!\n");
        exit(EXIT_FAILURE);
    }

	command(argv); 
	return 0; 
}
