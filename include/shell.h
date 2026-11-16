#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_LEN 512
#define MAXARGS 64
#define MAX_CMDS 16
#define ARGLEN 30
#define PROMPT "FCIT> "
#define HISTORY_SIZE 20

// One command of a pipeline, e.g.  sort -r < in.txt > out.txt
typedef struct {
    char* argv[MAXARGS + 1];   // NULL-terminated, points into the token list
    char* infile;              // file after '<', or NULL
    char* outfile;             // file after '>', or NULL
} command_t;

// Function prototypes
char* read_cmd(char* prompt, FILE* fp);
char** tokenize(char* cmdline);
int parse_pipeline(char** tokens, command_t cmds[], int* ncmds);
int execute_pipeline(command_t cmds[], int ncmds);
int handle_builtin(char** arglist);

// History functions (history.c)
void add_to_history(const char* cmdline);
const char* get_history(int n);
void print_history(void);

#endif // SHELL_H
