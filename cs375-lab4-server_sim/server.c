
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT   8080
#define BUFSZ  1024

static volatile sig_atomic_t stop_server = 0;


static void reap_children(int sig) {
    (void)sig;
    int saved_errno = errno;              
    while (waitpid(-1, NULL, WNOHANG) > 0) 
        ;                                 
    errno = saved_errno;                   

static void handle_shutdown(int sig) {
    (void)sig;
    stop_server = 1;
}

/* Runs in the child process, one per client. */
static void handle_client(int client_sock) {
    char buffer[BUFSZ];
    ssize_t n = read(client_sock, buffer, sizeof(buffer) - 1); 
    if (n < 0) {
        perror("read");
        close(client_sock);
        return;
    }
    buffer[n] = '\0';                    
    printf("Received (%zd bytes): %s\n", n, buffer);
    fflush(stdout);


    if (strncmp(buffer, "shutdown", 8) == 0) {
        const char *msg = "Server shutting down\n";
        write(client_sock, msg, strlen(msg));
        close(client_sock);
        kill(getppid(), SIGTERM);        
        _exit(0);
    }

    const char *reply = "Hello from server";
    write(client_sock, reply, strlen(reply));
    close(client_sock);
}

int main(void) {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

   
    struct sigaction sa_chld = {0};
    sa_chld.sa_handler = reap_children;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa_chld, NULL);

  
    struct sigaction sa_term = {0};
    sa_term.sa_handler = handle_shutdown;
    sigemptyset(&sa_term.sa_mask);
    sa_term.sa_flags = 0;                  /* no SA_RESTART: break accept */
    sigaction(SIGTERM, &sa_term, NULL);
    sigaction(SIGINT,  &sa_term, NULL);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) { perror("socket"); return 1; }

    
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        perror("setsockopt");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        return 1;
    }
    if (listen(server_sock, 5) < 0) { perror("listen"); return 1; }

    printf("Server listening on 127.0.0.1:%d  (send \"shutdown\" to stop)\n", PORT);
    fflush(stdout);

    while (!stop_server) {
        addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
        if (client_sock < 0) {
            if (errno == EINTR) continue;  /* interrupted by a signal     */
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
    printf("Server stopped cleanly.\n");
    return 0;
}
