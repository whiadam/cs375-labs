// Exercise 6: Fork with Error Handling (C++ server)
// All errors are written with a timestamp to server_errors.log instead of
// the console. The server keeps running if a child or a single call fails.
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cerrno>
#include <ctime>
#include <csignal>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define LOG_FILE "server_errors.log"

// Append "[YYYY-MM-DD HH:MM:SS] [pid N] where: strerror(errno)" to the log
void log_error(const std::string &where) {
    int saved_errno = errno;
    std::ofstream log(LOG_FILE, std::ios::app);
    if (!log) return;                         // nothing else we can do
    time_t now = time(nullptr);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
    log << "[" << ts << "] [pid " << getpid() << "] " << where
        << ": " << strerror(saved_errno) << "\n";
}

void handle_client(int client_sock) {
    char buffer[1024] = {0};

    ssize_t n = read(client_sock, buffer, sizeof(buffer) - 1);
    if (n < 0) {
        log_error("read() failed");
        close(client_sock);
        return;
    }
    if (n == 0) {                             // client closed without sending
        close(client_sock);
        return;
    }
    std::cout << "Received: " << buffer << std::endl;

    const char *reply = "Hello from C++ server";
    if (write(client_sock, reply, strlen(reply)) < 0)
        log_error("write() failed");

    if (close(client_sock) < 0)
        log_error("close() failed");
}

int main() {
    signal(SIGCHLD, SIG_IGN);   // auto-reap children
    signal(SIGPIPE, SIG_IGN);   // a write to a dead client returns EPIPE instead of killing us

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        log_error("socket() failed");
        return 1;               // can't run without a socket
    }

    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        log_error("setsockopt() failed");   // not fatal

    struct sockaddr_in server_addr{}, client_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        log_error("bind() failed");
        close(server_sock);
        return 1;
    }
    if (listen(server_sock, 5) < 0) {
        log_error("listen() failed");
        close(server_sock);
        return 1;
    }
    std::cout << "C++ server listening on port " << PORT
              << " (errors -> " << LOG_FILE << ")" << std::endl;

    while (true) {
        socklen_t addr_size = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
        if (client_sock < 0) {
            if (errno != EINTR) log_error("accept() failed");
            continue;           // keep serving other clients
        }

        pid_t pid = fork();
        if (pid < 0) {
            log_error("fork() failed");
            close(client_sock); // drop this client, keep the server alive
            continue;
        }
        if (pid == 0) {         // child
            close(server_sock);
            handle_client(client_sock);
            _exit(0);           // child errors never reach the parent loop
        }
        close(client_sock);     // parent
    }
    return 0;
}
