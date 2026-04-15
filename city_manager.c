#include <stdio.h>
#include <stdlib.h>

#define MANAGER 100007
#define INSPECTOR 2100007

void add(char** line) {
	return; 
}

void (*detect_command(char**))(char** line) {	
	return (add); 
}

void list(char** line) {
	int district_id = argv[6]; 

	
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
		fprintf(stderr, "ERROR: you have not specified the role and the command!\n"); 
		exit(EXIT_FAILURE);
	}			

	detect_command(argv);

	return 0; 
}

