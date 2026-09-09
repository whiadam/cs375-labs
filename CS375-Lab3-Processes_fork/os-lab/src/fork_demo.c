// fork_demo.c
// This shows a basic fork and waitpid.
// fork makes a copy of the process, so after it runs there are two processes.
// The return value tells you which one you are:
//   less than 0: fork failed
//   equal to 0: you are the child
//   more than 0: you are the parent, and the number is the child pid
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // this part runs in the child
        printf("[child] PID=%d, PPID=%d\n", getpid(), getppid());
        sleep(1); // wait a bit so the parent line prints too
        printf("[child] exiting with code 42\n");
        fflush(stdout); // flush now because _exit does not flush
        _exit(42);
    } else {
        // this part runs in the parent
        printf("[parent] created child PID=%d\n", pid);
        int status;
        pid_t w = waitpid(pid, &status, 0);
        if (w == -1) {
            perror("waitpid");
            return 1;
        }
        if (WIFEXITED(status)) {
            printf("[parent] child %d exited with status %d\n", w, WEXITSTATUS(status));
        } else {
            printf("[parent] child %d did not exit normally\n", w);
        }
    }
    return 0;
}
