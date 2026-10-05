#include <iostream>
#include <csignal>
#include "../common/protocol.h"
#include "../common/handler.h"
#include "threadpool.h"

#define THREADS 4
#define MAX_CONNECTIONS 16

int main() {
    signal(SIGPIPE, SIG_IGN);

    int server_fd = make_listen_socket(10);
    if (server_fd < 0) return 1;

    ThreadPool pool(THREADS);

    log_line("Thread pool server running on port " + std::to_string(PORT) + " with " +
             std::to_string(THREADS) + " workers, max " + std::to_string(MAX_CONNECTIONS) + " connections");

    while (true) {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0) {
            if (errno == EINTR) continue;
            log_error("accept");
            continue;
        }

        std::string ip = peer_ip(client_socket);

        if (pool.pending() >= MAX_CONNECTIONS) {
            send_u8(client_socket, GREET_BUSY);
            close(client_socket);
            log_line("[conn] rejected " + ip + " (server full)");
            continue;
        }

        if (!send_u8(client_socket, GREET_READY)) {
            log_error("send greeting");
            close(client_socket);
            continue;
        }

        log_line("[conn] accepted " + ip);
        pool.enqueue(client_socket);
    }

    close(server_fd);
    return 0;
}
