// ch5_exec_examples.c
// Two children print the same thing two different ways.
// Child A uses execl where you list the arguments one by one.
// Child B uses execv where you pass the arguments as an array.
// Both should print: one two
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    // child A uses execl
    pid_t a = fork();
    if (a < 0) { perror("fork"); return 1; }
    if (a == 0) {
        execl("/bin/echo", "echo", "one", "two", (char *)NULL);
        perror("execl");
        _exit(1);
    }
    waitpid(a, NULL, 0);

    // child B uses execv
    pid_t b = fork();
    if (b < 0) { perror("fork"); return 1; }
    if (b == 0) {
        char *argv[] = { "echo", "one", "two", NULL };
        execv("/bin/echo", argv);
        perror("execv");
        _exit(1);
    }
    waitpid(b, NULL, 0);
    return 0;
}
