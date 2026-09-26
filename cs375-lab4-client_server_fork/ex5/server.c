// Exercise 5: Fork with Timeout (server)
// Each child waits at most 10 seconds for the next message. If nothing
// arrives in time, it closes the connection and ends.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 8080
#define BUF_SIZE 1024
#define TIMEOUT_SEC 10

void handle_client(int client_sock) {
    char buffer[BUF_SIZE];

    while (1) {
        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(client_sock, &readfds);
        tv.tv_sec = TIMEOUT_SEC;          /* must be reset on every loop */
        tv.tv_usec = 0;

        int ready = select(client_sock + 1, &readfds, NULL, NULL, &tv);
        if (ready < 0) { perror("select"); break; }
        if (ready == 0) {                 /* timed out */
            printf("[child %d] No message for %d seconds. Closing connection.\n",
                   getpid(), TIMEOUT_SEC);
            const char *msg = "Timeout: connection closed by server";
            write(client_sock, msg, strlen(msg));
            break;
        }

        memset(buffer, 0, sizeof(buffer));
        ssize_t n = read(client_sock, buffer, sizeof(buffer) - 1);
        if (n <= 0) {
            printf("[child %d] Client disconnected\n", getpid());
            break;
        }
        buffer[strcspn(buffer, "\r\n")] = '\0';
        printf("[child %d] Received: %s\n", getpid(), buffer);
        write(client_sock, "Hello from server", 17);
    }
    close(client_sock);
}

int main() {
    /* line-buffer stdout so fork() doesn't duplicate unflushed output */
    setvbuf(stdout, NULL, _IOLBF, 0);

    int server_sock, client_sock, opt = 1;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    signal(SIGCHLD, SIG_IGN);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind"); exit(1);
    }
    listen(server_sock, 5);
    printf("Server listening on port %d (timeout %ds)...\n", PORT, TIMEOUT_SEC);

    while (1) {
        addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
        if (client_sock < 0) continue;

        if (fork() == 0) {
            close(server_sock);
            printf("[child %d] Handling new client\n", getpid());
            handle_client(client_sock);
            printf("[child %d] Terminating\n", getpid());
            exit(0);
        }
        close(client_sock);
    }
    return 0;
}
