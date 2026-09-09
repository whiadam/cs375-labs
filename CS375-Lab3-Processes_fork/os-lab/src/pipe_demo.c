// pipe_demo.c
// The parent writes some lines into a pipe and the child runs wc -l on them.
// A pipe has two ends. fd[0] is the read end and fd[1] is the write end.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    int fd[2];
    if (pipe(fd) == -1) { perror("pipe"); exit(1); }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }

    if (pid == 0) {
        // child uses the read end as its stdin, then runs wc -l
        close(fd[1]); // child does not write
        dup2(fd[0], STDIN_FILENO); // make the read end the stdin
        close(fd[0]);
        execlp("wc", "wc", "-l", (char *)NULL);
        perror("execlp wc");
        _exit(1);
    } else {
        // parent writes 5 lines then closes to send the end signal
        close(fd[0]); // parent does not read
        FILE *out = fdopen(fd[1], "w");
        for (int i = 0; i < 5; ++i)
            fprintf(out, "line %d\n", i);
        fclose(out); // closing the write end tells the child it is done
        waitpid(pid, NULL, 0);
    }
    return 0;
}
