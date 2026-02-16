#pragma once

#include <iostream>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <thread>
#include <utility>

#include <network/WebSocketClient.hpp>

#include "ThreadSafeQueue.hpp"

class NetworkSubsystem
{
public:
    using IoContext = boost::asio::io_context;
    using WorkGuard = boost::asio::executor_work_guard<IoContext::executor_type>;

    NetworkSubsystem()
    {
        startWebSocket();
    }
    ~NetworkSubsystem()
    {
        stop();
    }

    NetworkSubsystem(const NetworkSubsystem &) = delete;
    NetworkSubsystem &operator=(const NetworkSubsystem &) = delete;

    // Запуск сетевого потока
    void start()
    {
        bool expected = false;
        if (!isWorking_.compare_exchange_strong(expected, true))
            return; // уже запущено

        // Чтобы run() не завершался, когда нет задач/операций
        if (!workGuard_)
            workGuard_.emplace(boost::asio::make_work_guard(io_));

        // Если ранее был stop(), то run() будет сразу возвращаться — нужен restart()
        io_.restart();

        netThread_ = std::thread(std::bind(&NetworkSubsystem::netThread, this));
    }

    // Остановка и join потока
    void stop()
    {
        bool expected = true;
        if (!isWorking_.compare_exchange_strong(expected, false))
        {
            return; // уже остановлено
        }

        // Разрешаем io_context "закончить работу"
        if (workGuard_)
        {
            workGuard_->reset();
        }

        // Сигналим run() вернуться как можно скорее
        io_.stop(); // :contentReference[oaicite:2]{index=2}

        if (netThread_.joinable())
        {
            netThread_.join();
        }

        client_.reset();
    }

    // Внешнее управление (как ты и хотел)
    void setWorking(bool v)
    {
        v ? start() : stop();
    }
    bool isWorking() const noexcept
    {
        return isWorking_.load(std::memory_order_acquire);
    }

    // Доступ к io_context (если нужно)
    IoContext &io() noexcept
    {
        return io_;
    }
    const IoContext &io() const noexcept
    {
        return io_;
    }

    // Клиентом владеет подсистема (shared_ptr — ок)
    void setClient(std::shared_ptr<ws::WebSocketClient> c)
    {
        client_ = std::move(c);
    }
    std::shared_ptr<ws::WebSocketClient> getClient() const
    {
        return client_;
    }

    // Удобный способ выполнить код в io-потоке (thread-safe через очередь io_context)
    // io_context можно безопасно использовать конкурентно, например post() из другого потока. :contentReference[oaicite:3]{index=3}
    template <class F>
    void post(F &&f)
    {
        boost::asio::post(io_, std::forward<F>(f));
    }

    // Опционально: обёртка под отправку, чтобы сцены НЕ трогали client напрямую
    // (реальную реализацию sendText ты уже сделал)
    void sendText(std::string msg);

private:
    IoContext io_{};
    std::optional<WorkGuard> workGuard_;

    std::thread netThread_;
    std::atomic_bool isWorking_{false};

    std::shared_ptr<ws::WebSocketClient> client_;
    ThreadSafeQueue<int> queue_;

private:
    void startWebSocket()
    {
        client_ = ws::WebSocketClient::create(io_);
        client_->events().onOpen(std::bind(&NetworkSubsystem::onOpen, this));
        client_->events().onText(std::bind(&NetworkSubsystem::onText, this, std::placeholders::_1));
        client_->events().onBytes(std::bind(&NetworkSubsystem::onByte, this, std::placeholders::_1));
        client_->events().onClose(std::bind(&NetworkSubsystem::onClose, this, std::placeholders::_1));
        client_->events().onError(std::bind(&NetworkSubsystem::onError, this, std::placeholders::_1, std::placeholders::_2));
    }

private: // net Thread
    void netThread()
    {
        while (isWorking_.load(std::memory_order_acquire))
        {
            try
            {
                // Блокирует до stop() или до "закончилась работа". :contentReference[oaicite:4]{index=4}
                io_.run();
            }
            catch (...)
            {
                // лог/обработка исключений по желанию
            }

            // Если нас не просили остановиться — готовимся запускать run() снова
            if (isWorking_.load(std::memory_order_acquire))
            {
                io_.restart();
            }
        }
    }
    void onOpen()
    {
        std::cout << "void onOpen()" << '\n';
    }

    void onText(std::string_view s)
    {
        std::cout << "void onText(std::string_view" << s << '\n';
    }

    void onByte(std::span<const std::uint8_t> bytes)
    {
        std::cout << "void onByte(std::span<const " << '\n';
    }

    void onClose(boost::beast::websocket::close_reason reason)
    {
        std::cout << "void onClose(boost::beast::w" << reason.code << ' ' << reason.reason << '\n';
    }

    void onError(std::string_view stage, boost::beast::error_code ec)
    {
        std::cout << "void onError(std::string_vie" << stage << ' ' << ec << '\n';
    }
};