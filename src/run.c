#include "shell.h"

// Joins the words of a command back into one string (for the jobs list).
static void join_tokens(char** seg, char* buf, size_t size) {
    buf[0] = '\0';
    for (int i = 0; seg[i] != NULL; i++) {
        if (i > 0)
            strncat(buf, " ", size - strlen(buf) - 1);
        strncat(buf, seg[i], size - strlen(buf) - 1);
    }
}

// Runs one pipeline (no ';' or '&' inside it).
static int run_statement(char** seg, int background) {
    command_t cmds[MAX_CMDS];
    int ncmds;

    if (parse_pipeline(seg, cmds, &ncmds) != 0)
        return 1;

    // A plain built-in (no pipe, no redirection, not background) runs in the shell
    if (!background && ncmds == 1 && cmds[0].infile == NULL && cmds[0].outfile == NULL
        && handle_builtin(cmds[0].argv))
        return 0;

    char text[MAX_LEN];
    join_tokens(seg, text, sizeof(text));
    return execute_pipeline(cmds, ncmds, background, text);
}

// Tokenizes a line, splits it at ';' and '&', and runs each part in order.
// ';' ends a command that is waited for; '&' ends one that runs in the background.
// Returns the exit status of the last command that ran.
int run_line(char* cmdline) {
    char** tokens = tokenize(cmdline);
    if (tokens == NULL)
        return 0;

    char* seg[MAXARGS + 1];
    int status = 0;
    int i = 0;

    while (tokens[i] != NULL) {
        int n = 0;
        while (tokens[i] != NULL && strcmp(tokens[i], ";") != 0 && strcmp(tokens[i], "&") != 0)
            seg[n++] = tokens[i++];
        seg[n] = NULL;

        int background = 0;
        if (tokens[i] != NULL) {            // we stopped at ';' or '&'
            background = (strcmp(tokens[i], "&") == 0);
            i++;
        }

        if (n > 0)
            status = run_statement(seg, background);
    }

    for (int j = 0; tokens[j] != NULL; j++)
        free(tokens[j]);
    free(tokens);
    return status;
}
