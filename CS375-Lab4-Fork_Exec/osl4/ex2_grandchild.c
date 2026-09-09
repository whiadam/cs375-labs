// ex2_grandchild.c
// The parent makes a child, and the child makes its own child.
// That second child is the grandchild.
// So there are three processes: parent, child, grandchild.
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid1 = fork();

    if (pid1 > 0) {
        // parent
        printf("I am the parent, PID: %d\n", getpid());
        wait(NULL); // wait for the child to finish
    } else if (pid1 == 0) {
        // child
        printf("I am the child, PID: %d\n", getpid());
        fflush(stdout); // flush before fork so the grandchild does not copy this line
        pid_t pid2 = fork();
        if (pid2 == 0) {
            // grandchild
            printf("I am the grandchild, PID: %d\n", getpid());
        } else {
            wait(NULL); // child waits for the grandchild
        }
    } else {
        printf("Fork failed!\n");
    }
    return 0;
}
