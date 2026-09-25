3.1 How do we stop the program?
Ctrl-C won't work because SIGINT is being ignored. You can Ctrl-Z to stop it and then kill it. 

3.1.1 Did you really want to quit?
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

void handler(int sig) {
    char c, extra;
    char msg[] = "Do you really want to quit [y/n]? ";
    write(1, msg, sizeof(msg) - 1);
    read(0, &c, 1);
    // read the rest of the line so the enter key doesnt mess up next time
    if (c != '\n')
        while (read(0, &extra, 1) == 1 && extra != '\n');
    if (c == 'y')
        _exit(0);
    signal(SIGINT, handler);
}

int main() {
    signal(SIGINT, handler);
    while (1);
}
I used write and read instead of printf and scanf because they are syscalls and they are safe to use in a signal handler. I used _exit instead of exit for the same reason.

3.2 fopen vs open

open is a system call and it gives you back an int which is the file descriptor. It uses flags like O_RDONLY and O_CREAT. It does not buffer anything so every read or write goes right to the kernel.

fopen is from the C library and it gives you back a FILE pointer. It uses r&w. It has a buffer so it does not make a syscall every time you write. That means the data might not be in the file until you call fflush or fclose. fopen actually calls open inside of it.

4.1.1 Quick practice with write and seek
The file ends up being 700 bytes. The first write puts 200 a's in. Then it seeks back to 0 and reads 100 bytes, so the offset is at 100. Then it seeks 500 more so it's at 600, and writes 100 a's from 600 to 699. So it's 200 a's, then 400 zero bytes, then 100 a's. The part from 200 to 599 is zeros because it wrote past the end of the file.

4.2.1 Warmup
"Luke, I am your..." prints to the terminal and "father" goes into output_file.txt. The first printf has a newline so it prints right away. Then dup2 makes stdout point to the file, so the second printf ends up in the file when the program exits.

4.2.2 Redirection: executing a process after dup2
O_CAT is supposed to be O_CREAT. The program opens the file you give it and prints "writing output of the command /bin/ls to "filename"" to the terminal. Then dup2 makes stdout go to the file and execvp runs ls -al /. Since ls keeps the same file descriptors, the listing of the root directory goes into the file instead of the screen. The perror and exit only run if execvp fails. So the terminal only shows the "writing output..." line and the file has the ls output.

4.2.3 Redirecting in a new process
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
    int pid, status;
    int newfd;
    char *cmd[] = { "/bin/ls", "-al", "/", 0 };

    if (argc != 2) {
        fprintf(stderr, "usage: %s output_file\n", argv[0]);
        exit(1);
    }
    if ((newfd = open(argv[1], O_CREAT|O_TRUNC|O_WRONLY, 0644)) < 0) {
        perror(argv[1]);
        exit(1);
    }
    printf("writing output of the command %s to \"%s\"\n", cmd[0], argv[1]);
    fflush(stdout);

    pid = fork();
    if (pid == 0) {
        // child
        dup2(newfd, 1);
        close(newfd);
        execvp(cmd[0], cmd);
        perror(cmd[0]);
        exit(1);
    }

    // parent
    close(newfd);
    wait(&status);
    printf("all done\n");
    return 0;
}

The child does the dup2 and runs ls, so only the child's output goes to the file. The parent waits for the child to finish and then prints "all done" to the terminal, since its stdout never got changed. I put fflush before fork so the printf doesn't get printed twice.

