// q1.c - fork() and a variable set before the fork
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    int x = 100;
    printf("parent (pid:%d) before fork: x = %d\n", (int) getpid(), x);

    fflush(stdout);   // flush stdio buffer so the child does not inherit (and reprint) it
    int rc = fork();
    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        // child: gets its own COPY of the parent's address space
        printf("child  (pid:%d) sees x = %d\n", (int) getpid(), x);
        x = 200;
        printf("child  (pid:%d) changed x to %d\n", (int) getpid(), x);
    } else {
        x = 300;
        printf("parent (pid:%d) changed x to %d\n", (int) getpid(), x);
        wait(NULL);
        printf("parent (pid:%d) after child exits: x = %d\n", (int) getpid(), x);
    }
    return 0;
}
