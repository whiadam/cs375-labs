// Exercise 4: interactive C++ client (keeps the connection open so the
// counter can be observed with several clients at once)
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect"); return 1;
    }
    std::cout << "Connected. Type messages (\"exit\" to quit).\n";

    std::string line;
    char buffer[1024];
    while (std::cout << "> " && std::getline(std::cin, line)) {
        if (line.empty()) continue;
        send(sock, line.c_str(), line.size(), 0);
        if (line == "exit") break;
        memset(buffer, 0, sizeof(buffer));
        if (read(sock, buffer, sizeof(buffer) - 1) <= 0) break;
        std::cout << "Server response: " << buffer << std::endl;
    }
    close(sock);
    return 0;
}
