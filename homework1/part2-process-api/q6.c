// q6.c - same as q5 but with waitpid(), waiting for children in a chosen order
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    int pids[3];
    for (int i = 0; i < 3; i++) {
        int rc = fork();
        if (rc < 0) { fprintf(stderr, "fork failed\n"); exit(1); }
        if (rc == 0) {
            sleep(3 - i);          // child 0 sleeps longest, child 2 shortest
            printf("child %d (pid:%d) exiting\n", i, (int) getpid());
            exit(10 + i);
        }
        pids[i] = rc;
    }
    // Wait for children in creation order, regardless of who finishes first.
    for (int i = 0; i < 3; i++) {
        int status;
        int w = waitpid(pids[i], &status, 0);
        printf("parent: waitpid(%d) returned %d, exit status %d\n",
               pids[i], w, WEXITSTATUS(status));
    }
    // Non-blocking check: no children left, so this returns -1 (ECHILD).
    printf("parent: waitpid(-1, WNOHANG) now returns %d\n",
           waitpid(-1, NULL, WNOHANG));
    return 0;
}
