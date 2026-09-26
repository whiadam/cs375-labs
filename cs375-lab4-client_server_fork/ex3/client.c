// Exercise 3: Fork with Multiple Messages (interactive client)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

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
    printf("Connected. Type messages (\"exit\" to quit).\n");

    while (1) {
        printf("> ");
        fflush(stdout);
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) break;   /* Ctrl-D */
        buffer[strcspn(buffer, "\n")] = '\0';
        if (strlen(buffer) == 0) continue;

        send(sock, buffer, strlen(buffer), 0);
        if (strcmp(buffer, "exit") == 0) break;

        memset(buffer, 0, sizeof(buffer));
        ssize_t n = read(sock, buffer, sizeof(buffer) - 1);
        if (n <= 0) { printf("Server closed connection.\n"); break; }
        printf("Server response: %s\n", buffer);
    }
    close(sock);
    return 0;
}
