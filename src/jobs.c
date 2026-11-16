#include "shell.h"

#define MAX_JOBS 32

typedef struct {
    int id;
    pid_t pid;
    char cmd[MAX_LEN];
} job_t;

static job_t jobs[MAX_JOBS];
static int njobs = 0;
static int next_id = 1;

void add_job(pid_t pid, const char* cmd) {
    if (njobs == MAX_JOBS) {
        fprintf(stderr, "warning: job table full, job not tracked\n");
        return;
    }
    if (njobs == 0)
        next_id = 1;
    job_t* j = &jobs[njobs++];
    j->id = next_id++;
    j->pid = pid;
    strncpy(j->cmd, cmd, MAX_LEN - 1);
    j->cmd[MAX_LEN - 1] = '\0';
    printf("[%d] %d\n", j->id, (int)pid);
}

// Collect finished children without blocking, so no zombies are left behind.
void reap_jobs(void) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < njobs; i++) {
            if (jobs[i].pid == pid) {
                printf("[%d]  Done    %s\n", jobs[i].id, jobs[i].cmd);
                for (int k = i + 1; k < njobs; k++)     // close the gap
                    jobs[k - 1] = jobs[k];
                njobs--;
                break;
            }
        }
    }
}

void print_jobs(void) {
    reap_jobs();
    if (njobs == 0) {
        printf("No background jobs.\n");
        return;
    }
    for (int i = 0; i < njobs; i++)
        printf("[%d]  %d  Running  %s\n", jobs[i].id, (int)jobs[i].pid, jobs[i].cmd);
}
