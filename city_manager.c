#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

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
int parse_condition(const char* input, char field[], char op[], char value[]);
int match_condition(Report* r, const char field[], const char op[], const char value[]);
int compare_int(long long left, const char op[], long long right);
int compare_string(const char left[], const char op[], const char right[]);
void mode_to_string(mode_t mode, char output[]);
void scan_active_report_links(void);
void ensure_active_reports_symlink(const char district_id[]);

// =============== COMMANDS ===============
void add(Command* cmd) {
    size_t BUFFER_SIZE = 256; 

    char path[BUFFER_SIZE];
    int bytes_written;

    struct stat stat_buff;  

    bytes_written = snprintf(path, sizeof(path), "%s", cmd->district_id);

    if(bytes_written < 0 || bytes_written >= (int)BUFFER_SIZE) {
        fprintf(stderr, "ERROR: path is too long!\n");
        exit(EXIT_FAILURE);
    }

    // create district directory if it does not exist
    if(stat(path, &stat_buff) == -1) {
        if(mkdir(path, 0750) != 0) {
            perror("ERROR: couldn't create the directory");
            exit(EXIT_FAILURE);
        }

        chmod(path, 0750);
    }

    // inspector does NOT need write permission on the directory
    // inspector only needs execute permission to enter it
    check_permissions(cmd->district_id, cmd->role, ACCESS_EXEC);

    // create reports.dat path
    bytes_written = snprintf(path, sizeof(path), "%s/reports.dat", cmd->district_id);

    if(bytes_written < 0 || bytes_written >= (int)BUFFER_SIZE) {
        fprintf(stderr, "ERROR: path is too long!\n");
        exit(EXIT_FAILURE);
    }

    // create reports.dat if it does not exist yet
    int fd = open(path, O_WRONLY | O_APPEND | O_CREAT, 0664);

    if(fd == -1) {
        perror("ERROR: couldn't open reports.dat");
        exit(EXIT_FAILURE);
    }

    close(fd);
    chmod(path, 0664);

    ensure_active_reports_symlink(cmd->district_id);

    // now it exists, so now we can check permission
    check_permissions(path, cmd->role, ACCESS_WRITE);

    // create district.cfg if missing
    char cfg_path[BUFFER_SIZE];

    bytes_written = snprintf(cfg_path, sizeof(cfg_path), "%s/district.cfg", cmd->district_id);

    if(bytes_written < 0 || bytes_written >= (int)BUFFER_SIZE) {
        fprintf(stderr, "ERROR: path is too long!\n");
        exit(EXIT_FAILURE);
    }

    if(stat(cfg_path, &stat_buff) == -1) {
        fd = open(cfg_path, O_WRONLY | O_CREAT | O_TRUNC, 0640);

        if(fd == -1) {
            perror("ERROR: couldn't create district.cfg");
            exit(EXIT_FAILURE);
        }

        char threshold[] = "3\n";

        if(write(fd, threshold, strlen(threshold)) != (ssize_t)strlen(threshold)) {
            perror("ERROR: couldn't write district.cfg");
            close(fd);
            exit(EXIT_FAILURE);
        }

        close(fd);
        chmod(cfg_path, 0640);
    }

    // create logged_district if missing
    char log_path[BUFFER_SIZE];

    bytes_written = snprintf(log_path, sizeof(log_path), "%s/logged_district", cmd->district_id);

    if(bytes_written < 0 || bytes_written >= (int)BUFFER_SIZE) {
        fprintf(stderr, "ERROR: path is too long!\n");
        exit(EXIT_FAILURE);
    }

    if(stat(log_path, &stat_buff) == -1) {
        fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);

        if(fd == -1) {
            perror("ERROR: couldn't create logged_district");
            exit(EXIT_FAILURE);
        }

        close(fd);
        chmod(log_path, 0644);
    }

    Report r;

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
        perror("ERROR: couldn't open reports.dat");
        exit(EXIT_FAILURE);
    }

    if(write(fd, &r, sizeof(Report)) != sizeof(Report)) {
        perror("ERROR: couldn't write report");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);

    if(cmd->role == MANAGER) {
        log_action(cmd, "add");
    }
    else {
        fprintf(stderr, "WARNING: inspector add worked, but was not logged because logged_district is manager-write only.\n");
    }
}

void list(Command* cmd) {
    size_t BUFFER_SIZE = 256;
    char filename[BUFFER_SIZE];
    size_t bytes_written;
    int fd;
    Report r;
    ssize_t bytes_read;
    struct stat stat_buff;
    char permissions[10];
    char time_buffer[64];
    struct tm* time_info;
    int found_any = 0;

    bytes_written = snprintf(filename, sizeof(filename), "%s/%s", cmd->district_id, "reports.dat");
    if(bytes_written >= BUFFER_SIZE) {
        fprintf(stderr, "ERROR: name is too long!\n");
        exit(EXIT_FAILURE);
    }

    check_permissions(cmd->district_id, cmd->role, ACCESS_EXEC);
    check_permissions(filename, cmd->role, ACCESS_READ);

    if(stat(filename, &stat_buff) == -1) {
        perror("ERROR: stat failed for reports.dat");
        exit(EXIT_FAILURE);
    }

    mode_to_string(stat_buff.st_mode, permissions);

    time_info = localtime(&stat_buff.st_mtime);
    if(time_info != NULL) {
        strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", time_info);
    }
    else {
        strncpy(time_buffer, "unknown", sizeof(time_buffer) - 1);
        time_buffer[sizeof(time_buffer) - 1] = '\0';
    }

    printf("reports.dat permissions = %s\n", permissions);
    printf("reports.dat size = %lld bytes\n", (long long)stat_buff.st_size);
    printf("reports.dat last modified = %s\n", time_buffer);
    printf("-----------------------------\n");

    if((fd = open(filename, O_RDONLY)) == -1) {
        fprintf(stderr, "ERROR: you couldn't open the file!\n");
        exit(EXIT_FAILURE);
    }

    while((bytes_read = read(fd, &r, sizeof(Report))) == sizeof(Report)) {
        found_any = 1;

        printf("id = %d \n", r.id);
        printf("inspector = %s \n", r.inspector);
        printf("lat = %f \n", r.latitude);
        printf("long = %f \n", r.longitude);
        printf("cat = %s \n", r.category);
        printf("severity = %d \n", r.severity);
        printf("timestamp = %lld \n", (long long)r.timestamp);
        printf("description = %s \n", r.description);
        printf("-----------------------------\n");
    }

    if(bytes_read == -1) {
        perror("ERROR: couldn't read reports.dat");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if(bytes_read != 0) {
        fprintf(stderr, "ERROR: reports.dat contains an incomplete report!\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if(!found_any) {
        printf("No reports found.\n");
    }

    close(fd);

    if(cmd->role == MANAGER) {
        log_action(cmd, "list");
    }
}

void remove_report(Command* cmd) {
    size_t BUFFER_SIZE = 256;
    char path[BUFFER_SIZE];
    size_t bytes_written;
    struct stat stat_buff;
    int fd;
    Report r;
    int found = 0;
    int found_position = -1;
    long total_reports;
    long i;

    if(cmd->role != MANAGER) {
        fprintf(stderr, "ERROR: only manager can remove reports!\n");
        exit(EXIT_FAILURE);
    }

    bytes_written = snprintf(path, sizeof(path), "%s/reports.dat", cmd->district_id);
    if(bytes_written >= BUFFER_SIZE) {
        fprintf(stderr, "ERROR: name is too long!\n");
        exit(EXIT_FAILURE);
    }

    check_permissions(cmd->district_id, cmd->role, ACCESS_EXEC);
    check_permissions(path, cmd->role, ACCESS_READ | ACCESS_WRITE);

    if(stat(path, &stat_buff) == -1) {
        perror("ERROR: stat failed for reports.dat");
        exit(EXIT_FAILURE);
    }

    total_reports = stat_buff.st_size / sizeof(Report);

    fd = open(path, O_RDWR);
    if(fd == -1) {
        perror("ERROR: couldn't open reports.dat");
        exit(EXIT_FAILURE);
    }

    for(i = 0; i < total_reports; i++) {
        if(read(fd, &r, sizeof(Report)) != sizeof(Report)) {
            perror("ERROR: couldn't read report");
            close(fd);
            exit(EXIT_FAILURE);
        }

        if(r.id == cmd->report_id) {
            found = 1;
            found_position = i;
            break;
        }
    }

    if(!found) {
        fprintf(stderr, "ERROR: report with id %d was not found!\n", cmd->report_id);
        close(fd);
        exit(EXIT_FAILURE);
    }

    for(i = found_position + 1; i < total_reports; i++) {
        if(lseek(fd, i * sizeof(Report), SEEK_SET) == -1) {
            perror("ERROR: lseek failed");
            close(fd);
            exit(EXIT_FAILURE);
        }

        if(read(fd, &r, sizeof(Report)) != sizeof(Report)) {
            perror("ERROR: couldn't read report while shifting");
            close(fd);
            exit(EXIT_FAILURE);
        }

        if(lseek(fd, (i - 1) * sizeof(Report), SEEK_SET) == -1) {
            perror("ERROR: lseek failed");
            close(fd);
            exit(EXIT_FAILURE);
        }

        if(write(fd, &r, sizeof(Report)) != sizeof(Report)) {
            perror("ERROR: couldn't write shifted report");
            close(fd);
            exit(EXIT_FAILURE);
        }
    }

    if(ftruncate(fd, (total_reports - 1) * sizeof(Report)) == -1) {
        perror("ERROR: ftruncate failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);

    printf("Report %d removed successfully.\n", cmd->report_id);
    log_action(cmd, "remove_report");
}
void update_threshold(Command* cmd) {
    size_t BUFFER_SIZE = 256;
    char path[BUFFER_SIZE];
    char buffer[64];
    size_t bytes_written;
    int line_size;
    int fd;
    struct stat stat_buff;

    if(cmd->role != MANAGER) {
        fprintf(stderr, "ERROR: only manager can update threshold!\n");
        exit(EXIT_FAILURE);
    }

    if(cmd->severity < 1 || cmd->severity > 3) {
        fprintf(stderr, "ERROR: threshold must be 1, 2, or 3!\n");
        exit(EXIT_FAILURE);
    }

    bytes_written = snprintf(path, sizeof(path), "%s/district.cfg", cmd->district_id);
    if(bytes_written >= BUFFER_SIZE) {
        fprintf(stderr, "ERROR: name is too long!\n");
        exit(EXIT_FAILURE);
    }

    if(stat(path, &stat_buff) == -1) {
        perror("ERROR: stat failed for district.cfg");
        exit(EXIT_FAILURE);
    }

    if((stat_buff.st_mode & 0777) != 0640) {
        fprintf(stderr, "ERROR: district.cfg permissions must be 640!\n");
        exit(EXIT_FAILURE);
    }

    check_permissions(path, cmd->role, ACCESS_WRITE);

    fd = open(path, O_WRONLY | O_TRUNC);
    if(fd == -1) {
        perror("ERROR: couldn't open district.cfg");
        exit(EXIT_FAILURE);
    }

    line_size = snprintf(buffer, sizeof(buffer), "%d\n", cmd->severity);
    if(line_size < 0 || (size_t)line_size >= sizeof(buffer)) {
        fprintf(stderr, "ERROR: threshold line too long!\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if(write(fd, buffer, strlen(buffer)) != (ssize_t)strlen(buffer)) {
        perror("ERROR: couldn't write threshold");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);

    printf("Threshold updated to %d.\n", cmd->severity);
    log_action(cmd, "update_threshold");
}

void filter(Command* cmd) {
    size_t BUFFER_SIZE = 256;
    char path[BUFFER_SIZE];
    size_t bytes_written;
    int fd;
    Report r;
    size_t i;
    int good_report;
    char field[64];
    char op[8];
    char value[256];

    bytes_written = snprintf(path, sizeof(path), "%s/reports.dat", cmd->district_id);
    if(bytes_written >= BUFFER_SIZE) {
        fprintf(stderr, "ERROR: name is too long!\n");
        exit(EXIT_FAILURE);
    }

    check_permissions(cmd->district_id, cmd->role, ACCESS_EXEC);
    check_permissions(path, cmd->role, ACCESS_READ);

    fd = open(path, O_RDONLY);
    if(fd == -1) {
        perror("ERROR: couldn't open reports.dat");
        exit(EXIT_FAILURE);
    }

    while(read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        good_report = 1;

        for(i = 0; i < cmd->conditions_count; i++) {
            if(!parse_condition(cmd->conditions[i], field, op, value)) {
                fprintf(stderr, "ERROR: invalid condition: %s\n", cmd->conditions[i]);
                close(fd);
                exit(EXIT_FAILURE);
            }

            if(!match_condition(&r, field, op, value)) {
                good_report = 0;
                break;
            }
        }

        if(good_report) {
            printf("id = %d \n", r.id);
            printf("inspector = %s \n", r.inspector);
            printf("lat = %f \n", r.latitude);
            printf("long = %f \n", r.longitude);
            printf("cat = %s \n", r.category);
            printf("severity = %d \n", r.severity);
            printf("timestamp = %lld \n", (long long)r.timestamp);
            printf("description = %s \n", r.description);
            printf("-----------------------------\n");
        }
    }

    close(fd);

    if(cmd->role == MANAGER) {
        log_action(cmd, "filter");
    }
}

void view(Command* cmd) {
    size_t BUFFER_SIZE = 256;
    char filename[BUFFER_SIZE];
    size_t bytes_written;
    int fd;
    Report r;
    ssize_t bytes_read;
    int found = 0;

    bytes_written = snprintf(filename, sizeof(filename), "%s/%s", cmd->district_id, "reports.dat");
    if(bytes_written >= BUFFER_SIZE) {
        fprintf(stderr, "ERROR: name is too long!\n");
        exit(EXIT_FAILURE);
    }

    check_permissions(cmd->district_id, cmd->role, ACCESS_EXEC);
    check_permissions(filename, cmd->role, ACCESS_READ);

    if((fd = open(filename, O_RDONLY)) == -1) {
        fprintf(stderr, "ERROR: you couldn't open the file!\n");
        exit(EXIT_FAILURE);
    }

    while((bytes_read = read(fd, &r, sizeof(Report))) == sizeof(Report)) {
        if(cmd->report_id == r.id) {
            found = 1;

            printf("id = %d \n", r.id);
            printf("inspector = %s \n", r.inspector);
            printf("lat = %f \n", r.latitude);
            printf("long = %f \n", r.longitude);
            printf("cat = %s \n", r.category);
            printf("severity = %d \n", r.severity);
            printf("timestamp = %lld \n", (long long)r.timestamp);
            printf("description = %s \n", r.description);
            break;
        }
    }

    if(bytes_read == -1) {
        perror("ERROR: couldn't read reports.dat");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if(bytes_read != 0 && !found) {
        fprintf(stderr, "ERROR: reports.dat contains an incomplete report!\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);

    if(!found) {
        fprintf(stderr, "ERROR: report with id %d was not found!\n", cmd->report_id);
        exit(EXIT_FAILURE);
    }

    if(cmd->role == MANAGER) {
        log_action(cmd, "view");
    }
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
    ssize_t bytes_read; 
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

    if(stat(path, &stat_buff) == -1) {
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

int parse_condition(const char* input, char field[], char op[], char value[]) {
    const char* first_colon;
    const char* second_colon;
    size_t field_size;
    size_t op_size;

    first_colon = strchr(input, ':');
    if(first_colon == NULL) {
        return 0;
    }

    second_colon = strchr(first_colon + 1, ':');
    if(second_colon == NULL) {
        return 0;
    }

    field_size = first_colon - input;
    op_size = second_colon - first_colon - 1;

    if(field_size == 0 || field_size >= 64) {
        return 0;
    }

    if(op_size == 0 || op_size >= 8) {
        return 0;
    }

    strncpy(field, input, field_size);
    field[field_size] = '\0';

    strncpy(op, first_colon + 1, op_size);
    op[op_size] = '\0';

    strncpy(value, second_colon + 1, 255);
    value[255] = '\0';

    if(strcmp(op, "==") != 0 && strcmp(op, "!=") != 0 &&
       strcmp(op, "<") != 0  && strcmp(op, "<=") != 0 &&
       strcmp(op, ">") != 0  && strcmp(op, ">=") != 0) {
        return 0;
    }

    return 1;
}

int compare_int(long long left, const char op[], long long right) {
    if(strcmp(op, "==") == 0) return left == right;
    if(strcmp(op, "!=") == 0) return left != right;
    if(strcmp(op, "<") == 0)  return left < right;
    if(strcmp(op, "<=") == 0) return left <= right;
    if(strcmp(op, ">") == 0)  return left > right;
    if(strcmp(op, ">=") == 0) return left >= right;

    return 0;
}

int compare_string(const char left[], const char op[], const char right[]) {
    int result = strcmp(left, right);

    if(strcmp(op, "==") == 0) return result == 0;
    if(strcmp(op, "!=") == 0) return result != 0;
    if(strcmp(op, "<") == 0)  return result < 0;
    if(strcmp(op, "<=") == 0) return result <= 0;
    if(strcmp(op, ">") == 0)  return result > 0;
    if(strcmp(op, ">=") == 0) return result >= 0;

    return 0;
}

int match_condition(Report* r, const char field[], const char op[], const char value[]) {
    char* endptr;
    long long number;

    if(strcmp(field, "severity") == 0) {
        number = strtoll(value, &endptr, 10);
        if(endptr == value || *endptr != '\0') {
            return 0;
        }

        return compare_int(r->severity, op, number);
    }

    if(strcmp(field, "timestamp") == 0) {
        number = strtoll(value, &endptr, 10);
        if(endptr == value || *endptr != '\0') {
            return 0;
        }

        return compare_int((long long)r->timestamp, op, number);
    }

    if(strcmp(field, "category") == 0) {
        return compare_string(r->category, op, value);
    }

    if(strcmp(field, "inspector") == 0) {
        return compare_string(r->inspector, op, value);
    }

    return 0;
}

void scan_active_report_links(void) {
    DIR* dir;
    struct dirent* entry;
    struct stat stat_buff;
    char target[PATH_MAX];
    ssize_t bytes_read;

    dir = opendir(".");
    if(dir == NULL) {
        perror("WARNING: couldn't scan current directory");
        return;
    }

    while((entry = readdir(dir)) != NULL) {
        if(strncmp(entry->d_name, "active_reports-", strlen("active_reports-")) != 0) {
            continue;
        }

        if(lstat(entry->d_name, &stat_buff) == -1) {
            perror("WARNING: lstat failed while scanning active_reports links");
            continue;
        }

        if(!S_ISLNK(stat_buff.st_mode)) {
            continue;
        }

        bytes_read = readlink(entry->d_name, target, sizeof(target) - 1);
        if(bytes_read == -1) {
            perror("WARNING: couldn't read active_reports symlink while scanning");
            continue;
        }

        target[bytes_read] = '\0';

        if(stat(target, &stat_buff) == -1 && errno == ENOENT) {
            fprintf(stderr, "WARNING: dangling symlink detected: %s -> %s\n", entry->d_name, target);
        }
    }

    closedir(dir);
}

void ensure_active_reports_symlink(const char district_id[]) {
    char target[PATH_MAX];
    char link_name[PATH_MAX];
    char current_target[PATH_MAX];
    struct stat target_stat;
    struct stat link_stat;
    int bytes_written;
    ssize_t bytes_read;

    bytes_written = snprintf(target, sizeof(target), "%s/reports.dat", district_id);
    if(bytes_written < 0 || bytes_written >= (int)sizeof(target)) {
        fprintf(stderr, "ERROR: report path is too long!\n");
        exit(EXIT_FAILURE);
    }

    bytes_written = snprintf(link_name, sizeof(link_name), "active_reports-%s", district_id);
    if(bytes_written < 0 || bytes_written >= (int)sizeof(link_name)) {
        fprintf(stderr, "ERROR: symlink name is too long!\n");
        exit(EXIT_FAILURE);
    }

    if(stat(target, &target_stat) == -1) {
        fprintf(stderr, "WARNING: cannot create %s because target %s does not exist.\n", link_name, target);
        return;
    }

    if(lstat(link_name, &link_stat) == -1) {
        if(errno != ENOENT) {
            perror("WARNING: lstat failed for active_reports symlink");
            return;
        }

        if(symlink(target, link_name) == -1) {
            perror("WARNING: couldn't create active_reports symlink");
            return;
        }

        return;
    }

    if(!S_ISLNK(link_stat.st_mode)) {
        fprintf(stderr, "WARNING: %s already exists, but it is not a symlink. I will not replace it.\n", link_name);
        return;
    }

    bytes_read = readlink(link_name, current_target, sizeof(current_target) - 1);
    if(bytes_read == -1) {
        perror("WARNING: couldn't read active_reports symlink");
        return;
    }

    current_target[bytes_read] = '\0';

    if(strcmp(current_target, target) != 0) {
        if(unlink(link_name) == -1) {
            perror("WARNING: couldn't remove old active_reports symlink");
            return;
        }

        if(symlink(target, link_name) == -1) {
            perror("WARNING: couldn't recreate active_reports symlink");
            return;
        }
    }
}

int main(int argc, char** argv) {
    scan_active_report_links();
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

