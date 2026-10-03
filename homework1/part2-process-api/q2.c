// q2.c - open() a file, then fork(); both processes write to it
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main(void) {
    int fd = open("./q2.output", O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
    if (fd < 0) { perror("open"); exit(1); }

    fflush(stdout);
    int rc = fork();
    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    const char *msg = (rc == 0) ? "child  writing a line\n"
                                : "parent writing a line\n";
    for (int i = 0; i < 10000; i++) {
        if (write(fd, msg, strlen(msg)) < 0) perror("write");
    }
    printf("%s (pid:%d) used fd %d\n", rc == 0 ? "child " : "parent",
           (int) getpid(), fd);
    if (rc > 0) wait(NULL);
    close(fd);
    return 0;
}
