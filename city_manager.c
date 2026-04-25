#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

typedef enum {
    MANAGER=100007, 
    INSPECTOR=2100007
} Role; 

// since these are unsigned values, I can use switch 
typedef enum {
    ADD, 
    LIST, 
    VIEW, 
    REMOVE_REPORT, 
    UPDATE_THRESHOLD, 
    FILTER
} CommandType; 

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

typedef struct {
    Role role; 
    char user[256];
    CommandType command; 
    char district_id[256];

    // sometimes optional
    int report_id; 
    char** conditions; 
    size_t conditions_count; 
} Command; 


// =============== COMMANDS ===============
void add(Command* cmd) {
    
}

void view(Command* cmd) {
    return; 
}

void remove_report(Command* cmd) {
    return; 
}

void update_threshold(Command* cmd) {
    return; 
}

void filter(Command* cmd) {
    return; 
}

void view(Command* cmd) {
    size_t BUFFER_SIZE = 256;

	char filename[BUFFER_SIZE]; 
    size_t bytes_written = snprintf(filename, sizeof(filename), "%s/%s", cmd->district_id, "reports.dat"); 	
    if(bytes_written >= BUFFER_SIZE) {
        fprintf(stderr, "ERROR: name is too long!\n");
        exit(EXIT_FAILURE);
    }

    int fd; 

	if((fd = open(filename, O_RDONLY)) == -1) {
		fprintf(stderr, "ERROR: you couldn't open the file!\n"); 
        exit(EXIT_FAILURE); 
	}
    
    Report r; 
	while(read(fd, &r, sizeof(Report) == sizeof(Report))) {
        if(cmd->reports_id == r-)
        printf("ID: %d, Value: %.2f, Time: %s\n", r.id, r.value, r.timestamp);
	}

	close(fd); 
}

// =============== HELPERS =============== 

// high technical debt command, but it is what it is to keep single repsponsiblitiy 
void (*detect_command(Command* cmd))(Command *) {	
    switch (cmd->command) {
        case ADD:
            return add;
        case LIST:
            return list; 
        case VIEW:
            return view; 
        case REMOVE_REPORT:
            return remove_report; 
        case UPDATE_THRESHOLD:
            return update_threshold;
        case FILTER:
            return filter;
    }
    return NULL; 
}

Command parse_arguments(int argc, char** argv) {
    Command cmd = {0};

    cmd.report_id = -1; 
    cmd.conditions = NULL; 
    cmd.conditions_count = 0;

    // safety 
    if(argc < 6) {
        fprintf(stderr, "ERROR: invalid count of arguments!\n");
        exit(EXIT_FAILURE);
    }

    // role
    if(strcmp(argv[1], "--role") != 0)) {
        fprintf(stderr, "ERROR: missing --role\n");
        exit(EXIT_FAILURE);
    }

    if(strcmp(argv[2], "manager") == 0) {
        cmd.role = MANAGER;
    }
    else if (strcmp(argv[2], "inspector") == 0) {
        cmd.role = INSPECTOR;
    }
    else {
        fprintf(stderr, "ERROR: invalid role!\n");
        exit(EXIT_FAILURE);
    }
    
    // user name 
    if(strcmp(argv[3], "--user") != 0) {
        fprintf(stderr, "ERROR: missing --user\n");
        exit(EXIT_FAILURE);
    }

    strncpy(cmd.user, argv[4], sizeof(cmd.user) - 1);
    cmd.user[sizeof(cmd.user) - 1] = '\0';

    // command type 
    if(strcmp(argv[5], "--list") == 0) {
        if(argc != 7) exit(EXIT_FAILURE);

        cmd.command = LIST;  
        strncpy(cmd.district_id, argv[6], sizeof(cmd.district_id) - 1);
        cmd.district_id[sizeof(cmd.district_id) - 1] = '\0';
   
      }

    else if(strcmp(argv[5], "--add") == 0) {
        if(argc != 6) exit(EXIT_FAILURE);

        cmd.command = ADD; 
        strncpy(cmd.district_id, argv[6], sizeof(cmd.district_id) - 1);
        cmd.district_id[sizeof(cmd.district_id) - 1] = '\0';
    }

    else if(strcmp(argv[5], "--view") == 0) {
        if(argc != 8) exit(EXIT_FAILURE)

        cmd.command = VIEW;
        strncpy(cmd.district_id, argv[6], sizeof(cmd.district_id) - 1);
        cmd.district_id[sizeof(cmd.district_id) - 1] = '\0';

        char *endptr;
        long val = strtol(argv[7], &endptr, 10):
        if(endptr == argv[7]) {
            fprintf(stderr, "ERROR: %s is not a valid number", argv[7]);
            exit(EXIT_FAILURE);
        }

        if(errno == ERANGE || val > INT_MAX || val < INT_MIN) {
            fprintf(stderr, "ERROR: severity value out of range!\n");
            exit(EXIT_FAILURE);
        }

        cmd.report_id = (int)val;
    }

    else if(strcmp(argv[5], "--remove_report") == 0) {
        cmd.command = REMOVE_REPORT;
        strncpy(cmd.district_id, argv[6], sizeof(cmd.district_id) - 1);
        cmd.district_id[sizeof(cmd.district_id) - 1] = '\0';

        char *endptr;
        long val = strtol(argv[7], &endptr, 10):
        if(endptr == argv[7]) {
            fprintf(stderr, "ERROR: %s is not a valid number", argv[7]);
            exit(EXIT_FAILURE);
        }

        if(errno == ERANGE || val > INT_MAX || val < INT_MIN) {
            fprintf(stderr, "ERROR: severity value out of range!\n");
            exit(EXIT_FAILURE);
        }

        cmd.report_id = (int)val;
    }

    else if(strcmp(argv[5], "--update_threshold") == 0) {
        if(argc != 8) exit(EXIT_FAILURE);

        cmd.command = UPDATE_THRESHOLD;
        strncpy(cmd.district_id, argv[6], sizeof(cmd.district_id) - 1);
        cmd.district_id[sizeof(cmd.district_id) - 1] = '\0';

        char *endptr;
        long val = strtol(argv[7], &endptr, 10):
        if(endptr == argv[7]) {
            fprintf(stderr, "ERROR: %s is not a valid number", argv[7]);
            exit(EXIT_FAILURE);
        }

        if(errno == ERANGE || val > INT_MAX || val < INT_MIN) {
            fprintf(stderr, "ERROR: severity value out of range!\n");
            exit(EXIT_FAILURE);
        }

        cmd.severity = (int)val;
    }

    else if (strcmp(argv[5], "--filter") == 0) {
        if(argc < 8){
            fprintf(stderr, "ERROR: no conditions, no result!\n");
            exit(EXIT_FAILURE);
        }

        cmd.command = FILTER;
        strncpy(cmd.district_id, argv[6], sizeof(cmd.district_id) - 1);
        cmd.district_id[sizeof(cmd.district_id) - 1] = '\0';

        cmd.conditions = &argv[7];
        cmd.conditions_count = argc - 7; 
    }

    else {
        fprintf(stderr, "ERROR: unknown command\n");
        exit(EXIT_FAILURE);
    }

    return cmd; 
}

char* get_path_name(char district_id[], char filename[]) {

}

int main(int argc, char** argv) {
	if(argc < 7) {
		// write(stderr, "ERROR: you have not specified the role and the command!\n", 60); 
		fprintf(stderr, "ERROR: not enough arguments!");  
		exit(EXIT_FAILURE);
	}

    Command cmd_line = parse_arguments(argc, argv)

    void (*command)(Comand*) = detect_command(&cmd_line);
    if(command == NULL) {
        fprintf(stderr, "ERROR: unknown command!\n");
        exit(EXIT_FAILURE);
    }

	command(cmd_line); 
	return 0; 
}
