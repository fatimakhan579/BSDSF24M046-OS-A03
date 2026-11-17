#include "shell.h"
#include <fcntl.h>

// Runs a pipeline of ncmds commands (ncmds == 1 means a plain command).
// If background is set, the shell does not wait; the job is added to the job list.
// Returns the exit status of the last command (0 for a background launch).
int execute_pipeline(command_t cmds[], int ncmds, int background, const char* cmdtext) {
    pid_t pids[MAX_CMDS];
    int launched = 0;
    int prev_read = -1;      // read end of the previous pipe (-1 = none)
    int fd[2];

    fflush(NULL);            // don't let children inherit unflushed output

    for (int i = 0; i < ncmds; i++) {
        int has_next = (i < ncmds - 1);

        if (has_next && pipe(fd) == -1) {
            perror("pipe");
            break;
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            if (has_next) { close(fd[0]); close(fd[1]); }
            break;
        }

        if (pid == 0) {                         // ---- child ----
            if (prev_read != -1) {              // read from previous command
                dup2(prev_read, STDIN_FILENO);
                close(prev_read);
            }
            if (has_next) {                     // write to next command
                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
            }
            if (background && i == 0 && cmds[i].infile == NULL) {
                int dn = open("/dev/null", O_RDONLY);   // don't steal the shell's input
                if (dn != -1) { dup2(dn, STDIN_FILENO); close(dn); }
            }
            if (cmds[i].infile) {               // '<' overrides the pipe
                int in = open(cmds[i].infile, O_RDONLY);
                if (in == -1) { perror(cmds[i].infile); _exit(1); }
                dup2(in, STDIN_FILENO);
                close(in);
            }
            if (cmds[i].outfile) {              // '>' overrides the pipe
                int out = open(cmds[i].outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (out == -1) { perror(cmds[i].outfile); _exit(1); }
                dup2(out, STDOUT_FILENO);
                close(out);
            }
            if (handle_builtin(cmds[i].argv)) { // e.g. history | grep ls
                fflush(stdout);
                _exit(builtin_status);;
            }
            execvp(cmds[i].argv[0], cmds[i].argv);
            perror(cmds[i].argv[0]);            // only reached if exec failed
            _exit(127);
        }

        // ---- parent ----
        pids[launched++] = pid;
        if (prev_read != -1) close(prev_read);  // done with the old read end
        if (has_next) {
            close(fd[1]);                       // parent never writes to the pipe
            prev_read = fd[0];                  // keep read end for the next child
        } else {
            prev_read = -1;
        }
    }

    if (prev_read != -1) close(prev_read);      // only if we stopped early

    if (background) {                           // do not wait; remember the job
        if (launched == ncmds) {
            add_job(pids[launched - 1], cmdtext);
            return 0;
        }
        return 1;
    }

    int exit_code = 1;
    for (int i = 0; i < launched; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (i == ncmds - 1)
            exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
    }
    return exit_code;
}
