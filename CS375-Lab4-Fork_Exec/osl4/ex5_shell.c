// ex5_shell.c
// A very simple shell.
// It reads a line, splits it into words, and runs it with fork and exec.
// Type exit to quit. Pressing Ctrl+D also quits.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main() {
    char command[256];

    while (1) {
        printf("myshell> ");
        fflush(stdout);

        // read a line. if fgets returns NULL it means end of input (Ctrl+D)
        if (fgets(command, sizeof(command), stdin) == NULL) {
            printf("\n");
            break;
        }

        command[strcspn(command, "\n")] = 0; // take off the newline

        if (strcmp(command, "exit") == 0) {
            break;
        }
        if (strlen(command) == 0) {
            continue; // nothing was typed, so just loop again
        }

        // split the line into words so exec can use them
        char *args[64];
        int i = 0;
        char *token = strtok(command, " ");
        while (token != NULL && i < 63) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        pid_t pid = fork();
        if (pid == 0) {
            // child runs the command the user typed
            execvp(args[0], args);
            printf("Command execution failed!\n"); // only if exec failed
            exit(1);
        } else {
            wait(NULL); // parent waits for the command to finish
        }
    }
    return 0;
}
