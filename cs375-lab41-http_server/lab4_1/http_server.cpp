// ... (includes and thread-pool setup from Part 3)
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
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

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

int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::string url_decode(const std::string& in) {
    std::string out;
    for (size_t i = 0; i < in.length(); i++) {
        if (in[i] == '%' && i + 2 < in.length() && hex_value(in[i + 1]) >= 0 && hex_value(in[i + 2]) >= 0) {
            out += (char)(hex_value(in[i + 1]) * 16 + hex_value(in[i + 2]));
            i += 2;
        } else if (in[i] == '+') {
            out += ' ';
        } else {
            out += in[i];
        }
    }
    return out;
}

std::string parse_request(int client_fd) {
    char buffer[4096];
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) return "";

    std::string request(buffer, bytes_read);
    size_t pos = request.find('\n');
    if (pos == std::string::npos) return "";
    std::string first_line = request.substr(0, pos);

    // TODO: Improve parsing: Handle full request (headers), support POST, decode URL (%20 -> space)
    if (!first_line.empty() && first_line.back() == '\r') first_line.pop_back();

    size_t header_end = request.find("\r\n\r\n");
    std::string headers = request.substr(0, header_end == std::string::npos ? request.length() : header_end);
    log_msg("Full request:\n" + headers);

    size_t method_end = first_line.find(' ', 0);
    if (method_end == std::string::npos || first_line.substr(0, 3) != "GET") {
        return "400 Bad Request";
    }
    size_t path_end = first_line.find(' ', method_end + 1);
    if (path_end == std::string::npos) return "400 Bad Request";
    std::string path = first_line.substr(method_end + 1, path_end - method_end - 1);

    size_t q = path.find('?');
    if (q != std::string::npos) {
        std::string query = path.substr(q + 1);
        path = path.substr(0, q);
        log_msg("Query string ignored: " + query);
    }

    path = url_decode(path);
    if (path.empty() || path == "/") path = "/index.html";

    return path;
}

std::string get_content_type(const std::string& path) {
    std::string ext = fs::path(path).extension().string();
    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif") return "image/gif";
    if (ext == ".ico") return "image/x-icon";
    if (ext == ".txt") return "text/plain";
    return "application/octet-stream";
}

// TODO: Student Implementation
std::string get_file_content(const std::string& path) {
    // Resolve URL to filesystem path, e.g., "/index.html" -> "./www/index.html"
    // Check for security: Reject paths with "../" or absolute paths to prevent traversal
    // Use std::filesystem (C++17) to check exists
    // Map extensions to Content-Type (e.g., .html -> text/html, .png -> image/png)
    // Read file content as string (binary-safe for images)
    // Return empty string on 404 or invalid

    if (path.find("..") != std::string::npos) {
        return "";  // Basic security check - enhance this
    }
    if (path.empty() || path[0] != '/' || path.find('\0') != std::string::npos) {
        return "";
    }

    std::string fs_path = "./www" + path;
    // TODO: Full implementation here
    std::error_code ec;
    fs::path root = fs::canonical("./www", ec);
    if (ec) return "";
    fs::path full = fs::weakly_canonical(fs_path, ec);
    if (ec) return "";

    std::string root_str = root.string() + "/";
    std::string full_str = full.string();
    if (full_str.compare(0, root_str.length(), root_str) != 0) {
        log_msg("Blocked path outside www: " + full_str);
        return "";
    }
    if (!fs::exists(full, ec) || !fs::is_regular_file(full, ec)) {
        return "";
    }

    std::ifstream file(full, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void write_all(int client_fd, const std::string& data) {
    size_t sent = 0;
    while (sent < data.length()) {
        ssize_t n = write(client_fd, data.c_str() + sent, data.length() - sent);
        if (n <= 0) {
            perror("write");
            return;
        }
        sent += n;
    }
}

void send_response(int client_fd, const std::string& content, const std::string& content_type = "text/html") {
    std::string status = content.empty() ? "404 Not Found" : "200 OK";
    std::string response = "HTTP/1.1 " + status + "\r\n";
    response += "Content-Type: " + content_type + "\r\n";
    if (!content.empty()) {
        response += "Content-Length: " + std::to_string(content.length()) + "\r\n";
    }
    response += "Connection: close\r\n\r\n";
    response += content;

    write_all(client_fd, response);
    log_msg("Response: " + status + " (" + content_type + ", " + std::to_string(content.length()) + " bytes)");
}

void send_bad_request(int client_fd) {
    std::string body = "<h1>400 Bad Request</h1>";
    std::string response = "HTTP/1.1 400 Bad Request\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + std::to_string(body.length()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += body;
    write_all(client_fd, response);
    log_msg("Response: 400 Bad Request");
}

void worker_thread() {
    while (true) {
        int client_fd;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            cv.wait(lock, [&] { return !task_queue.empty() || stop_pool; });
            if (stop_pool && task_queue.empty()) return;
            client_fd = task_queue.front();
            task_queue.pop();
        }

        // In worker_thread, after dequeuing client_fd:
        std::string path = parse_request(client_fd);
        std::string content_type = "text/html";  // Default
        // TODO: Set content_type based on path extension (e.g., .css -> text/css, .png -> image/png)

        if (path.empty()) {
            close(client_fd);
            continue;
        }

        if (path == "400 Bad Request") {
            send_bad_request(client_fd);
        } else {
            std::string content = get_file_content(path);
            // TODO: Determine content_type from path
            if (!content.empty()) content_type = get_content_type(path);
            send_response(client_fd, content, content_type);
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

    if (!fs::is_directory("./www")) {
        std::cerr << "Missing ./www directory" << std::endl;
        return 1;
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
        workers.emplace_back(worker_thread);
    }

    pthread_sigmask(SIG_SETMASK, &old_mask, nullptr);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);

    std::cout << "HTTP server listening on port " << port << ", serving ./www" << std::endl;

    while (!got_sigint) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        log_msg(std::string("Connection from ") + inet_ntoa(client_addr.sin_addr));

        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            task_queue.push(client_fd);
        }
        cv.notify_one();
    }

    log_msg("Shutting down...");
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        stop_pool = true;
    }
    cv.notify_all();
    for (auto& w : workers) w.join();
    close(server_fd);
    return 0;
}
