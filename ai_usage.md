## Tools used
For this project, I have used ChatGPT 5.5 and Gemini 3.1 Pro

I used it as a learning and debugging tool, not as replacement to write code. 
The code was written by me, adapted to my style - but I have used AI to test the correcteness for my program - to avoid missing a corner case. 

## Where AI helped me
AI helped me in the following parts of the project:

- understanding how binary files work when storing fixed-sized records 
- understanding syscalls (such as 'open()', 'read()', 'write()')
- debugging permission logic, stat command
- understanding how to work with symlink
- debug and testing my  'add' command
- debug and test other commands
- implementing AI-assisted filter helper functions

## Prompts used 
The basis for my prompt was this one

"
I am implementing a C project called city_manager. The program stores city infrastructure reports in a binary file called reports.dat. Each record has this structure:

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

The filter command receives conditions in this format:

field:operator:value

Supported fields are:
- severity
- category
- inspector
- timestamp

Supported operators are:
==, !=, <, <=, >, >=

Please help me implement:

int parse_condition(const char *input, char *field, char *op, char *value);

This function should split the input condition into field, operator and value.

Also help me implement:

int match_condition(Report *r, const char *field, const char *op, const char *value);

This function should return 1 if the report satisfies the condition and 0 otherwise.

Please explain the functions line by line, because I need to understand and present them.
"

From there, I have tried to ask it nicely, one at a time,  to rewrite certain parts of the program to better fit my style and understanding (I have attached my code as a file as a reference).

## What AI generated 

For the filter command, AI generated helper logic for:

- `parse_condition()`, which separates a condition of the form `field:operator:value` into three strings: `field`, `op`, and `value`.
- `match_condition()`, which checks if one `Report` record satisfies one parsed condition.
- `compare_string()`, a small helper function used by `match_condition()` for text fields. It compares two strings using the supported operators for text comparison.

## Critical evaluation of AI output 
The AI output was useful, but it was not perfect. Some code had to be adapted because it did not match my style. 

I also had to make sure that I understood the code line by line, as this is vital for passing the presentation.   

