#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <ctime>

#define BUFFER_SIZE 5

class ThreadSafeCircularBuffer {
private:
    int buffer[BUFFER_SIZE];
    int in = 0, out = 0, count = 0;
    std::mutex mtx;
    std::condition_variable not_full, not_empty;

public:
    void push(int value) {
        std::unique_lock<std::mutex> lock(mtx);
        not_full.wait(lock, [this] { return count < BUFFER_SIZE; });
        buffer[in] = value;
        in = (in + 1) % BUFFER_SIZE;
        count++;
        lock.unlock();
        not_empty.notify_one();
    }

    bool pop(int& value) {
        std::unique_lock<std::mutex> lock(mtx);
        not_empty.wait(lock, [this] { return count > 0; });
        value = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        count--;
        lock.unlock();
        not_full.notify_one();
        return true;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mtx);
        return count == 0;
    }

    bool full() {
        std::lock_guard<std::mutex> lock(mtx);
        return count == BUFFER_SIZE;
    }
};

void circularBufferTest() {
    ThreadSafeCircularBuffer cb;
    std::vector<std::thread> producers, consumers;
    const int NUM_ITEMS = 10;
    const int NUM_PRODUCERS = 2;
    const int NUM_CONSUMERS = 2;
    std::mutex print_mtx;

    auto producer = [&cb, &print_mtx](int id) {
        for (int i = 0; i < NUM_ITEMS; ++i) {
            int value = rand() % 100;
            cb.push(value);
            {
                std::lock_guard<std::mutex> lock(print_mtx);
                std::cout << "Producer " << id << " produced: " << value << "\n";
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    };

    auto consumer = [&cb, &print_mtx](int id) {
        for (int i = 0; i < NUM_ITEMS; ++i) {
            int value;
            if (cb.pop(value)) {
                std::lock_guard<std::mutex> lock(print_mtx);
                std::cout << "Consumer " << id << " consumed: " << value << "\n";
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
    };

    for (int i = 0; i < NUM_PRODUCERS; ++i) {
        producers.emplace_back(producer, i);
    }
    for (int i = 0; i < NUM_CONSUMERS; ++i) {
        consumers.emplace_back(consumer, i);
    }

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();

    std::cout << "Buffer empty at end: " << (cb.empty() ? "yes" : "no") << "\n";
}

int main() {
    srand(time(0));
    circularBufferTest();
    return 0;
}
