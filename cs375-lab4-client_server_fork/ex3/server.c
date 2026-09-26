// Exercise 3: Fork with Multiple Messages (server)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUF_SIZE 1024

void handle_client(int client_sock) {
    char buffer[BUF_SIZE];
    char reply[BUF_SIZE + 16];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t n = read(client_sock, buffer, sizeof(buffer) - 1);
        if (n <= 0) {                       /* client closed or error */
            printf("[child %d] Client disconnected\n", getpid());
            break;
        }
        buffer[strcspn(buffer, "\r\n")] = '\0';   /* strip newline */

        if (strcmp(buffer, "exit") == 0) {
            printf("[child %d] Client sent exit\n", getpid());
            break;
        }

        printf("[child %d] Received: %s\n", getpid(), buffer);
        snprintf(reply, sizeof(reply), "Echo: %s", buffer);
        write(client_sock, reply, strlen(reply));
    }
    close(client_sock);
}

int main() {
    /* line-buffer stdout so fork() doesn't duplicate unflushed output */
    setvbuf(stdout, NULL, _IOLBF, 0);

    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    int opt = 1;

    signal(SIGCHLD, SIG_IGN);   /* auto-reap finished children (no zombies) */

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind"); exit(1);
    }
    listen(server_sock, 5);
    printf("Server listening on port %d...\n", PORT);

    while (1) {
        addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
        if (client_sock < 0) continue;

        printf("New connection from %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        if (fork() == 0) {            /* child */
            close(server_sock);       /* child doesn't need the listener */
            handle_client(client_sock);
            exit(0);
        }
        close(client_sock);           /* parent doesn't need the client socket */
    }
    return 0;
}
