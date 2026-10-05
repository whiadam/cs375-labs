#include <iostream>
#include <fstream>
#include <chrono>
#include <cstdio>
#include <csignal>
#include <algorithm>
#include <arpa/inet.h>
#include <unistd.h>
#include "../common/protocol.h"
#include "../common/sha256.h"

static double seconds_since(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

static void print_result(const std::string& what, uint64_t bytes, double secs) {
    double mb = bytes / (1024.0 * 1024.0);
    double rate = secs > 0 ? mb / secs : 0;
    printf("%s %llu bytes in %.4f s (%.2f MB/s)\n", what.c_str(), (unsigned long long)bytes, secs, rate);
}

static int connect_to_server(const char* host) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    if (inet_pton(AF_INET, host, &server.sin_addr) != 1) {
        std::cerr << "Bad server address: " << host << "\n";
        close(sock);
        return -1;
    }

    if (connect(sock, (sockaddr*)&server, sizeof(server)) < 0) {
        perror("connect");
        close(sock);
        return -1;
    }

    uint8_t greeting;
    if (!recv_u8(sock, greeting)) {
        std::cerr << "Server closed the connection before saying hello.\n";
        close(sock);
        return -1;
    }
    if (greeting == GREET_BUSY) {
        std::cerr << "Server is busy (too many connections). Try again later.\n";
        close(sock);
        return -2;
    }
    return sock;
}

static bool send_request(int sock, uint8_t cmd, const std::string& name) {
    return send_u8(sock, cmd) && send_u32(sock, (uint32_t)name.size()) &&
           send_all(sock, name.c_str(), name.size()) == (ssize_t)name.size();
}

static int do_get(int sock, const std::string& filename) {
    auto start = std::chrono::steady_clock::now();

    if (!send_request(sock, CMD_GET, filename)) {
        perror("send request");
        return 1;
    }

    uint8_t status;
    if (!recv_u8(sock, status)) {
        std::cerr << "No reply from server.\n";
        return 1;
    }
    if (status != ST_OK) {
        std::cerr << "Server said: " << status_text(status) << "\n";
        return 1;
    }

    uint64_t file_size;
    if (!recv_u64(sock, file_size)) {
        std::cerr << "Did not get file size.\n";
        return 1;
    }

    std::string out_name = "received_" + filename;
    std::string temp_name = out_name + ".part" + std::to_string(getpid());
    std::ofstream output(temp_name, std::ios::binary);
    if (!output) {
        std::cerr << "Could not open " << temp_name << " for writing.\n";
        return 1;
    }

    SHA256 hasher;
    char buffer[BUFFER_SIZE];
    uint64_t received = 0;
    while (received < file_size) {
        size_t want = (size_t)std::min<uint64_t>(BUFFER_SIZE, file_size - received);
        ssize_t bytes = recv(sock, buffer, want, 0);
        if (bytes < 0 && errno == EINTR) continue;
        if (bytes <= 0) {
            std::cerr << "Connection lost after " << received << " of " << file_size << " bytes.\n";
            output.close();
            std::remove(temp_name.c_str());
            return 1;
        }
        hasher.update(buffer, (size_t)bytes);
        output.write(buffer, bytes);
        received += (uint64_t)bytes;
    }
    output.close();

    uint8_t server_digest[HASH_LEN];
    if (recv_all(sock, server_digest, HASH_LEN) != HASH_LEN) {
        std::cerr << "Did not get checksum.\n";
        std::remove(temp_name.c_str());
        return 1;
    }

    uint8_t digest[HASH_LEN];
    hasher.finish(digest);
    if (memcmp(digest, server_digest, HASH_LEN) != 0) {
        std::cerr << "Checksum mismatch! File is corrupted and was deleted.\n";
        std::remove(temp_name.c_str());
        return 1;
    }

    if (std::rename(temp_name.c_str(), out_name.c_str()) != 0) {
        perror("rename");
        return 1;
    }

    print_result("Received " + out_name + ":", file_size, seconds_since(start));
    std::cout << "SHA-256 verified: " << SHA256::to_hex(digest) << "\n";
    return 0;
}

static int do_put(int sock, const std::string& path) {
    auto start = std::chrono::steady_clock::now();

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Cannot open local file " << path << "\n";
        return 1;
    }
    file.seekg(0, std::ios::end);
    uint64_t file_size = (uint64_t)file.tellg();
    file.seekg(0);

    std::string name = path.substr(path.find_last_of('/') + 1);

    if (!send_request(sock, CMD_PUT, name) || !send_u64(sock, file_size)) {
        perror("send request");
        return 1;
    }

    uint8_t status;
    if (!recv_u8(sock, status)) {
        std::cerr << "No reply from server.\n";
        return 1;
    }
    if (status != ST_OK) {
        std::cerr << "Server said: " << status_text(status) << "\n";
        return 1;
    }

    SHA256 hasher;
    char buffer[BUFFER_SIZE];
    uint64_t sent = 0;
    while (sent < file_size) {
        size_t want = (size_t)std::min<uint64_t>(BUFFER_SIZE, file_size - sent);
        file.read(buffer, want);
        std::streamsize got = file.gcount();
        if (got <= 0) {
            std::cerr << "Local file read stopped early.\n";
            return 1;
        }
        hasher.update(buffer, (size_t)got);
        if (send_all(sock, buffer, (size_t)got) < 0) {
            perror("send");
            return 1;
        }
        sent += (uint64_t)got;
    }

    uint8_t digest[HASH_LEN];
    hasher.finish(digest);
    if (send_all(sock, digest, HASH_LEN) < 0) {
        perror("send checksum");
        return 1;
    }

    if (!recv_u8(sock, status)) {
        std::cerr << "No final reply from server.\n";
        return 1;
    }
    if (status != ST_OK) {
        std::cerr << "Upload failed: " << status_text(status) << "\n";
        return 1;
    }

    print_result("Uploaded " + name + ":", file_size, seconds_since(start));
    std::cout << "SHA-256 verified by server: " << SHA256::to_hex(digest) << "\n";
    return 0;
}

int main(int argc, char* argv[]) {
    signal(SIGPIPE, SIG_IGN);

    std::string mode = "get";
    std::string filename;
    const char* host = "127.0.0.1";

    if (argc == 2) {
        filename = argv[1];
    } else if (argc == 3 || argc == 4) {
        mode = argv[1];
        filename = argv[2];
        if (argc == 4) host = argv[3];
    } else {
        std::cout << "Usage: ./client <filename>\n"
                  << "       ./client get <filename> [server_ip]\n"
                  << "       ./client put <filename> [server_ip]\n";
        return 1;
    }

    if (mode != "get" && mode != "put") {
        std::cerr << "Mode must be get or put\n";
        return 1;
    }

    auto total_start = std::chrono::steady_clock::now();
    int sock = connect_to_server(host);
    if (sock < 0) return sock == -2 ? 2 : 1;

    int result = (mode == "get") ? do_get(sock, filename) : do_put(sock, filename);
    close(sock);
    printf("Total time including connect and wait: %.4f s\n", seconds_since(total_start));
    return result;
}
