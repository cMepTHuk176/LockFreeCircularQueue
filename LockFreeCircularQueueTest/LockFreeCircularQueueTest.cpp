#include "LockFreeCircularQueue.h"
#include <cassert>
#include <iostream>
#include <thread>

void test_enqueue_dequeue() {
    LockFreeCircularQueue<int, 5> queue;

    // Тест: добавление элементов
    assert(queue.enqueue(1));
    assert(queue.enqueue(2));
    assert(queue.enqueue(3));

    int value;
    assert(queue.dequeue(value) && value == 1);
    assert(queue.dequeue(value) && value == 2);
    assert(queue.dequeue(value) && value == 3);

    // Тест: очередь пуста
    assert(!queue.dequeue(value));
}

void test_multithreaded() {
    LockFreeCircularQueue<int, 10> queue;

    std::thread producer([&queue]() {
        for (int i = 0; i < 20; ++i) {
            while (!queue.enqueue(i));
        }
        });

    std::thread consumer([&queue]() {
        for (int i = 0; i < 20; ++i) {
            int value;
            while (!queue.dequeue(value));
            std::cout << "Consumed: " << value << std::endl;
        }
        });

    producer.join();
    consumer.join();
}

int main() {
    test_enqueue_dequeue();
    std::cout << "Single-threaded tests passed." << std::endl;

    test_multithreaded();
    std::cout << "Multithreaded tests passed." << std::endl;

    return 0;
}