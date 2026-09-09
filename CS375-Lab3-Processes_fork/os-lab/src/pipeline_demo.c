// pipeline_demo.c
// This does the same thing as the shell command: ls | grep .c
// It uses one pipe and two children.
// child 1 runs ls and sends its output into the pipe.
// child 2 runs grep and reads from the pipe.
// The parent has to close both ends or grep waits forever.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    int fd[2];
    if (pipe(fd) == -1) { perror("pipe"); exit(1); }

    pid_t p1 = fork(); // ls into the pipe
    if (p1 < 0) { perror("fork"); exit(1); }
    if (p1 == 0) {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        execlp("ls", "ls", (char *)NULL);
        perror("execlp ls");
        _exit(1);
    }

    pid_t p2 = fork(); // grep out of the pipe
    if (p2 < 0) { perror("fork"); exit(1); }
    if (p2 == 0) {
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        close(fd[1]);
        execlp("grep", "grep", "\\.c", (char *)NULL);
        perror("execlp grep");
        _exit(1);
    }

    // parent closes both ends so grep sees the end, then waits for both
    close(fd[0]);
    close(fd[1]);
    waitpid(p1, NULL, 0);
    waitpid(p2, NULL, 0);
    return 0;
}
