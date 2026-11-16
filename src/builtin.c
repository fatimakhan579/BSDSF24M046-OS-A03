#include "shell.h"

int handle_builtin(char** arglist) {
    if (arglist == NULL || arglist[0] == NULL)
        return 0;

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
                return 1;
            }
        }
        if (chdir(dir) != 0)
            perror("cd");
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
        return 1;
    }

    if (strcmp(arglist[0], "history") == 0) {
        print_history();
        return 1;
    }

    if (strcmp(arglist[0], "jobs") == 0) {
        print_jobs();
        return 1;
    }

    return 0;
}
