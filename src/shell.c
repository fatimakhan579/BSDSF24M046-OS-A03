#include "shell.h"

char* read_cmd(char* prompt, FILE* fp) {
    printf("%s", prompt);
    char* cmdline = (char*) malloc(sizeof(char) * MAX_LEN);
    int c, pos = 0;

    while ((c = getc(fp)) != EOF) {
        if (c == '\n') break;
        cmdline[pos++] = c;
    }

    if (c == EOF && pos == 0) {
        free(cmdline);
        return NULL; // Handle Ctrl+D
    }

    cmdline[pos] = '\0';
    return cmdline;
}

static void free_tokens(char** tokens, int count) {
    for (int i = 0; i < count; i++)
        free(tokens[i]);
    free(tokens);
}

// Splits a command line into tokens. Words are separated by spaces/tabs.
// The characters < > | are always tokens on their own, even without spaces.
// Text inside "double" or 'single' quotes stays together (quotes are removed).
char** tokenize(char* cmdline) {
    if (cmdline == NULL)
        return NULL;

    char** tokens = malloc(sizeof(char*) * (MAXARGS + 1));
    int ntok = 0;
    char* p = cmdline;

    while (*p != '\0') {
        while (*p == ' ' || *p == '\t') p++;      // skip whitespace
        if (*p == '\0') break;

        if (ntok >= MAXARGS) {
            fprintf(stderr, "error: too many arguments\n");
            free_tokens(tokens, ntok);
            return NULL;
        }

        if (*p == '<' || *p == '>' || *p == '|') { // operator token
            tokens[ntok++] = strndup(p, 1);
            p++;
            continue;
        }

        char* word = malloc(strlen(p) + 1);        // normal word
        int len = 0;
        char quote = 0;
        while (*p != '\0') {
            if (quote) {
                if (*p == quote) quote = 0;
                else word[len++] = *p;
            } else if (*p == '"' || *p == '\'') {
                quote = *p;
            } else if (*p == ' ' || *p == '\t' || *p == '<' || *p == '>' || *p == '|') {
                break;
            } else {
                word[len++] = *p;
            }
            p++;
        }
        word[len] = '\0';

        if (quote) {
            fprintf(stderr, "error: unterminated quote\n");
            free(word);
            free_tokens(tokens, ntok);
            return NULL;
        }
        tokens[ntok++] = word;
    }

    if (ntok == 0) {
        free(tokens);
        return NULL;
    }
    tokens[ntok] = NULL;
    return tokens;
}

static int is_operator(const char* t) {
    return strcmp(t, "<") == 0 || strcmp(t, ">") == 0 || strcmp(t, "|") == 0;
}

// Turns the token list into an array of commands (one per pipeline stage).
// Returns 0 on success, -1 on a syntax error.
int parse_pipeline(char** tokens, command_t cmds[], int* ncmds) {
    int n = 0;       // index of the command being filled
    int argc = 0;    // number of arguments collected for it
    memset(&cmds[0], 0, sizeof(command_t));

    for (int i = 0; tokens[i] != NULL; i++) {
        char* t = tokens[i];

        if (strcmp(t, "|") == 0) {
            if (argc == 0) {
                fprintf(stderr, "syntax error: missing command before '|'\n");
                return -1;
            }
            cmds[n].argv[argc] = NULL;
            if (++n == MAX_CMDS) {
                fprintf(stderr, "error: too many commands in pipeline\n");
                return -1;
            }
            memset(&cmds[n], 0, sizeof(command_t));
            argc = 0;
        } else if (strcmp(t, "<") == 0 || strcmp(t, ">") == 0) {
            char* file = tokens[i + 1];
            if (file == NULL || is_operator(file)) {
                fprintf(stderr, "syntax error: missing file name after '%s'\n", t);
                return -1;
            }
            if (t[0] == '<') cmds[n].infile = file;
            else             cmds[n].outfile = file;
            i++;                                    // skip the file name
        } else {
            cmds[n].argv[argc++] = t;
        }
    }

    if (argc == 0) {
        fprintf(stderr, "syntax error: missing command\n");
        return -1;
    }
    cmds[n].argv[argc] = NULL;
    *ncmds = n + 1;
    return 0;
}
