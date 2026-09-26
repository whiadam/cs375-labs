
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT   8080
#define BUFSZ  1024

int main(int argc, char **argv) {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFSZ];

    /* build the outgoing message */
    char big[10001];
    const char *msg = "Hello Server";
    if (argc > 1) {
        if (strcmp(argv[1], "--big") == 0) {
            memset(big, 'A', 10000);       /* Q4: 10,000-char payload */
            big[10000] = '\0';
            msg = big;
        } else {
            msg = argv[1];
        }
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return 1;
    }

    send(sock, msg, strlen(msg), 0);

    ssize_t n = read(sock, buffer, sizeof(buffer) - 1);
    if (n >= 0) {
        buffer[n] = '\0';
        printf("Server response: %s\n", buffer);
    }
    close(sock);
    return 0;
}
