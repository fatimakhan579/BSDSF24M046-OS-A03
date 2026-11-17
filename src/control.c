#include "shell.h"
#include <readline/readline.h>

#define MAX_BLOCK_LINES 128

static int run_block(char** lines, int n);

// Returns 1 if the first word of 'line' is exactly 'kw'.
// If rest is not NULL, it is set to the text after the keyword.
static int is_keyword(const char* line, const char* kw, const char** rest) {
    while (*line == ' ' || *line == '\t')
        line++;
    size_t len = strlen(kw);
    if (strncmp(line, kw, len) != 0)
        return 0;
    if (line[len] != '\0' && line[len] != ' ' && line[len] != '\t')
        return 0;
    if (rest != NULL)
        *rest = line + len;
    return 1;
}

static int is_blank(const char* s) {
    return s[strspn(s, " \t")] == '\0';
}

// Runs an ordinary line, but rejects a stray then/else/fi.
static int run_simple(char* line) {
    static const char* stray[] = { "then", "else", "fi", NULL };
    for (int i = 0; stray[i] != NULL; i++) {
        if (is_keyword(line, stray[i], NULL)) {
            fprintf(stderr, "syntax error: unexpected '%s'\n", stray[i]);
            return 1;
        }
    }
    return run_line(line);
}

// Given that lines[start] begins an 'if', returns the index of its matching 'fi'
// (nested ifs are skipped), or -1 if there is none.
static int find_matching_fi(char** lines, int start, int n) {
    int depth = 0;
    for (int j = start; j < n; j++) {
        if (is_keyword(lines[j], "if", NULL)) {
            depth++;
        } else if (is_keyword(lines[j], "fi", NULL)) {
            depth--;
            if (depth == 0)
                return j;
        }
    }
    return -1;
}

// lines[0] is "if <condition>", lines[n-1] is "fi".
static int run_if(char** lines, int n) {
    const char* cond = NULL;
    is_keyword(lines[0], "if", &cond);
    while (*cond == ' ' || *cond == '\t')
        cond++;
    if (*cond == '\0') {
        fprintf(stderr, "syntax error: missing condition after 'if'\n");
        return 1;
    }

    // Find this if's own 'then' and 'else' (ignore those of nested ifs)
    int then_idx = -1, else_idx = -1, depth = 0;
    for (int j = 1; j < n - 1; j++) {
        const char* rest;
        if (is_keyword(lines[j], "if", NULL)) {
            depth++;
        } else if (is_keyword(lines[j], "fi", NULL)) {
            depth--;
        } else if (depth == 0 && is_keyword(lines[j], "then", &rest)) {
            if (then_idx >= 0 || else_idx >= 0) {
                fprintf(stderr, "syntax error: unexpected 'then'\n");
                return 1;
            }
            if (!is_blank(rest)) {
                fprintf(stderr, "syntax error: put commands after 'then' on the next line\n");
                return 1;
            }
            then_idx = j;
        } else if (depth == 0 && is_keyword(lines[j], "else", &rest)) {
            if (then_idx < 0 || else_idx >= 0) {
                fprintf(stderr, "syntax error: unexpected 'else'\n");
                return 1;
            }
            if (!is_blank(rest)) {
                fprintf(stderr, "syntax error: put commands after 'else' on the next line\n");
                return 1;
            }
            else_idx = j;
        }
    }
    if (then_idx < 0) {
        fprintf(stderr, "syntax error: missing 'then'\n");
        return 1;
    }

    const char* rest;
    is_keyword(lines[n - 1], "fi", &rest);
    if (!is_blank(rest)) {
        fprintf(stderr, "syntax error: unexpected text after 'fi'\n");
        return 1;
    }

    // The condition is the text on the 'if' line plus any lines before 'then'
    char* cond_lines[then_idx];
    cond_lines[0] = (char*)cond;
    for (int j = 1; j < then_idx; j++)
        cond_lines[j] = lines[j];

    int status = run_block(cond_lines, then_idx);   // exit status decides the branch

    int start, end;                                 // branch is lines[start .. end-1]
    if (status == 0) {
        start = then_idx + 1;
        end = (else_idx >= 0) ? else_idx : n - 1;
    } else if (else_idx >= 0) {
        start = else_idx + 1;
        end = n - 1;
    } else {
        return 0;                                   // false and no else: nothing to run
    }
    return run_block(lines + start, end - start);
}

// Runs a list of lines in order, handling (nested) if-blocks.
static int run_block(char** lines, int n) {
    int status = 0;
    int i = 0;

    while (i < n) {
        if (is_keyword(lines[i], "if", NULL)) {
            int end = find_matching_fi(lines, i, n);
            if (end < 0) {
                fprintf(stderr, "syntax error: missing 'fi'\n");
                return 1;
            }
            status = run_if(lines + i, end - i + 1);
            i = end + 1;
        } else {
            status = run_simple(lines[i]);
            i++;
        }
    }
    return status;
}

static int depth_change(const char* line) {
    if (is_keyword(line, "if", NULL))
        return 1;
    if (is_keyword(line, "fi", NULL))
        return -1;
    return 0;
}

// Reads continuation lines (prompt "> ") until the matching 'fi', then runs the block.
static int read_and_run_if(const char* first_line) {
    char* lines[MAX_BLOCK_LINES];
    int n = 0;
    int status = 1;

    lines[n++] = strdup(first_line);
    int depth = depth_change(first_line);

    while (depth > 0) {
        char* line = readline("> ");
        if (line == NULL) {
            fprintf(stderr, "syntax error: unexpected end of file (missing 'fi')\n");
            goto done;
        }
        if (n == MAX_BLOCK_LINES) {
            fprintf(stderr, "error: if-block is too long\n");
            free(line);
            goto done;
        }
        lines[n++] = line;
        depth += depth_change(line);
    }
    status = run_block(lines, n);

done:
    for (int i = 0; i < n; i++)
        free(lines[i]);
    return status;
}

// Entry point used by main(): an 'if' starts a multi-line block, anything else is one line.
int run_command(char* cmdline) {
    if (is_keyword(cmdline, "if", NULL))
        return read_and_run_if(cmdline);
    return run_simple(cmdline);
}
