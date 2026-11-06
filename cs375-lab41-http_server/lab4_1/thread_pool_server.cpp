#include <iostream>
#include <string>
#include <cstring>
#include <ctime>
#include <cerrno>
#include <csignal>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>

std::queue<int> task_queue;  // Queue of client_fds
std::mutex queue_mutex;
std::condition_variable cv;
bool stop_pool = false;
const int NUM_THREADS = 10;

std::mutex log_mutex;
volatile sig_atomic_t got_sigint = 0;

void handle_sigint(int) {
    got_sigint = 1;
}

std::string timestamp() {
    char buf[32];
    std::time_t now = std::time(nullptr);
    std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&now));
    return std::string("[") + buf + "] ";
}

void log_msg(const std::string& msg) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << timestamp() << msg << std::endl;
}

void worker_thread(int id) {
    while (true) {
        int client_fd;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            cv.wait(lock, [&] { return !task_queue.empty() || stop_pool; });
            if (stop_pool && task_queue.empty()) return;
            client_fd = task_queue.front();
            task_queue.pop();
            log_msg("Worker " + std::to_string(id) + " popped fd " + std::to_string(client_fd) +
                    ", queue size now " + std::to_string(task_queue.size()));
        }

        // Handle client
        char buffer[1024];
        ssize_t bytes_read;
        while ((bytes_read = read(client_fd, buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';
            log_msg("Worker " + std::to_string(id) + " received: " + buffer);
            write(client_fd, buffer, bytes_read);
        }
        close(client_fd);
    }
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

    // TODO: Implement socket setup (copy from Part 1)
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

    sigset_t mask, old_mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    pthread_sigmask(SIG_BLOCK, &mask, &old_mask);

    std::vector<std::thread> workers;
    for (int i = 0; i < NUM_THREADS; ++i) {
        workers.emplace_back(worker_thread, i);
    }

    pthread_sigmask(SIG_SETMASK, &old_mask, nullptr);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);

    std::cout << "Thread-pool echo server listening on port " << port << std::endl;

    while (!got_sigint) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        log_msg(std::string("Client queued from ") + inet_ntoa(client_addr.sin_addr));

        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            task_queue.push(client_fd);
            log_msg("Pushed fd " + std::to_string(client_fd) + ", queue size now " +
                    std::to_string(task_queue.size()));
        }
        cv.notify_one();
    }

    log_msg("SIGINT received, finishing queued clients...");

    // Cleanup
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        stop_pool = true;
    }
    cv.notify_all();
    for (auto& w : workers) w.join();
    close(server_fd);
    log_msg("All workers stopped, server shut down");
    return 0;
}
