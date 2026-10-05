#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>
#include <cstring>
#include <cerrno>
#include <string>
#include <iostream>
#include <mutex>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 9000
#define BUFFER_SIZE 4096
#define MAX_NAME_LEN 255
#define MAX_FILE_SIZE (1ULL << 30)
#define IDLE_TIMEOUT_SEC 10
#define HASH_LEN 32

enum Greeting : uint8_t {
    GREET_READY = 0,
    GREET_BUSY = 1
};

enum Command : uint8_t {
    CMD_GET = 'G',
    CMD_PUT = 'P'
};

enum Status : uint8_t {
    ST_OK = 0,
    ST_NOT_FOUND = 1,
    ST_TOO_LARGE = 2,
    ST_BAD_NAME = 3,
    ST_CHECKSUM_FAIL = 4,
    ST_BAD_REQUEST = 5,
    ST_SERVER_ERROR = 6
};

inline const char* status_text(uint8_t s) {
    switch (s) {
        case ST_OK: return "ok";
        case ST_NOT_FOUND: return "file not found";
        case ST_TOO_LARGE: return "file too large";
        case ST_BAD_NAME: return "bad file name";
        case ST_CHECKSUM_FAIL: return "checksum mismatch";
        case ST_BAD_REQUEST: return "bad request";
        case ST_SERVER_ERROR: return "server error";
        default: return "unknown status";
    }
}

inline std::mutex& log_mutex() {
    static std::mutex m;
    return m;
}

inline void log_line(const std::string& msg) {
    std::lock_guard<std::mutex> lock(log_mutex());
    std::cout << msg << std::endl;
}

inline void log_error(const std::string& where) {
    std::lock_guard<std::mutex> lock(log_mutex());
    std::cerr << "[error] " << where << ": " << strerror(errno) << std::endl;
}

inline ssize_t send_all(int sock, const void* buffer, size_t length) {
    const char* p = static_cast<const char*>(buffer);
    size_t sent = 0;
    while (sent < length) {
        ssize_t n = send(sock, p + sent, length - sent, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        sent += (size_t)n;
    }
    return (ssize_t)sent;
}

inline ssize_t recv_all(int sock, void* buffer, size_t length) {
    char* p = static_cast<char*>(buffer);
    size_t got = 0;
    while (got < length) {
        ssize_t n = recv(sock, p + got, length - got, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return 0;
        got += (size_t)n;
    }
    return (ssize_t)got;
}

inline bool send_u8(int sock, uint8_t v) {
    return send_all(sock, &v, 1) == 1;
}

inline bool recv_u8(int sock, uint8_t& v) {
    return recv_all(sock, &v, 1) == 1;
}

inline bool send_u32(int sock, uint32_t v) {
    uint32_t n = htonl(v);
    return send_all(sock, &n, sizeof(n)) == (ssize_t)sizeof(n);
}

inline bool recv_u32(int sock, uint32_t& v) {
    uint32_t n;
    if (recv_all(sock, &n, sizeof(n)) != (ssize_t)sizeof(n)) return false;
    v = ntohl(n);
    return true;
}

inline bool send_u64(int sock, uint64_t v) {
    uint8_t b[8];
    for (int i = 0; i < 8; i++) b[i] = (uint8_t)(v >> (56 - 8 * i));
    return send_all(sock, b, 8) == 8;
}

inline bool recv_u64(int sock, uint64_t& v) {
    uint8_t b[8];
    if (recv_all(sock, b, 8) != 8) return false;
    v = 0;
    for (int i = 0; i < 8; i++) v = (v << 8) | b[i];
    return true;
}

inline bool set_timeouts(int sock, int seconds) {
    timeval tv{};
    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) return false;
    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) return false;
    return true;
}

inline bool is_safe_name(const std::string& name) {
    if (name.empty() || name.size() > MAX_NAME_LEN) return false;
    if (name[0] == '.') return false;
    for (char c : name) {
        if (c == '/' || c == '\\' || c == '\0') return false;
    }
    return true;
}

inline std::string peer_ip(int sock) {
    sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    if (getpeername(sock, (sockaddr*)&addr, &len) < 0) return "unknown";
    char buf[INET_ADDRSTRLEN];
    if (!inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf))) return "unknown";
    return std::string(buf) + ":" + std::to_string(ntohs(addr.sin_port));
}

#endif
