// Exercise 4: Fork with Client Counter (C++ server, shared memory via shmget)
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define PORT 8080
#define BUF_SIZE 1024

static int shm_id = -1;
static int *active_clients = nullptr;   // lives in shared memory

// Remove the shared memory segment when the server is stopped with Ctrl-C
void cleanup(int) {
    if (active_clients) shmdt(active_clients);
    if (shm_id != -1) shmctl(shm_id, IPC_RMID, nullptr);
    std::cout << "\nShared memory removed. Server exiting.\n";
    exit(0);
}

void handle_client(int client_sock) {
    // Atomic increment: safe even if several children update at the same time
    int count = __sync_add_and_fetch(active_clients, 1);
    std::cout << "[child " << getpid() << "] Client connected. Active clients: "
              << count << std::endl;

    char buffer[BUF_SIZE];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t n = read(client_sock, buffer, sizeof(buffer) - 1);
        if (n <= 0) break;
        buffer[strcspn(buffer, "\r\n")] = '\0';
        if (strcmp(buffer, "exit") == 0) break;
        std::cout << "[child " << getpid() << "] Received: " << buffer << std::endl;
        std::string reply = "Hello from C++ server";
        write(client_sock, reply.c_str(), reply.size());
    }
    close(client_sock);

    count = __sync_sub_and_fetch(active_clients, 1);
    std::cout << "[child " << getpid() << "] Client disconnected. Active clients: "
              << count << std::endl;
}

int main() {
    // Create a shared memory segment big enough for one int
    shm_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (shm_id < 0) { perror("shmget"); return 1; }
    active_clients = (int*)shmat(shm_id, nullptr, 0);
    if (active_clients == (int*)-1) { perror("shmat"); return 1; }
    *active_clients = 0;

    signal(SIGINT, cleanup);
    signal(SIGCHLD, SIG_IGN);   // auto-reap children

    int server_sock, client_sock, opt = 1;
    struct sockaddr_in server_addr{}, client_addr{};
    socklen_t addr_size;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind"); cleanup(0);
    }
    listen(server_sock, 5);
    std::cout << "C++ server listening on port " << PORT << std::endl;

    while (true) {
        addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
        if (client_sock < 0) continue;

        std::cout << "New connection. Active clients before this one: "
                  << *active_clients << std::endl;

        if (fork() == 0) {
            signal(SIGINT, SIG_DFL);   // only the parent removes the segment
            close(server_sock);
            handle_client(client_sock);
            shmdt(active_clients);
            exit(0);
        }
        close(client_sock);
    }
    return 0;
}
