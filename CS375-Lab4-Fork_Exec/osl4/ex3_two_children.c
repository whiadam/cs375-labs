// ex3_two_children.c
// The parent makes two children.
// The starter code called fork twice in a row, which actually makes four
// processes. So instead I only fork the second time inside the parent branch.
// That way there are exactly two children.
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid1 = fork();

    if (pid1 == 0) {
        // this is child 1
        printf("Child 1, PID: %d\n", getpid());
    } else {
        pid_t pid2 = fork();
        if (pid2 == 0) {
            // this is child 2
            printf("Child 2, PID: %d\n", getpid());
        } else {
            // this is the parent
            printf("I am the parent, PID: %d\n", getpid());
            wait(NULL); // wait for both children
            wait(NULL);
        }
    }
    return 0;
}
