#pragma once

#include <memory>
#include <string_view>
#include <chrono>
#include <unifex/task.hpp>

#include "export.h"

namespace boost::asio
{
    class io_context;
}

namespace mpp::net
{
    namespace ex = unifex;
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;

    class WebsocketSession;

    MPP_SUPPRESS_EXPORT_WARNING
    class MPP_API Client final
    {
    public:
        class MPP_API Scheduler final
        {
        public:
            using time_point = TimePoint;

        private:
            Client& client_;

        public:
            explicit Scheduler(Client& client): client_(client) {}
            static TimePoint now() noexcept { return Clock::now(); }
            ex::task<void> schedule() const { return client_.schedule(); }
            ex::task<void> schedule_at(const TimePoint tp) const { return client_.wait_async(tp); }
            ex::task<void> schedule_after(const Duration dur) const { return client_.wait_async(now() + dur); }

            boost::asio::io_context& io_context() const noexcept { return client_.io_context(); }

            [[nodiscard]] bool operator==(const Scheduler& other) const noexcept { return &client_ == &other.client_; }
        };

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;

    public:
        Client(std::string_view host, std::string_view port);
        ~Client() noexcept;
        Client(const Client&) = delete;
        Client(Client&&) noexcept;
        Client& operator=(const Client&) = delete;
        Client& operator=(Client&&) noexcept;

        void run();
        boost::asio::io_context& io_context() noexcept;

        std::string_view host() const noexcept;

        std::string http_get(std::string_view target);
        ex::task<std::string> http_get_async(std::string_view target);
        std::string http_post(std::string_view target, std::string_view content_type, std::string body);
        ex::task<std::string> http_post_async(std::string_view target, std::string_view content_type, std::string body);
        std::string http_post_json(std::string_view target, std::string body);
        ex::task<std::string> http_post_json_async(std::string_view target, std::string body);

        ex::task<void> schedule();
        ex::task<void> wait_async(TimePoint tp);

        WebsocketSession new_websocket_session();
        void connect_websocket(WebsocketSession& ws, std::string_view target);
        ex::task<void> connect_websocket_async(WebsocketSession& ws, std::string_view target);

        Impl* pimpl_ptr() const { return impl_.get(); }
    };

    class MPP_API WebsocketSession final
    {
    private:
        class Impl;
        std::unique_ptr<Impl> impl_;

    public:
        explicit WebsocketSession(boost::asio::io_context& ctx);
        ~WebsocketSession() noexcept;
        WebsocketSession(const WebsocketSession&) = delete;
        WebsocketSession(WebsocketSession&&) noexcept;
        WebsocketSession& operator=(const WebsocketSession&) = delete;
        WebsocketSession& operator=(WebsocketSession&&) noexcept;

        std::string read();
        ex::task<std::string> read_async();
        void close();
        ex::task<void> close_async();

        Impl* pimpl_ptr() const noexcept { return impl_.get(); }
    };
    MPP_RESTORE_EXPORT_WARNING
}
