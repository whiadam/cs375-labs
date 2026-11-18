#include <iostream>
#include <queue>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <ctime>

template <typename T>
class ThreadSafePriorityQueue {
private:
    std::priority_queue<T> pq;
    std::mutex mtx;

public:
    void push(T value) {
        std::lock_guard<std::mutex> lock(mtx);
        pq.push(value);
    }

    bool pop(T& value) {
        std::lock_guard<std::mutex> lock(mtx);
        if (pq.empty()) return false;
        value = pq.top();
        pq.pop();
        return true;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mtx);
        return pq.empty();
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(mtx);
        return pq.size();
    }
};

void priorityQueueTest() {
    ThreadSafePriorityQueue<int> pq;
    std::vector<std::thread> threads;
    const int NUM_THREADS = 4;

    auto pusher = [&pq](int id) {
        for (int i = 0; i < 5; ++i) {
            int priority = rand() % 100;
            pq.push(priority);
            std::cout << "Thread " << id << " pushed: " << priority << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    };

    auto popper = [&pq]() {
        for (int i = 0; i < 20; ++i) {
            int value;
            if (pq.pop(value)) {
                std::cout << "Popped: " << value << "\n";
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
    };

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(pusher, i);
    }
    threads.emplace_back(popper);

    for (auto& t : threads) t.join();

    std::cout << "Items left in queue: " << pq.size() << "\n";
}

int main() {
    srand(time(0));
    priorityQueueTest();
    return 0;
}
