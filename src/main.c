#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>

int main() {
    char* cmdline;

    for (;;) {
        reap_jobs();                       // collect finished background jobs first

        cmdline = readline(PROMPT);
        if (cmdline == NULL)               // Ctrl+D
            break;

        reap_jobs();                       // also reap jobs that ended while we waited

        // History re-execution: replace "!n" with the stored command
        if (cmdline[0] == '!') {
            char* end;
            long n = strtol(cmdline + 1, &end, 10);
            const char* old = get_history((int)n);
            if (end == cmdline + 1 || *end != '\0' || old == NULL) {
                fprintf(stderr, "%s: event not found\n", cmdline);
                free(cmdline);
                continue;
            }
            free(cmdline);
            cmdline = strdup(old);
            printf("%s\n", cmdline);       // show what is being run
        }

        // Store non-blank commands in history
        if (cmdline[strspn(cmdline, " \t")] != '\0') {
            add_to_history(cmdline);       // our own list (history, !n)
            add_history(cmdline);          // readline's list (up/down arrows)
        }

        run_command(cmdline);
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
