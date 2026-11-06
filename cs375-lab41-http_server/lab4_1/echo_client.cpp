#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <host> <port> <message>" << std::endl;
        return 1;
    }
    std::string host = argv[1];
    if (host == "localhost") host = "127.0.0.1";
    int port;
    try {
        port = std::stoi(argv[2]);
    } catch (...) {
        std::cerr << "Invalid port: " << argv[2] << std::endl;
        return 1;
    }
    if (port < 1 || port > 65535) {
        std::cerr << "Port must be between 1 and 65535" << std::endl;
        return 1;
    }
    std::string message = argv[3];

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        std::cerr << "Invalid address: " << host << std::endl;
        close(client_fd);
        return 1;
    }

    if (connect(client_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(client_fd);
        return 1;
    }

    size_t sent = 0;
    while (sent < message.length()) {
        ssize_t n = write(client_fd, message.c_str() + sent, message.length() - sent);
        if (n < 0) {
            perror("write");
            close(client_fd);
            return 1;
        }
        sent += n;
    }
    shutdown(client_fd, SHUT_WR);

    std::string reply;
    char buffer[1024];
    ssize_t bytes_read;
    while ((bytes_read = read(client_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        reply += buffer;
    }
    std::cout << "Echo: " << reply << std::endl;

    close(client_fd);
    return 0;
}
