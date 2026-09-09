// ch10_pool.c
// A worker pool with a limit.
// You give it a limit M and a list of tasks.
// It runs at most M children at the same time.
// When one finishes it starts the next one until all the tasks are done.
// run it like: ./ch10_pool 3 f1 f2 f3 f4 f5
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// this is the work each child does. it just prints and sleeps for a second.
static void do_task(const char *task) {
    printf("[worker %d] START task '%s'\n", getpid(), task);
    fflush(stdout);
    sleep(1); // pretend to do work
    printf("[worker %d] DONE  task '%s'\n", getpid(), task);
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s M task1 [task2 ...]\n", argv[0]);
        return 1;
    }
    int m = atoi(argv[1]);
    if (m <= 0) { fprintf(stderr, "M must be > 0\n"); return 1; }

    int total   = argc - 2; // the tasks start at argv[2]
    int next    = 2;        // which task to start next
    int running = 0;

    while (next < argc || running > 0) {
        // start new children until we hit the limit
        while (running < m && next < argc) {
            fflush(stdout); // clear the parent buffer before forking
            pid_t pid = fork();
            if (pid < 0) { perror("fork"); return 1; }
            if (pid == 0) {
                do_task(argv[next]);
                _exit(0);
            }
            next++;
            running++;
        }
        // wait for one child to finish before starting more
        int status;
        pid_t w = wait(&status);
        if (w > 0) {
            running--;
            printf("[parent] worker %d finished (%d still running)\n", w, running);
            fflush(stdout);
        }
    }
    printf("[parent] all %d tasks processed (limit was %d)\n", total, m);
    return 0;
}
