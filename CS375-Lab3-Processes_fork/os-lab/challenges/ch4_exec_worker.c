// ch4_exec_worker.c
// Fork and use execve to run the worker program.
// execve lets you pass your own argument list and your own environment.
// Here the environment sets MYVAR=hello.
// Run this from the project root so the path bin/worker works.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        char *args[] = { "worker", "alpha", "beta", NULL };
        char *env[]  = { "MYVAR=hello", NULL };
        execve("bin/worker", args, env); // normal case: run from project root
        execve("./worker",   args, env); // backup: worker is in this folder
        perror("execve worker"); // only runs if both tries failed
        _exit(1);
    } else {
        waitpid(pid, NULL, 0);
        printf("[parent] worker finished\n");
    }
    return 0;
}
