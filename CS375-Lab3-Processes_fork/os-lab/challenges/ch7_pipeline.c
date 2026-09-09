// ch7_pipeline.c
// This builds the command ls | grep pattern by hand.
// The pattern comes from the command line.
// child 1 runs ls and writes into the pipe.
// child 2 runs grep and reads from the pipe.
// run it like: ./ch7_pipeline "\.c$"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <pattern>\n", argv[0]);
        return 1;
    }

    int fd[2];
    if (pipe(fd) == -1) { perror("pipe"); return 1; }

    pid_t p1 = fork(); // ls into the pipe
    if (p1 < 0) { perror("fork"); return 1; }
    if (p1 == 0) {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        execlp("ls", "ls", (char *)NULL);
        perror("execlp ls");
        _exit(1);
    }

    pid_t p2 = fork(); // grep out of the pipe
    if (p2 < 0) { perror("fork"); return 1; }
    if (p2 == 0) {
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        close(fd[1]);
        execlp("grep", "grep", argv[1], (char *)NULL);
        perror("execlp grep");
        _exit(1);
    }

    close(fd[0]); // parent closes both ends
    close(fd[1]);
    waitpid(p1, NULL, 0);
    waitpid(p2, NULL, 0);
    return 0;
}
