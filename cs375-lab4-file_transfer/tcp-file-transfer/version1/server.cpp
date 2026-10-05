#include <iostream>
#include <csignal>
#include <chrono>
#include "../common/protocol.h"
#include "../common/handler.h"

int main() {
    signal(SIGPIPE, SIG_IGN);

    int server_fd = make_listen_socket(5);
    if (server_fd < 0) return 1;

    log_line("Single-threaded server listening on port " + std::to_string(PORT));

    while (true) {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0) {
            if (errno == EINTR) continue;
            log_error("accept");
            continue;
        }

        log_line("[conn] accepted " + peer_ip(client_socket));

        if (!send_u8(client_socket, GREET_READY)) {
            log_error("send greeting");
            close(client_socket);
            continue;
        }

        auto start = std::chrono::steady_clock::now();
        handle_client(client_socket);
        close(client_socket);
        log_line("[conn] closed after " + std::to_string(seconds_since(start)) + " s");
    }

    close(server_fd);
    return 0;
}
