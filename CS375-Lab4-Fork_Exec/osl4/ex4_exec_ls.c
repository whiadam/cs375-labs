// ex4_exec_ls.c
// One child runs the ls program using execlp.
// After exec works the child code stops, so the lines after exec only run
// if exec failed. The parent waits and then prints a message.
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        // child turns into ls -l
        printf("Executing ls command...\n");
        fflush(stdout); // flush before exec replaces this process
        execlp("ls", "ls", "-l", NULL);
        perror("execlp"); // only runs if exec failed
        return 1;
    } else {
        wait(NULL);
        printf("Parent process finished.\n");
    }
    return 0;
}
