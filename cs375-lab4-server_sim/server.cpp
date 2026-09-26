
#include <iostream>
#include <cstring>
#include <csignal>
#include <cerrno>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT  8080
#define BUFSZ 1024

static volatile sig_atomic_t stop_server = 0;

static void reap_children(int) {
    int saved = errno;
    while (waitpid(-1, nullptr, WNOHANG) > 0)
        ;
    errno = saved;
}

static void handle_shutdown(int) { stop_server = 1; }

static void handle_client(int client_sock) {
    char buffer[BUFSZ];
    ssize_t n = read(client_sock, buffer, sizeof(buffer) - 1);
    if (n < 0) { perror("read"); close(client_sock); return; }
    buffer[n] = '\0';
    std::cout << "Received (" << n << " bytes): " << buffer << std::endl;

    if (std::strncmp(buffer, "shutdown", 8) == 0) {
        const char *msg = "Server shutting down\n";
        write(client_sock, msg, std::strlen(msg));
        close(client_sock);
        kill(getppid(), SIGTERM);
        _exit(0);
    }

    const char *reply = "Hello from C++ server";
    write(client_sock, reply, std::strlen(reply));
    close(client_sock);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr{}, client_addr{};
    socklen_t addr_size;

    struct sigaction sa_chld{};
    sa_chld.sa_handler = reap_children;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa_chld, nullptr);

    struct sigaction sa_term{};
    sa_term.sa_handler = handle_shutdown;
    sigemptyset(&sa_term.sa_mask);
    sa_term.sa_flags = 0;
    sigaction(SIGTERM, &sa_term, nullptr);
    sigaction(SIGINT,  &sa_term, nullptr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind"); return 1;
    }
    if (listen(server_sock, 5) < 0) { perror("listen"); return 1; }

    std::cout << "C++ server listening on 127.0.0.1:" << PORT
              << "  (send \"shutdown\" to stop)" << std::endl;

    while (!stop_server) {
        addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
        if (client_sock < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(client_sock);
        } else if (pid == 0) {
            close(server_sock);
            handle_client(client_sock);
            _exit(0);
        } else {
            close(client_sock);
        }
    }

    close(server_sock);
    std::cout << "Server stopped cleanly." << std::endl;
    return 0;
}
