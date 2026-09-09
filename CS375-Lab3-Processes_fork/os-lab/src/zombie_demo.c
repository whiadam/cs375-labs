// zombie_demo.c
// A zombie is a child that already exited but the parent has not waited for it yet.
// The child stays in the process table until the parent calls wait.
// While the parent sleeps below you can open another terminal and run:
//   ps -el | grep defunct
// The child shows up as Z until the parent reaps it.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }

    if (pid == 0) {
        printf("[child] PID=%d exiting immediately\n", getpid());
        fflush(stdout);
        _exit(0);
    } else {
        printf("[parent] PID=%d created child %d\n", getpid(), pid);
        printf("[parent] sleeping 10s without reaping so the child is a zombie now\n");
        printf("[parent] in another terminal run:  ps -el | grep defunct\n");
        fflush(stdout);
        sleep(10);
        wait(NULL); // reap the child so the zombie goes away
        printf("[parent] reaped child; zombie gone\n");
    }
    return 0;
}
