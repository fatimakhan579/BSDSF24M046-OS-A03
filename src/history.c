#include "shell.h"

static char* history[HISTORY_SIZE];
static int history_count = 0;

void add_to_history(const char* cmdline) {
    if (history_count == HISTORY_SIZE) {
        // Full: drop the oldest entry and shift the rest down by one
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++)
            history[i - 1] = history[i];
        history_count--;
    }
    history[history_count++] = strdup(cmdline);
}

// n is 1-based, as shown by the history command. Returns NULL if out of range.
const char* get_history(int n) {
    if (n < 1 || n > history_count)
        return NULL;
    return history[n - 1];
}

void print_history(void) {
    for (int i = 0; i < history_count; i++)
        printf("%d  %s\n", i + 1, history[i]);
}
