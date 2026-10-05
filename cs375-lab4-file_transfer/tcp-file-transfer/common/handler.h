#ifndef HANDLER_H
#define HANDLER_H

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include "protocol.h"
#include "sha256.h"

#define UPLOAD_DIR "uploads"

inline std::atomic<uint64_t>& temp_counter() {
    static std::atomic<uint64_t> c{0};
    return c;
}

inline double seconds_since(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

inline std::string fail_reason(const char* what) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) return std::string(what) + " (idle timeout)";
    if (errno == 0) return std::string(what) + " (client disconnected)";
    return std::string(what) + " (" + strerror(errno) + ")";
}

inline void handle_get(int sock, const std::string& ip, const std::string& name) {
    auto start = std::chrono::steady_clock::now();
    std::ifstream file(name, std::ios::binary);
    if (!file) {
        send_u8(sock, ST_NOT_FOUND);
        log_line("[get] " + ip + " asked for '" + name + "' -> not found");
        return;
    }

    file.seekg(0, std::ios::end);
    std::streamoff end = file.tellg();
    file.seekg(0);
    if (end < 0) {
        send_u8(sock, ST_SERVER_ERROR);
        log_line("[get] " + ip + " '" + name + "' -> could not read size");
        return;
    }
    uint64_t file_size = (uint64_t)end;

    if (file_size > MAX_FILE_SIZE) {
        send_u8(sock, ST_TOO_LARGE);
        log_line("[get] " + ip + " '" + name + "' size " + std::to_string(file_size) + " -> too large");
        return;
    }

    log_line("[get] " + ip + " requested '" + name + "' size " + std::to_string(file_size) + " bytes");

    errno = 0;
    if (!send_u8(sock, ST_OK) || !send_u64(sock, file_size)) {
        log_line("[get] " + ip + " " + fail_reason("failed sending header"));
        return;
    }

    SHA256 hasher;
    char buffer[BUFFER_SIZE];
    uint64_t sent = 0;
    while (sent < file_size) {
        size_t want = (size_t)std::min<uint64_t>(BUFFER_SIZE, file_size - sent);
        file.read(buffer, want);
        std::streamsize got = file.gcount();
        if (got <= 0) {
            log_line("[get] " + ip + " file read stopped early");
            return;
        }
        hasher.update(buffer, (size_t)got);
        errno = 0;
        if (send_all(sock, buffer, (size_t)got) < 0) {
            log_line("[get] " + ip + " " + fail_reason("send failed"));
            return;
        }
        sent += (uint64_t)got;
    }

    uint8_t digest[HASH_LEN];
    hasher.finish(digest);
    if (send_all(sock, digest, HASH_LEN) < 0) {
        log_line("[get] " + ip + " " + fail_reason("failed sending checksum"));
        return;
    }

    double secs = seconds_since(start);
    std::ostringstream out;
    out << "[get] " << ip << " done '" << name << "' " << file_size << " bytes in "
        << secs << " s sha256=" << SHA256::to_hex(digest);
    log_line(out.str());
}

inline void handle_put(int sock, const std::string& ip, const std::string& name) {
    auto start = std::chrono::steady_clock::now();
    uint64_t file_size;
    errno = 0;
    if (!recv_u64(sock, file_size)) {
        log_line("[put] " + ip + " " + fail_reason("no size received"));
        return;
    }

    if (file_size > MAX_FILE_SIZE) {
        send_u8(sock, ST_TOO_LARGE);
        log_line("[put] " + ip + " '" + name + "' size " + std::to_string(file_size) + " -> too large");
        return;
    }

    std::error_code ec;
    std::filesystem::create_directories(UPLOAD_DIR, ec);
    std::string temp_path = std::string(UPLOAD_DIR) + "/.part_" + std::to_string(getpid()) + "_" +
                            std::to_string(temp_counter().fetch_add(1));
    std::ofstream out(temp_path, std::ios::binary);
    if (!out) {
        send_u8(sock, ST_SERVER_ERROR);
        log_line("[put] " + ip + " could not open temp file");
        return;
    }

    if (!send_u8(sock, ST_OK)) {
        out.close();
        std::remove(temp_path.c_str());
        return;
    }

    log_line("[put] " + ip + " uploading '" + name + "' size " + std::to_string(file_size) + " bytes");

    SHA256 hasher;
    char buffer[BUFFER_SIZE];
    uint64_t received = 0;
    bool ok = true;
    while (received < file_size) {
        size_t want = (size_t)std::min<uint64_t>(BUFFER_SIZE, file_size - received);
        errno = 0;
        ssize_t n = recv(sock, buffer, want, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) {
            log_line("[put] " + ip + " " + fail_reason("upload stopped early"));
            ok = false;
            break;
        }
        hasher.update(buffer, (size_t)n);
        out.write(buffer, n);
        received += (uint64_t)n;
    }
    out.close();

    uint8_t client_digest[HASH_LEN];
    if (ok) {
        errno = 0;
        if (recv_all(sock, client_digest, HASH_LEN) != HASH_LEN) {
            log_line("[put] " + ip + " " + fail_reason("no checksum received"));
            ok = false;
        }
    }

    if (!ok) {
        std::remove(temp_path.c_str());
        return;
    }

    uint8_t digest[HASH_LEN];
    hasher.finish(digest);
    if (memcmp(digest, client_digest, HASH_LEN) != 0) {
        std::remove(temp_path.c_str());
        send_u8(sock, ST_CHECKSUM_FAIL);
        log_line("[put] " + ip + " '" + name + "' checksum mismatch, file thrown away");
        return;
    }

    std::string final_path = std::string(UPLOAD_DIR) + "/" + name;
    if (std::rename(temp_path.c_str(), final_path.c_str()) != 0) {
        std::remove(temp_path.c_str());
        send_u8(sock, ST_SERVER_ERROR);
        log_error("rename");
        return;
    }

    send_u8(sock, ST_OK);
    double secs = seconds_since(start);
    std::ostringstream msg;
    msg << "[put] " << ip << " saved '" << final_path << "' " << file_size << " bytes in "
        << secs << " s sha256=" << SHA256::to_hex(digest);
    log_line(msg.str());
}

inline void handle_client(int sock) {
    std::string ip = peer_ip(sock);

    if (!set_timeouts(sock, IDLE_TIMEOUT_SEC)) log_error("setsockopt timeout");

    uint8_t cmd;
    uint32_t name_len;
    errno = 0;
    if (!recv_u8(sock, cmd) || !recv_u32(sock, name_len)) {
        log_line("[conn] " + ip + " " + fail_reason("no request"));
        return;
    }

    if (name_len == 0 || name_len > MAX_NAME_LEN) {
        send_u8(sock, ST_BAD_NAME);
        log_line("[conn] " + ip + " bad name length " + std::to_string(name_len));
        return;
    }

    char filename[MAX_NAME_LEN + 1] = {0};
    errno = 0;
    if (recv_all(sock, filename, name_len) != (ssize_t)name_len) {
        log_line("[conn] " + ip + " " + fail_reason("name not received"));
        return;
    }
    std::string name(filename, name_len);

    if (!is_safe_name(name)) {
        send_u8(sock, ST_BAD_NAME);
        log_line("[conn] " + ip + " rejected unsafe name '" + name + "'");
        return;
    }

    if (cmd == CMD_GET) {
        handle_get(sock, ip, name);
    } else if (cmd == CMD_PUT) {
        handle_put(sock, ip, name);
    } else {
        send_u8(sock, ST_BAD_REQUEST);
        log_line("[conn] " + ip + " unknown command");
    }
}

inline int make_listen_socket(int backlog) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        log_error("socket");
        return -1;
    }

    int yes = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        log_error("setsockopt SO_REUSEADDR");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        log_error("bind");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, backlog) < 0) {
        log_error("listen");
        close(server_fd);
        return -1;
    }

    return server_fd;
}

#endif
