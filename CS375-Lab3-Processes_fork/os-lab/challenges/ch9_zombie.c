// ch9_zombie.c
// Make a zombie on purpose.
// The child exits right away but the parent sleeps 10 seconds before waiting.
// During those 10 seconds the child is a zombie.
// To see it, open another terminal and run:
//   ps -el | grep defunct
// After the parent waits, run ps again and it is gone.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        _exit(0); // child exits now and becomes a zombie until it is reaped
    } else {
        printf("[parent] child %d created; sleeping 10s (child is a zombie now)\n", pid);
        printf("[parent] check in another terminal:  ps -el | grep defunct\n");
        fflush(stdout);
        sleep(10);
        wait(NULL); // reap the child so the zombie goes away
        printf("[parent] child reaped; zombie gone\n");
    }
    return 0;
}
