// q5.c - parent uses wait(); also try wait() in the child
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    int rc = fork();
    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        printf("child  (pid:%d) running\n", (int) getpid());
        int w = wait(NULL);        // child has no children of its own
        printf("child  wait() returned %d (errno=%d: %s)\n",
               w, errno, strerror(errno));
        exit(7);
    } else {
        int status;
        int w = wait(&status);
        printf("parent (pid:%d) wait() returned %d (child pid was %d)\n",
               (int) getpid(), w, rc);
        if (WIFEXITED(status))
            printf("parent: child exited with status %d\n", WEXITSTATUS(status));
    }
    return 0;
}
