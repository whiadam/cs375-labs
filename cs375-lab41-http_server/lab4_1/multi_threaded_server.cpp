#include <iostream>
#include <string>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
int total_connections = 0;

std::string timestamp() {
    char buf[32];
    std::time_t now = std::time(nullptr);
    std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&now));
    return std::string("[") + buf + "] ";
}

void log_msg(const std::string& msg) {
    pthread_mutex_lock(&log_mutex);
    std::cout << timestamp() << "[thread " << pthread_self() % 10000 << "] " << msg << std::endl;
    pthread_mutex_unlock(&log_mutex);
}

void* handle_client(void* arg) {
    int client_fd = *(int*)arg;
    free(arg);  // Free the allocated int

    pthread_mutex_lock(&counter_mutex);
    total_connections++;
    int my_number = total_connections;
    pthread_mutex_unlock(&counter_mutex);

    log_msg("Handling connection #" + std::to_string(my_number));

    char buffer[1024];
    ssize_t bytes_read;
    while ((bytes_read = read(client_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        log_msg(std::string("Received: ") + buffer);
        write(client_fd, buffer, bytes_read);
    }
    if (bytes_read < 0) {
        perror("read");
    }
    close(client_fd);
    log_msg("Finished connection #" + std::to_string(my_number));
    return nullptr;
}

int main(int argc, char* argv[]) {
    int port = 8080;
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

    // ... (same socket setup as Part 1)
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

    if (listen(server_fd, 128) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    std::cout << "Multi-threaded echo server listening on port " << port << std::endl;

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        log_msg(std::string("Client connected from ") + inet_ntoa(client_addr.sin_addr));

        int* client_ptr = (int*)malloc(sizeof(int));
        *client_ptr = client_fd;

        pthread_t thread;
        if (pthread_create(&thread, nullptr, handle_client, client_ptr) != 0) {
            perror("pthread_create");
            close(client_fd);
            free(client_ptr);
            continue;
        }
        pthread_detach(thread);  // No need to join
    }

    close(server_fd);
    return 0;
}
