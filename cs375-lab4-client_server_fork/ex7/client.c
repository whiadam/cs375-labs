// Exercise 7: Fork with Broadcast (client)
// Uses select() to watch both the keyboard and the socket, so broadcast
// messages from other clients are displayed even while waiting for input.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 8080
#define BUF_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUF_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect"); exit(1);
    }
    printf("Connected. Type messages to broadcast (\"exit\" to quit).\n");

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sock, &readfds);

        if (select(sock + 1, &readfds, NULL, NULL, NULL) < 0) { perror("select"); break; }

        /* incoming broadcast */
        if (FD_ISSET(sock, &readfds)) {
            ssize_t n = read(sock, buffer, sizeof(buffer) - 1);
            if (n <= 0) { printf("Disconnected from server.\n"); break; }
            buffer[n] = '\0';
            printf("%s", buffer);
            fflush(stdout);
        }

        /* user typed something */
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (fgets(buffer, sizeof(buffer), stdin) == NULL) break;   /* Ctrl-D */
            buffer[strcspn(buffer, "\n")] = '\0';
            if (strlen(buffer) == 0) continue;
            send(sock, buffer, strlen(buffer), 0);
            if (strcmp(buffer, "exit") == 0) break;
        }
    }
    close(sock);
    return 0;
}
