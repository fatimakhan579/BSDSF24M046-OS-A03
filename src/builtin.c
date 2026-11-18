#include "shell.h"

int builtin_status = 0;

int handle_builtin(char** arglist) {
    if (arglist == NULL || arglist[0] == NULL)
        return 0;

    builtin_status = 0;

    if (strcmp(arglist[0], "exit") == 0) {
        printf("Shell exited.\n");
        exit(0);
    }

    if (strcmp(arglist[0], "cd") == 0) {
        const char* dir = arglist[1];
        if (dir == NULL) {
            dir = getenv("HOME");
            if (dir == NULL) {
                fprintf(stderr, "cd: HOME not set\n");
                builtin_status = 1;
                return 1;
            }
        }
        if (chdir(dir) != 0) {
            perror("cd");
            builtin_status = 1;
        }
        return 1;
    }

    if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  exit        Exit the shell\n");
        printf("  cd <dir>    Change the current directory\n");
        printf("  help        Show this help message\n");
        printf("  history     Show the last commands entered\n");
        printf("  !n          Re-run command number n from history\n");
        printf("  jobs        List background jobs\n");

                printf("  set         Show all shell variables\n");
        printf("\nShell syntax:\n");
        printf("  cmd1 ; cmd2        Run commands one after another\n");
        printf("  cmd &              Run a command in the background\n");
        printf("  cmd1 | cmd2        Pipe the output of cmd1 into cmd2\n");
        printf("  cmd > file         Redirect output to a file\n");
        printf("  cmd < file         Read input from a file\n");
        printf("  NAME=value         Set a variable; use it as $NAME or ${NAME}\n");
        printf("  if cmd / then / else / fi   Conditional (each keyword on its own line)\n");

return 1;
    }

    if (strcmp(arglist[0], "history") == 0) {
        print_history();
        return 1;
    }
    if (strcmp(arglist[0], "set") == 0) {
        print_vars();
        return 1;
    }

    if (strcmp(arglist[0], "jobs") == 0) {
        print_jobs();
        return 1;
    }

    return 0;
}
