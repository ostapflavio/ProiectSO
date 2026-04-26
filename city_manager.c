#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>

#define ACCESS_READ 4 
#define ACCESS_WRITE 2 
#define ACCESS_EXEC 1 

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
    int severity;
} Command; 

// =============== PROTOTYPES ===============
int get_next_report_id(char* path);
void check_permissions(char path[], Role role, int access_type);
void log_action(Command* cmd, char action[]);
char* role_to_string(Role role);

// =============== COMMANDS ===============
// TODO: add logger
void add(Command* cmd) {
    size_t BUFFER_SIZE = 256; 

    char path[BUFFER_SIZE];
    size_t bytes_written = snprintf(path, sizeof(path), "%s", cmd->district_id);

    if(bytes_written >= BUFFER_SIZE) {
        fprintf(stderr, "ERROR: overwrite!\n");
        exit(EXIT_FAILURE);
    }

    struct stat stat_buff;  
    if(stat(path, &stat_buff) == -1) {
        if(mkdir(path, 0750) != 0) {
            fprintf(stderr, "ERROR: couldn't create the directory!\n");
            exit(EXIT_FAILURE);
        }

        chmod(path, 0750);
    }
    
    bytes_written = snprintf(path, sizeof(path), "%s/reports.dat", cmd->district_id);
    if(bytes_written >= BUFFER_SIZE) {
        fprintf(stderr, "ERROR: overwrite!\n");
        exit(EXIT_FAILURE);
    }
    check_permissions(cmd->district_id, cmd->role, ACCESS_WRITE);
    check_permissions(path, cmd->role, ACCESS_WRITE);

    Report r;
    int fd; 
    int threshold; 

    memset(&r, 0, sizeof(Report));

    r.id = get_next_report_id(path);
    strncpy(r.inspector, cmd->user, sizeof(r.inspector) - 1);
    r.inspector[sizeof(r.inspector) - 1] = '\0';

    printf("Latitude: ");
    if(scanf("%lf", &r.latitude) != 1) {
        fprintf(stderr, "ERROR: invalid latitude!\n");
        exit(EXIT_FAILURE);
    }

    printf("Longitude: ");
    if(scanf("%lf", &r.longitude) != 1) {
        fprintf(stderr, "ERROR: invalid longitude!\n");
        exit(EXIT_FAILURE);
    }

    printf("Category (road/lighting/flooding/other): ");
    if(scanf("%31s", r.category) != 1) {
        fprintf(stderr, "ERROR: invalid category!\n");
        exit(EXIT_FAILURE);
    }

    printf("Severity level (1/2/3): ");
    if(scanf("%d", &r.severity) != 1) {
        fprintf(stderr, "ERROR: invalid severity!\n");
        exit(EXIT_FAILURE);
    }

    if(r.severity < 1 || r.severity > 3) {
        fprintf(stderr, "ERROR: severity must be 1, 2, or 3!\n");
        exit(EXIT_FAILURE);
    }

    // remove the remaining newline before fgets
    int ch;
    while((ch = getchar()) != '\n' && ch != EOF) {
    }

    printf("Description: ");
    if(fgets(r.description, sizeof(r.description), stdin) == NULL) {
        fprintf(stderr, "ERROR: invalid description!\n");
        exit(EXIT_FAILURE);
    }

    r.description[strcspn(r.description, "\n")] = '\0';
    r.timestamp = time(NULL);

    fd = open(path, O_WRONLY | O_APPEND);
    if(fd == -1) {
        if(errno == ENOENT) {
            return 1;
        }
        perror("ERROR: couldn't open reports.dat");
        exit(EXIT_FAILURE);
    }

    if(write(fd, &r, sizeof(Report)) != sizeof(Report)) {
        perror("ERROR: couldn't write report");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
    chmod(path, 0664);

    if(r.id == 1) {
        snprintf(path, sizeof(path), "%s/district.cfg", cmd->district_id);
        FILE* f = fopen(path, "w"); 

        if(f != NULL) {
            fprintf(f, "%d", 3);
            fclose(f);
            chmod(path, 0640);
        }
        else {
            fprintf(stderr, "ERROR: Failed district.cfg!\n");
            exit(EXIT_FAILURE);
        }
    }

    log_action(cmd, "add");
}


void list(Command* cmd) {
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
	while(read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        if(cmd->report_id == r.id) {
            printf("id = %d \n", r.id);
            printf("inspector = %s \n", r.inspector);
            printf("lat = %f \n", r.latitude);
            printf("long = %f \n", r.longitude);
            printf("cat = %s \n", r.category);
            printf("severity = %d \n", r.severity);
            printf("timestamp = %lld \n", (long long)r.timestamp);
        }
            printf("description = %s \n", r.description);
	}

	close(fd); 

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
	while(read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        if(cmd->report_id == r.id) {
            printf("id = %d \n", r.id);
            printf("inspector = %s \n", r.inspector);
            printf("lat = %f \n", r.latitude);
            printf("long = %f \n", r.longitude);
            printf("cat = %s \n", r.category);
            printf("severity = %d \n", r.severity);
            printf("timestamp = %lld \n", (long long)r.timestamp);
            printf("description = %s \n", r.description);
        }
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
    if(strcmp(argv[1], "--role") != 0) {
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
        if(argc != 7) exit(EXIT_FAILURE);

        cmd.command = ADD; 
        strncpy(cmd.district_id, argv[6], sizeof(cmd.district_id) - 1);
        cmd.district_id[sizeof(cmd.district_id) - 1] = '\0';
    }

    else if(strcmp(argv[5], "--view") == 0) {
        if(argc != 8) exit(EXIT_FAILURE);

        cmd.command = VIEW;
        strncpy(cmd.district_id, argv[6], sizeof(cmd.district_id) - 1);
        cmd.district_id[sizeof(cmd.district_id) - 1] = '\0';

        char *endptr;
        long val = strtol(argv[7], &endptr, 10);
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
        if(argc != 8) exit(EXIT_FAILURE);
        cmd.command = REMOVE_REPORT;
        strncpy(cmd.district_id, argv[6], sizeof(cmd.district_id) - 1);
        cmd.district_id[sizeof(cmd.district_id) - 1] = '\0';

        char *endptr;
        long val = strtol(argv[7], &endptr, 10);
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
        long val = strtol(argv[7], &endptr, 10);
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

int get_next_report_id(char* path) {
    int fd; 
    Report r; 
    size_t bytes_read; 
    int max_id = 0; 

    fd = open(path, O_RDONLY);
    if(fd == -1) {
        if(errno == ENOENT) {
            return 1;
        }

        fprintf(stderr, "ERROR: couldn't open reports.dat");
        exit(EXIT_FAILURE);
    }

    while((bytes_read = read(fd, &r, sizeof(Report))) == sizeof(Report)) {
        if(r.id > max_id) {
            max_id = r.id;
        }
    }

    if(bytes_read == -1) {
        fprintf(stderr, "ERROR: random error!\'n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
    return max_id + 1;
}

void mode_to_string(mode_t mode, char output[]) {
    output[0] = (mode & S_IRUSR) ? 'r' : '-';
    output[1] = (mode & S_IWUSR) ? 'w' : '-';
    output[2] = (mode & S_IXUSR) ? 'x' : '-';
   
    output[3] = (mode & S_IRGRP) ? 'r' : '-';
    output[4] = (mode & S_IWGRP) ? 'w' : '-'; 
    output[5] = (mode & S_IXGRP) ? 'x' : '-';
   
    output[6] = (mode & S_IROTH) ? 'r' : '-';
    output[7] = (mode & S_IWOTH) ? 'w' : '-';
    output[8] = (mode & S_IXOTH) ? 'x' : '-';
    
    output[9] = '\0';
}

char* role_to_string(Role role) {
    if(role == MANAGER) {
        return "manager";
    }
    else if(role == INSPECTOR) {
        return "inspector";
    }

    return "other";
}

void check_permissions(char path[], Role role, int access_type) {
    struct stat stat_buff; 
    mode_t read_bit = S_IROTH; 
    mode_t write_bit = S_IWOTH; 
    mode_t exec_bit  = S_IXOTH; 
    char permissions[10];

    if(stat(path, &stat_buff) == 1) {
        fprintf(stderr, "ERROR: couldn't open the file/directory!\n");
        exit(EXIT_FAILURE);
    }

    if(role == MANAGER) {
        read_bit = S_IRUSR;
        write_bit = S_IWUSR;
        exec_bit = S_IXUSR;
    }
    else if(role == INSPECTOR){
        read_bit = S_IRGRP; 
        write_bit = S_IWGRP; 
        exec_bit = S_IXGRP; 
    }

    if((access_type & ACCESS_READ) && !(stat_buff.st_mode & read_bit)) {
        fprintf(stderr, "ERROR: role %s cannot execute read command on that file / directory with path=%s!\n", role_to_string(role), path);
        exit(EXIT_FAILURE);
    }

    if((access_type & ACCESS_WRITE) && !(stat_buff.st_mode & write_bit)) {
        fprintf(stderr, "ERROR: role %s cannot execute write command on that file / directory with path=%s!\n", role_to_string(role), path);
        exit(EXIT_FAILURE);
    }

    if((access_type & ACCESS_EXEC) && !(stat_buff.st_mode & exec_bit)) {
        fprintf(stderr, "ERROR: role %s cannot execute exec command on that file / directory with path=%s!\n", role_to_string(role), path);
        exit(EXIT_FAILURE);
    }
}


void log_action(Command* cmd, char action[]) {
    int BUFFER_SIZE = 256;
    char path[BUFFER_SIZE];
    char line[1024];
    snprintf(path, sizeof(path), "%s/logged_district", cmd->district_id);

    struct stat stat_buff; 
    if(stat(path, &stat_buff) == -1) {
        fprintf(stderr, "ERROR: stat failed!\n");
        exit(EXIT_FAILURE);
    }

    if(!(stat_buff.st_mode & S_IWUSR)) {
        fprintf(stderr, "ERROR: logged_ditsr does not have manager write permission!\n");
        exit(EXIT_FAILURE);
    }

    check_permissions(path, cmd->role, ACCESS_WRITE);

    int fd; 
    char time_buffer[64];
    time_t now; 
    struct tm* time_info; 
    now = time(NULL);
    time_info = localtime(&now);
    if(time_info != NULL) {
        strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", time_info);
    }
    else {
        strncpy(time_buffer, "unknown", sizeof(time_buffer) - 1);
        time_buffer[sizeof(time_buffer) - 1] = '\0';
    }

    int line_size = snprintf(line, sizeof(line), "%s role=%s user=%s action=%s\n", time_buffer, role_to_string(cmd->role), cmd->user, action);
    if(line_size < 0 || (size_t)line_size >= sizeof(line)) {
        fprintf(stderr, "ERROR: log line is too long!\n");
        exit(EXIT_FAILURE);
    }

    fd = open(path, O_WRONLY | O_APPEND);
    if(fd == -1) {
        perror("ERROR: couldn't open log file");
        exit(EXIT_FAILURE);
    }

    if(write(fd, line, strlen(line)) != (ssize_t)strlen(line)) {
        perror("ERROR: couldn't write log");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
}
int main(int argc, char** argv) {
	if(argc < 7) {
		// write(stderr, "ERROR: you have not specified the role and the command!\n", 60); 
		fprintf(stderr, "ERROR: not enough arguments!");  
		exit(EXIT_FAILURE);
	}

    Command cmd_line = parse_arguments(argc, argv);

    void (*command)(Command*) = detect_command(&cmd_line);
    if(command == NULL) {
        fprintf(stderr, "ERROR: unknown command!\n");
        exit(EXIT_FAILURE);
    }

	command(&cmd_line); 
	return 0; 
}
