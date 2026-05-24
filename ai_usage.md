# Phase 1

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


# Phase 2

## Tools used
For this project, I have used ChatGPT 5.5 and Gemini 3.1 Pro

I used it as a learning and debugging tool, not as replacement to write code. 
The code was written by me, adapted to my style - but I have used AI to test the correctness for my program - to avoid missing a corner case. 

## Where AI helped me
Primarily it was used as a reviewer for my code, whose feedback in some cases, I would listen to (for example, in monitor_reports.c it suggested me to use pause() instead of continue inside of while(running)) and it other cases I would ignore it (it suggested me that if unlink(symlink_name) fails, then I don't need to panic and keep the execution of the program going, which I don't think should be the right behavior).

Also it generated me the commands on which I would test the correctness of my program. 

## Prompts used 
Absolutely straightforward - I would attach my code and ask it "what do you think?" and we would continue to talk until I fix all the mistakes and the quality was good enough. 

Also I would write to it "generate me some commands on which I would test my code".

## What AI generated 
It would copy snippets of code from my program and would criticize it and suggest me ways to improve - I would take only the decision, to make it how it told me or in the other way. Also it would point out to logical errors in my program and corner cases that I didn't check. 

## Critical evaluation of AI output 
AI didn't help me a lot during that phase, it was the internet. 
The article that helped me the most was this one [https://www.codequoi.com/en/sending-and-intercepting-a-signal-in-c/].

Still it was very useful, because it gave me additional believe that my program was correct. 

# Phase ALL 

## How?
I have used AI as a code reviewer and a "programming mentor", which helped me spot mistakes in my understanding and generate tests for what I had written. Sometimes AI suggested code solutions for my bugs, and when I was completely stuck, I would ask it to rewrite the code in pseudocode / plain text, so I could write it in my own style. 

## Why?
The way I used AI was to maximize productivity: I delegated routine tasks, such as generating test commands for my program, while trying to code everything in my program by myself. Sometimes, when I hit a wall, I asked about the steps to implement something, without blindly copying the code, which would have compromised the learning experience.