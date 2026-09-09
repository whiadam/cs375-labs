// ex1_basic_fork.c
// Makes a child with fork.
// The parent prints one message and the child prints another.
// fork returns the child pid to the parent and 0 to the child.
#include <stdio.h>
#include <unistd.h>

int main() {
    pid_t pid = fork();

    if (pid > 0) {
        printf("I am the parent, PID: %d\n", getpid());
    } else if (pid == 0) {
        printf("I am the child, PID: %d\n", getpid());
    } else {
        printf("Fork failed!\n");
    }
    return 0;
}
