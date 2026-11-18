#include "shell.h"
#include <ctype.h>

// Shell variables are stored in a singly linked list of name/value pairs.
typedef struct var {
    char* name;
    char* value;
    struct var* next;
} var_t;

static var_t* vars = NULL;
static int last_status = 0;      // exit status of the last command, for $?

static int is_name_start(char c) { return isalpha((unsigned char)c) || c == '_'; }
static int is_name_char(char c)  { return isalnum((unsigned char)c) || c == '_'; }

static void set_var(const char* name, const char* value) {
    var_t* last = NULL;
    for (var_t* v = vars; v != NULL; v = v->next) {
        if (strcmp(v->name, name) == 0) {        // already exists: replace the value
            char* copy = strdup(value);
            free(v->value);
            v->value = copy;
            return;
        }
        last = v;
    }
    var_t* v = malloc(sizeof(var_t));            // new variable: add at the end
    v->name = strdup(name);
    v->value = strdup(value);
    v->next = NULL;
    if (last == NULL) vars = v;
    else              last->next = v;
}

// Shell variable first, then the environment (so $HOME, $USER work), else "".
static const char* lookup(const char* name) {
    for (var_t* v = vars; v != NULL; v = v->next) {
        if (strcmp(v->name, name) == 0)
            return v->value;
    }
    const char* env = getenv(name);
    return env != NULL ? env : "";
}

// True for tokens of the form NAME=value (NAME = letters, digits, underscore).
int is_assignment(const char* token) {
    if (!is_name_start(token[0]))
        return 0;
    int i = 1;
    while (is_name_char(token[i]))
        i++;
    return token[i] == '=';
}

// Stores "NAME=value" (already expanded) in the variable list.
void assign_variable(const char* token) {
    const char* eq = strchr(token, '=');
    char* name = strndup(token, eq - token);
    set_var(name, eq + 1);
    free(name);
}

void set_last_status(int status) {
    last_status = status;
}

void print_vars(void) {
    for (var_t* v = vars; v != NULL; v = v->next)
        printf("%s=%s\n", v->name, v->value);
}

static void append(char** buf, size_t* len, size_t* cap, const char* s, size_t n) {
    while (*len + n + 1 > *cap) {
        *cap *= 2;
        *buf = realloc(*buf, *cap);
    }
    memcpy(*buf + *len, s, n);
    *len += n;
}

// Returns a newly allocated copy of 'word' in which $NAME, ${NAME} and $? are
// replaced by their values. The caller must free the result.
char* expand_variables(const char* word) {
    size_t cap = strlen(word) + 16, len = 0;
    char* out = malloc(cap);
    const char* p = word;

    while (*p != '\0') {
        const char* value = NULL;
        size_t consumed = 0;
        char name[256];
        char number[16];

        if (*p == '$') {
            if (p[1] == '?') {                               // $?
                snprintf(number, sizeof(number), "%d", last_status);
                value = number;
                consumed = 2;
            } else if (p[1] == '{') {                        // ${NAME}
                const char* close = strchr(p + 2, '}');
                if (close != NULL && close > p + 2 && (size_t)(close - (p + 2)) < sizeof(name)) {
                    size_t nlen = close - (p + 2);
                    memcpy(name, p + 2, nlen);
                    name[nlen] = '\0';
                    value = lookup(name);
                    consumed = nlen + 3;
                }
            } else if (is_name_start(p[1])) {                // $NAME
                size_t nlen = 1;
                while (is_name_char(p[1 + nlen]))
                    nlen++;
                if (nlen < sizeof(name)) {
                    memcpy(name, p + 1, nlen);
                    name[nlen] = '\0';
                    value = lookup(name);
                    consumed = nlen + 1;
                }
            }
        }

        if (consumed > 0) {
            append(&out, &len, &cap, value, strlen(value));
            p += consumed;
        } else {                                             // ordinary character
            append(&out, &len, &cap, p, 1);
            p++;
        }
    }
    out[len] = '\0';
    return out;
}
