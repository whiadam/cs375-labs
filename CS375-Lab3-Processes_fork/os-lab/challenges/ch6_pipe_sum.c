// ch6_pipe_sum.c
// The parent writes the numbers 1 to 10 into a pipe.
// The child reads them and adds them up.
// The answer should be Sum = 55.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    int fd[2];
    if (pipe(fd) == -1) { perror("pipe"); return 1; }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        // child reads numbers until the pipe is done, then adds them up
        close(fd[1]);
        int val, sum = 0;
        while (read(fd[0], &val, sizeof(val)) == (ssize_t)sizeof(val))
            sum += val;
        close(fd[0]);
        printf("Sum = %d\n", sum);
        fflush(stdout);
        _exit(0);
    } else {
        // parent writes 1 to 10 then closes the write end
        close(fd[0]);
        for (int i = 1; i <= 10; ++i)
            write(fd[1], &i, sizeof(i));
        close(fd[1]);
        waitpid(pid, NULL, 0);
    }
    return 0;
}
