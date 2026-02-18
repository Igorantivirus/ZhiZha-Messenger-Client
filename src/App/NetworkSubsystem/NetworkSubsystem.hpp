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

#include "NetEvent.hpp"
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
        io_.stop();

        if (netThread_.joinable())
        {
            netThread_.join();
        }

        client_.reset();
    }

    // Внешнее управление
    void setWorking(bool v)
    {
        v ? start() : stop();
    }
    bool isWorking() const noexcept
    {
        return isWorking_.load(std::memory_order_acquire);
    }

    IoContext &io() noexcept
    {
        return io_;
    }
    const IoContext &io() const noexcept
    {
        return io_;
    }

    void setClient(std::shared_ptr<ws::WebSocketClient> c)
    {
        client_ = std::move(c);
    }
    std::shared_ptr<ws::WebSocketClient> getClient() const
    {
        return client_;
    }

    // template <class F>
    // void post(F &&f)
    // {
    //     boost::asio::post(io_, std::forward<F>(f));
    // }

    ThreadSafeQueue<NetEvent> &getQueue()
    {
        return queue_;
    }
    const ThreadSafeQueue<NetEvent> &getQueue() const
    {
        return queue_;
    }

private:
    IoContext io_{};
    std::optional<WorkGuard> workGuard_;

    std::thread netThread_;
    std::atomic_bool isWorking_{false};

    std::shared_ptr<ws::WebSocketClient> client_;
    ThreadSafeQueue<NetEvent> queue_;

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
                io_.run();
            }
            catch (...)
            {
            }

            if (isWorking_.load(std::memory_order_acquire))
            {
                io_.restart();
            }
        }
    }
    void onOpen()
    {
        NetEvent ev;
        ev.setData(NetEvent::OnOpen{}, NetEvent::Type::onOpen);
        queue_.push(std::move(ev));
        std::cout << "void onOpen()" << '\n';
    }

    void onText(std::string s)
    {
        std::cout << "onText(std::string): " << s << '\n';
        NetEvent ev;
        ev.setData(NetEvent::OnText{.text = std::move(s)}, NetEvent::Type::onText);
        queue_.push(std::move(ev));
    }
    
    void onByte(std::vector<std::uint8_t> bytes)
    {
        NetEvent ev;
        ev.setData(NetEvent::OnBytes{.bytes = std::move(bytes)}, NetEvent::Type::onBytes);
        queue_.push(std::move(ev));
        std::cout << "onBytes(std::vector<uint8_t>): received" << '\n';
    }

    void onClose(boost::beast::websocket::close_reason reason)
    {
        NetEvent ev;
        ev.setData(NetEvent::OnClose{.reason = reason}, NetEvent::Type::onClose);
        queue_.push(std::move(ev));
        std::cout << "onClose(boost::beast::websocket::close_reason reason): " << reason.code << ' ' << reason.reason << '\n';
    }

    void onError(std::string_view stage, boost::beast::error_code ec)
    {
        NetEvent ev;
        ev.setData(NetEvent::OnError{.stage = std::string(stage), .ec = ec}, NetEvent::Type::onError);
        queue_.push(std::move(ev));
        std::cout << "void onError(std::string_view): " << stage << ' ' << ec << '\n';
    }
};

