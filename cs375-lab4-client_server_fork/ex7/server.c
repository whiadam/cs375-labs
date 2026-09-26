// Exercise 7: Fork with Broadcast (server)
//
//The parent server accepts new clients and keeps track of all the connected client sockets. 
//When a client connects, the server creates a child process for that client.

//Each child listens for messages from its own client. 
//When it gets a message, it sends that message to the parent through a pipe. 
//The parent then sends the message to all the other clients, except for the person who sent it.

//The pipe is used because each child only has access to the sockets that existed when it was created. 
//This means a child cannot easily communicate with clients that connect later. 
//The parent has all of the client sockets, so it handles sending the messages to everyone.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 8080
#define MAX_CLIENTS 32
#define TEXT_SIZE 1000

enum { MSG_TEXT, MSG_LEAVE };

/* Fixed-size record, smaller than PIPE_BUF (4096), so every write() to the
   pipe is atomic: records from different children never interleave. */
struct pipe_msg {
    int type;
    int sender_fd;
    int sender_id;
    char text[TEXT_SIZE];
};

int clients[MAX_CLIENTS];      /* socket fds, -1 = free slot */
int client_ids[MAX_CLIENTS];

void child_loop(int client_sock, int id, int pipe_wr) {
    char buf[TEXT_SIZE];
    struct pipe_msg m;

    while (1) {
        memset(buf, 0, sizeof(buf));
        ssize_t n = read(client_sock, buf, sizeof(buf) - 1);
        if (n <= 0) break;
        buf[strcspn(buf, "\r\n")] = '\0';
        if (strcmp(buf, "exit") == 0) break;
        if (buf[0] == '\0') continue;

        memset(&m, 0, sizeof(m));
        m.type = MSG_TEXT;
        m.sender_fd = client_sock;
        m.sender_id = id;
        strncpy(m.text, buf, TEXT_SIZE - 1);
        write(pipe_wr, &m, sizeof(m));
    }

    /* tell the parent this client is gone */
    memset(&m, 0, sizeof(m));
    m.type = MSG_LEAVE;
    m.sender_fd = client_sock;
    m.sender_id = id;
    write(pipe_wr, &m, sizeof(m));
    close(client_sock);
}

void broadcast(int sender_fd, const char *text) {
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i] != -1 && clients[i] != sender_fd)
            write(clients[i], text, strlen(text));
}

int main() {
    /* line-buffer stdout so fork() doesn't duplicate unflushed output */
    setvbuf(stdout, NULL, _IOLBF, 0);

    int server_sock, opt = 1, pipefd[2], next_id = 1;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    signal(SIGCHLD, SIG_IGN);   /* auto-reap children */
    signal(SIGPIPE, SIG_IGN);   /* writing to a closed client must not kill us */

    for (int i = 0; i < MAX_CLIENTS; i++) clients[i] = -1;
    if (pipe(pipefd) < 0) { perror("pipe"); exit(1); }

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind"); exit(1);
    }
    listen(server_sock, 5);
    printf("Broadcast server listening on port %d...\n", PORT);

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_sock, &readfds);
        FD_SET(pipefd[0], &readfds);
        int maxfd = server_sock > pipefd[0] ? server_sock : pipefd[0];

        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) continue;

        /* ---- new connection ---- */
        if (FD_ISSET(server_sock, &readfds)) {
            addr_size = sizeof(client_addr);
            int cs = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
            if (cs >= 0) {
                int slot = -1;
                for (int i = 0; i < MAX_CLIENTS; i++)
                    if (clients[i] == -1) { slot = i; break; }

                if (slot == -1) {
                    const char *full = "Server full, try later\n";
                    write(cs, full, strlen(full));
                    close(cs);
                } else {
                    int id = next_id++;
                    pid_t pid = fork();
                    if (pid == 0) {
                        /* child: close everything it doesn't own */
                        close(server_sock);
                        close(pipefd[0]);
                        for (int i = 0; i < MAX_CLIENTS; i++)
                            if (clients[i] != -1) close(clients[i]);
                        child_loop(cs, id, pipefd[1]);
                        exit(0);
                    } else if (pid > 0) {
                        clients[slot] = cs;       /* parent keeps the socket */
                        client_ids[slot] = id;
                        printf("Client %d connected (pid %d)\n", id, pid);
                        char note[64];
                        snprintf(note, sizeof(note), "[Server] Client %d joined\n", id);
                        broadcast(cs, note);
                        snprintf(note, sizeof(note), "[Server] Welcome, you are Client %d\n", id);
                        write(cs, note, strlen(note));
                    } else {
                        perror("fork");
                        close(cs);
                    }
                }
            }
        }

        /* ---- message from a child ---- */
        if (FD_ISSET(pipefd[0], &readfds)) {
            struct pipe_msg m;
            if (read(pipefd[0], &m, sizeof(m)) == sizeof(m)) {
                char out[TEXT_SIZE + 64];
                if (m.type == MSG_TEXT) {
                    printf("Client %d: %s\n", m.sender_id, m.text);
                    snprintf(out, sizeof(out), "Client %d: %s\n", m.sender_id, m.text);
                    broadcast(m.sender_fd, out);
                } else {  /* MSG_LEAVE */
                    for (int i = 0; i < MAX_CLIENTS; i++)
                        if (clients[i] == m.sender_fd) {
                            close(clients[i]);
                            clients[i] = -1;
                        }
                    printf("Client %d disconnected\n", m.sender_id);
                    snprintf(out, sizeof(out), "[Server] Client %d left\n", m.sender_id);
                    broadcast(-1, out);
                }
            }
        }
    }
    return 0;
}
