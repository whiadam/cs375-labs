#include <iostream>
#include <queue>
#include <stack>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
#include <atomic>

// Thread-safe Queue class
template <typename T>
class ThreadSafeQueue {
private:
    std::queue<T> queue;
    std::mutex mtx;

public:
    ThreadSafeQueue() {}

    void push(T value) {
        std::lock_guard<std::mutex> lock(mtx);
        queue.push(value);
    }

    bool pop(T& value) {
        std::lock_guard<std::mutex> lock(mtx);
        if (queue.empty()) return false;
        value = queue.front();
        queue.pop();
        return true;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.empty();
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.size();
    }
};

// Thread-safe Stack class
template <typename T>
class ThreadSafeStack {
private:
    std::stack<T> stack;
    std::mutex mtx;

public:
    ThreadSafeStack() {}

    void push(T value) {
        std::lock_guard<std::mutex> lock(mtx);
        stack.push(value);
    }

    bool pop(T& value) {
        std::lock_guard<std::mutex> lock(mtx);
        if (stack.empty()) return false;
        value = stack.top();
        stack.pop();
        return true;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mtx);
        return stack.empty();
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(mtx);
        return stack.size();
    }
};

// Problem 1: Producer-Consumer Simulation
void producerConsumerProblem() {
    ThreadSafeQueue<std::string> messageQueue;
    const int NUM_PRODUCERS = 3;
    const int NUM_CONSUMERS = 2;
    const int MESSAGES_PER_PRODUCER = 5;
    std::atomic<int> messages_produced(0);
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    auto producer = [&messageQueue, &messages_produced](int id) {
        for (int i = 0; i < MESSAGES_PER_PRODUCER; ++i) {
            std::string message = "Producer " + std::to_string(id) + " Message " + std::to_string(i);
            messageQueue.push(message);
            messages_produced++;
            std::cout << "Produced: " << message << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    };

    auto consumer = [&messageQueue, &messages_produced](int id) {
        int consumed = 0;
        while (consumed < NUM_PRODUCERS * MESSAGES_PER_PRODUCER) {
            std::string message;
            if (messageQueue.pop(message)) {
                std::cout << "Consumer " << id << " processed: " << message << std::endl;
                consumed++;
            } else if (messages_produced == NUM_PRODUCERS * MESSAGES_PER_PRODUCER) {
                break; // All messages produced and queue empty
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
}

// Problem 2: Undo-Redo System
void undoRedoProblem() {
    ThreadSafeStack<std::string> undoStack;
    ThreadSafeStack<std::string> redoStack;
    std::string currentText = "";

    auto editText = [&undoStack, &redoStack, &currentText](const std::string& newText) {
        undoStack.push(currentText);
        while (!redoStack.empty()) {
            std::string temp;
            redoStack.pop(temp);
        }
        currentText = newText;
        std::cout << "Text updated to: " << currentText << std::endl;
    };

    auto undo = [&undoStack, &redoStack, &currentText]() {
        std::string prevText;
        if (undoStack.pop(prevText)) {
            redoStack.push(currentText);
            currentText = prevText;
            std::cout << "Undo - Current text: " << currentText << std::endl;
        }
    };

    auto redo = [&undoStack, &redoStack, &currentText]() {
        std::string nextText;
        if (redoStack.pop(nextText)) {
            undoStack.push(currentText);
            currentText = nextText;
            std::cout << "Redo - Current text: " << currentText << std::endl;
        }
    };

    editText("Hello");
    editText("Hello World");
    editText("Hello Universe");
    undo();
    undo();
    redo();
    editText("Hello Galaxy");
}

int main() {
    std::cout << "Problem 1: Producer-Consumer Simulation\n";
    producerConsumerProblem();

    std::cout << "\nProblem 2: Undo-Redo System\n";
    undoRedoProblem();
    return 0;
}
