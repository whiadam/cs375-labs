// ch3_exec_ls.c
// Fork, then the child runs ls -la. The parent waits and prints a message after.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        execlp("ls", "ls", "-la", (char *)NULL);
        perror("execlp"); // only runs if exec failed
        _exit(1);
    } else {
        waitpid(pid, NULL, 0);
        printf("[parent] child finished ls -la\n");
    }
    return 0;
}
