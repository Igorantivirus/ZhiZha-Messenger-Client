#pragma once
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

template <typename T>
class ThreadSafeQueue
{
public:
    // Добавить элемент в очередь
    void push(T value)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(std::move(value));
        }
        m_cv.notify_one(); // Уведомить ждущие потоки
    }

    // Попытаться получить элемент (неблокирующий)
    std::optional<T> try_pop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty())
        {
            return std::nullopt;
        }
        T value = std::move(m_queue.front());
        m_queue.pop();
        return value;
    }

    // Получить элемент (блокирующий - ждет пока появится элемент)
    T wait_and_pop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this]
                  {
                      return !m_queue.empty();
                  });
        T value = std::move(m_queue.front());
        m_queue.pop();
        return value;
    }

    // Проверить пуста ли очередь
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    // Получить размер очереди
    size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

private:
    mutable std::mutex m_mutex;
    std::queue<T> m_queue;
    std::condition_variable m_cv;
};

