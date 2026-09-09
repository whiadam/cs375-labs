// ch2_multi_child.c
// The parent makes N children. N comes from the command line.
// Each child prints its number and exits with code number+1.
// The parent waits for all of them and prints each exit status.
// run it like: ./ch2_multi_child 5
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s N\n", argv[0]);
        return 1;
    }
    int n = atoi(argv[1]);
    if (n <= 0) { fprintf(stderr, "N must be > 0\n"); return 1; }

    for (int i = 0; i < n; ++i) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); return 1; }
        if (pid == 0) {
            printf("[child index=%d] PID=%d, exiting with %d\n", i, getpid(), i + 1);
            fflush(stdout);
            _exit(i + 1);
        }
        // the parent keeps going to make the next child
    }

    // wait for all the children. waitpid with -1 grabs whichever one finishes.
    for (int i = 0; i < n; ++i) {
        int status;
        pid_t w = waitpid(-1, &status, 0);
        if (w == -1) { perror("waitpid"); break; }
        if (WIFEXITED(status))
            printf("[parent] child %d finished, exit status %d\n", w, WEXITSTATUS(status));
    }
    return 0;
}
