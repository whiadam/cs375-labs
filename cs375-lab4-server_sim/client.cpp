
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT  8080
#define BUFSZ 1024

int main(int argc, char **argv) {
    std::string msg = "Hello C++ Server";
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--big") msg = std::string(10000, 'A');   /* Q4 */
        else                msg = arg;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect"); return 1;
    }

    send(sock, msg.c_str(), msg.size(), 0);

    char buffer[BUFSZ];
    ssize_t n = read(sock, buffer, sizeof(buffer) - 1);
    if (n >= 0) {
        buffer[n] = '\0';
        std::cout << "Server response: " << buffer << std::endl;
    }
    close(sock);
    return 0;
}
