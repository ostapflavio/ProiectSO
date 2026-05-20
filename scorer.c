#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_REPORTS_COUNT 256 

typedef struct {
	int workload_score; 
	char inspector[64]; 	
} Score; 

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

int main(int argc, char* argv[]) {
	if(argc != 2) {
		fprintf(stderr, "ERROR: incorrect argument count!\n"); 
		exit(EXIT_FAILURE); 
	}

	Score scorer[MAX_REPORTS_COUNT]; 
	int pointer_scorer = 0; 

	// for each district - calculate the score
	size_t BUFFER_SIZE = 256; 
	char filename[BUFFER_SIZE]; 
	size_t bytes_written; 
	int fd; 
	int found_any = 0; 
	Report r; 

	// start the loop one by one 
	ssize_t bytes_read; 
	bytes_written = snprintf(filename, sizeof(filename), "%s/reports.dat", argv[1]); 
	if(bytes_written >= BUFFER_SIZE) {
		fprintf(stderr, "ERROR: name is too long!\n"); 
		exit(EXIT_FAILURE); 
	}

	if((fd = open(filename, O_RDONLY)) == -1) {
		fprintf(stderr, "ERROR: you couldn't open the file!\n"); 
		exit(EXIT_FAILURE); 
	}
	

	while((bytes_read = read(fd, &r, sizeof(Report))) == sizeof(Report)) {
		found_any = 1; 

		int new_inspector = 1; 
		int pointer_for_inspector = pointer_scorer; 
		for(int i = 0; i < pointer_scorer; i++) {
			if(strcmp(scorer[i].inspector, r.inspector) == 0) {
				new_inspector = 0;  
				pointer_for_inspector = i; 
				break; 
			}
		}

		if(new_inspector) {
			pointer_scorer++; 
			size_t inspector_size = sizeof(scorer[pointer_for_inspector].inspector); 
			strncpy(scorer[pointer_for_inspector].inspector, r.inspector, inspector_size - 1); 
			scorer[pointer_for_inspector].inspector[inspector_size] = '\0'; 
		}

		scorer[pointer_for_inspector].workload_score += r.severity; 
	}

	if(bytes_read != 0) {
		fprintf(stderr, "ERROR: reports.dat contains an incomplete report!\n"); 
		close(fd); 
		exit(EXIT_FAILURE); 
	}

	if(!found_any) {
		printf("No reports found. \n"); 
	}

	close(fd); 

	// print out the summary 
	for(int i = 0; i < pointer_scorer; i++) {
		printf("%s - %d\n", scorer[i].inspector, scorer[i].workload_score); 
	}

	return 0; 
}
