// ch1_single_fork.c
// Fork one time. The child says hello, sleeps 2 seconds, and exits with code 7.
// The parent waits and prints the exit code.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        printf("Hello from child (PID=%d, PPID=%d)\n", getpid(), getppid());
        sleep(2);
        fflush(stdout);
        _exit(7);
    } else {
        int status;
        pid_t w = waitpid(pid, &status, 0);
        if (w == -1) { perror("waitpid"); return 1; }
        if (WIFEXITED(status))
            printf("child %d exited with status %d\n", w, WEXITSTATUS(status));
        else
            printf("child %d did not exit normally\n", w);
    }
    return 0;
}
