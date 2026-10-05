#include "threadpool.h"
#include <unistd.h>
#include <iostream>
#include <sstream>
#include "../common/protocol.h"
#include "../common/handler.h"

static uint64_t micros_between(std::chrono::steady_clock::time_point a, std::chrono::steady_clock::time_point b) {
    return (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(b - a).count();
}

ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for (size_t i = 0; i < threads; ++i)
        workers.emplace_back(&ThreadPool::worker, this, i);
}

void ThreadPool::enqueue(int client_socket) {
    size_t length;
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.push({client_socket, std::chrono::steady_clock::now()});
        length = tasks.size();
    }
    condition.notify_one();

    std::ostringstream msg;
    msg << "[pool] queued socket " << client_socket << " | queue length " << length
        << " | active workers " << active.load();
    log_line(msg.str());
}

void ThreadPool::worker(size_t id) {
    while (true) {
        Task task;
        size_t left;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            condition.wait(lock, [this] {
                return stop || !tasks.empty();
            });

            if (stop && tasks.empty())
                return;

            task = tasks.front();
            tasks.pop();
            left = tasks.size();
        }

        auto start = std::chrono::steady_clock::now();
        uint64_t wait_us = micros_between(task.queued_at, start);
        int now_active = ++active;

        std::ostringstream begin_msg;
        begin_msg << "[pool] worker " << id << " took socket " << task.client_socket
                  << " after waiting " << wait_us / 1000.0 << " ms | queue length " << left
                  << " | active workers " << now_active;
        log_line(begin_msg.str());

        handle_client(task.client_socket);
        close(task.client_socket);

        uint64_t service_us = micros_between(start, std::chrono::steady_clock::now());
        --active;
        total_service_us += service_us;
        total_wait_us += wait_us;
        uint64_t count = ++done;

        std::ostringstream end_msg;
        end_msg << "[pool] worker " << id << " finished in " << service_us / 1000.0 << " ms | jobs done "
                << count << " | avg service " << average_service_time() * 1000.0 << " ms"
                << " | avg wait " << average_wait_time() * 1000.0 << " ms";
        log_line(end_msg.str());
    }
}

size_t ThreadPool::queue_length() {
    std::unique_lock<std::mutex> lock(queue_mutex);
    return tasks.size();
}

int ThreadPool::active_workers() const {
    return active.load();
}

size_t ThreadPool::pending() {
    return queue_length() + (size_t)active_workers();
}

uint64_t ThreadPool::completed() const {
    return done.load();
}

double ThreadPool::average_service_time() const {
    uint64_t n = done.load();
    if (n == 0) return 0.0;
    return (total_service_us.load() / (double)n) / 1e6;
}

double ThreadPool::average_wait_time() const {
    uint64_t n = done.load();
    if (n == 0) return 0.0;
    return (total_wait_us.load() / (double)n) / 1e6;
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();

    for (std::thread &worker : workers)
        worker.join();
}
