// q3.c - child prints "hello" before parent prints "goodbye", without wait()
// A pipe is used: the parent blocks in read() until the child writes a byte,
// which the child only does after it has printed "hello".
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    int fds[2];
    if (pipe(fds) < 0) { perror("pipe"); exit(1); }

    int rc = fork();
    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        close(fds[0]);
        printf("hello\n");
        fflush(stdout);
        char c = 'x';
        write(fds[1], &c, 1);      // tell parent we're done
        close(fds[1]);
    } else {
        close(fds[1]);
        char c;
        read(fds[0], &c, 1);       // blocks until child writes
        close(fds[0]);
        printf("goodbye\n");
    }
    return 0;
}
