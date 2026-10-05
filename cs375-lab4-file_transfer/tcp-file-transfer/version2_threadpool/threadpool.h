#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <cstdint>

class ThreadPool {
public:
    ThreadPool(size_t threads);
    ~ThreadPool();

    void enqueue(int client_socket);

    size_t queue_length();
    int active_workers() const;
    size_t pending();
    uint64_t completed() const;
    double average_service_time() const;
    double average_wait_time() const;

private:
    struct Task {
        int client_socket;
        std::chrono::steady_clock::time_point queued_at;
    };

    void worker(size_t id);

    std::vector<std::thread> workers;
    std::queue<Task> tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;

    std::atomic<int> active{0};
    std::atomic<uint64_t> done{0};
    std::atomic<uint64_t> total_service_us{0};
    std::atomic<uint64_t> total_wait_us{0};
};

#endif
