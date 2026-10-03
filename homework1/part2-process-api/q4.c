// q4.c - fork() then run /bin/ls with every exec() variant
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

extern char **environ;

static void run(const char *name) {
    int rc = fork();
    if (rc < 0) { fprintf(stderr, "fork failed\n"); exit(1); }
    if (rc == 0) {
        char *argv[] = { "ls", NULL };
        char *envp[] = { "PATH=/bin:/usr/bin", NULL };
        printf("----- %s -----\n", name);
        fflush(stdout);
        if      (!strcmp(name, "execl"))   execl("/bin/ls", "ls", (char *) NULL);
        else if (!strcmp(name, "execle"))  execle("/bin/ls", "ls", (char *) NULL, envp);
        else if (!strcmp(name, "execlp"))  execlp("ls", "ls", (char *) NULL);
        else if (!strcmp(name, "execv"))   execv("/bin/ls", argv);
        else if (!strcmp(name, "execvp"))  execvp("ls", argv);
        else if (!strcmp(name, "execvpe")) execvpe("ls", argv, environ);
        perror("exec failed");     // only reached if exec fails
        exit(1);
    }
    wait(NULL);
}

int main(void) {
    const char *variants[] = { "execl", "execle", "execlp",
                               "execv", "execvp", "execvpe" };
    for (int i = 0; i < 6; i++) run(variants[i]);
    return 0;
}
