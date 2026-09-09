// ch8_wait_nonblock.c
// Make 3 children that sleep 1, 2, and 3 seconds and then exit.
// The parent uses waitpid with WNOHANG so it does not get stuck waiting.
// It keeps checking and prints each child as soon as it finishes.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    int sleeps[3] = { 1, 2, 3 };

    for (int i = 0; i < 3; ++i) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); return 1; }
        if (pid == 0) {
            sleep(sleeps[i]);
            printf("[child] PID=%d slept %ds\n", getpid(), sleeps[i]);
            fflush(stdout);
            _exit(sleeps[i]);
        }
    }

    int remaining = 3;
    while (remaining > 0) {
        int status;
        pid_t w = waitpid(-1, &status, WNOHANG);
        if (w == -1) { perror("waitpid"); break; }
        if (w == 0) {
            // no child is done yet, so wait a little and check again
            usleep(100 * 1000); // 100 ms
            continue;
        }
        if (WIFEXITED(status))
            printf("[parent] reaped child %d (exit %d)\n", w, WEXITSTATUS(status));
        remaining--;
    }
    printf("[parent] all children done\n");
    return 0;
}
