#ifndef LOCKFREECIRCULARQUEUE_H
#define LOCKFREECIRCULARQUEUE_H

#include <atomic>
#include <stdexcept>
#include <new>

template <typename T, size_t CAPACITY>
class LockFreeCircularQueue {
private:
    alignas(T) char buffer[CAPACITY * sizeof(T)]; // Статическая аллокация памяти
    std::atomic<size_t> readIndex{ 0 };   // Индекс чтения (атомарный)
    std::atomic<size_t> writeIndex{ 0 };  // Индекс записи (атомарный)

     // Возвращает указатель на объект в буфере
    T* getBufferSlot(size_t index) noexcept {
        return reinterpret_cast<T*>(&buffer[index * sizeof(T)]);
    }

public:
    LockFreeCircularQueue() noexcept = default;

    // Конструктор копирования
    LockFreeCircularQueue(const LockFreeCircularQueue& other) noexcept {
        size_t read = other.readIndex.load(std::memory_order_acquire);
        size_t write = other.writeIndex.load(std::memory_order_acquire);
        for (size_t i = read; i != write; i = (i + 1) % CAPACITY) {
            new (getBufferSlot(i)) T(*other.getBufferSlot(i));
        }
        readIndex.store(read, std::memory_order_release);
        writeIndex.store(write, std::memory_order_release);
    }

    // Оператор присваивания
    LockFreeCircularQueue& operator=(const LockFreeCircularQueue& other) noexcept {
        if (this == &other) return *this;

        // Уничтожить текущие элементы
        clear();

        size_t read = other.readIndex.load(std::memory_order_acquire);
        size_t write = other.writeIndex.load(std::memory_order_acquire);
        for (size_t i = read; i != write; i = (i + 1) % CAPACITY) {
            new (getBufferSlot(i)) T(*other.getBufferSlot(i));
        }
        readIndex.store(read, std::memory_order_release);
        writeIndex.store(write, std::memory_order_release);
        return *this;
    }

    ~LockFreeCircularQueue() noexcept {
        clear();
    }

    // Добавление элемента в очередь
    bool enqueue(const T& item) noexcept {
        size_t currentWrite = writeIndex.load(std::memory_order_relaxed);
        size_t nextWrite = (currentWrite + 1) % CAPACITY;

        // Если очередь полна, пропускаем запись
        if (nextWrite == readIndex.load(std::memory_order_acquire)) {
            return false;
        }

        // Размещение объекта в буфере
        new (getBufferSlot(currentWrite)) T(item);
        writeIndex.store(nextWrite, std::memory_order_release);
        return true;
    }

    // Извлечение элемента из очереди
    bool dequeue(T& item) noexcept {
        size_t currentRead = readIndex.load(std::memory_order_relaxed);

        // Если очередь пуста
        if (currentRead == writeIndex.load(std::memory_order_acquire)) {
            return false;
        }

        // Копирование объекта
        T* slot = getBufferSlot(currentRead);
        item = std::move(*slot);
        slot->~T(); // Уничтожение объекта

        readIndex.store((currentRead + 1) % CAPACITY, std::memory_order_release);

        return true;
    }

    // Очистка очереди
    void clear() noexcept {
        size_t currentRead = readIndex.load(std::memory_order_relaxed);
        size_t currentWrite = writeIndex.load(std::memory_order_relaxed);
        while (currentRead != currentWrite) {
            getBufferSlot(currentRead)->~T();
            currentRead = (currentRead + 1) % CAPACITY;
        }
        readIndex.store(0, std::memory_order_release);
        writeIndex.store(0, std::memory_order_release);
    }
};

#endif // LOCKFREECIRCULARQUEUE_H