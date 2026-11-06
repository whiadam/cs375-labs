#include <iostream>
#include <string>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

std::string timestamp() {
    char buf[32];
    std::time_t now = std::time(nullptr);
    std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&now));
    return std::string("[") + buf + "] ";
}

int main(int argc, char* argv[]) {
    int port = 8080;  // Default port
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "Invalid port: " << argv[1] << std::endl;
            return 1;
        }
        if (port < 1 || port > 65535) {
            std::cerr << "Port must be between 1 and 65535" << std::endl;
            return 1;
        }
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0) {  // Backlog of 5
        perror("listen");
        close(server_fd);
        return 1;
    }

    std::cout << timestamp() << "Echo server listening on port " << port << std::endl;

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        std::cout << timestamp() << "Client connected from " << inet_ntoa(client_addr.sin_addr) << std::endl;

        char buffer[1024];
        ssize_t bytes_read;
        size_t total = 0;
        while ((bytes_read = read(client_fd, buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';
            std::cout << timestamp() << "Received: " << buffer << std::endl;
            write(client_fd, buffer, bytes_read);  // Echo back
            total += bytes_read;
        }
        if (bytes_read < 0) {
            perror("read");
        }

        std::cout << timestamp() << "Client disconnected, " << total << " bytes echoed" << std::endl;
        close(client_fd);
    }

    close(server_fd);
    return 0;
}
