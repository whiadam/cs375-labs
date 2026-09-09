// exec_demo.c
// This replaces the child with the ls -l program using execlp.
// After exec works the rest of the child code does not run.
// The lines after exec only run if exec failed.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }

    if (pid == 0) {
        // child turns into ls -l. execlp looks for ls in PATH.
        printf("[child] about to exec ls -l\n");
        fflush(stdout); // flush before exec wipes this process
        execlp("ls", "ls", "-l", (char *)NULL);
        perror("execlp"); // only gets here if exec failed
        _exit(1);
    } else {
        waitpid(pid, NULL, 0);
        printf("[parent] child finished exec\n");
    }
    return 0;
}
