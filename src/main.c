#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>

int main() {
    char* cmdline;
    char** arglist;

    while ((cmdline = readline(PROMPT)) != NULL) {

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
            printf("%s\n", cmdline);   // show what is being run
        }

        // Store non-blank commands in history
        if (cmdline[strspn(cmdline, " \t")] != '\0')
        {
            add_to_history(cmdline);   // our own list (history, !n)
            add_history(cmdline);      // readline's list (up/down arrows)
        }

        if ((arglist = tokenize(cmdline)) != NULL) {
            command_t cmds[MAX_CMDS];
            int ncmds;

            if (parse_pipeline(arglist, cmds, &ncmds) == 0) {
                int handled = 0;
                // A plain built-in (no pipe, no redirection) runs inside the shell
                if (ncmds == 1 && cmds[0].infile == NULL && cmds[0].outfile == NULL)
                    handled = handle_builtin(cmds[0].argv);
                if (!handled)
                    execute_pipeline(cmds, ncmds);
            }

            // Free the memory allocated by tokenize()
            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }
            free(arglist);
        }
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
