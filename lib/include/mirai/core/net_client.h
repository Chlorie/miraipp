#pragma once

#include <memory>
#include <string_view>
#include <chrono>
#include <clu/coroutine/task.h>
#include <clu/coroutine/cancellable_task.h>

namespace boost::asio
{
    class io_context;
}

namespace mpp::net
{
    using TimePoint = std::chrono::steady_clock::time_point;

    class WebsocketSession;

    class Client final
    {
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
        boost::asio::io_context& io_context();

        std::string_view host() const;

        std::string http_get(std::string_view target);
        clu::task<std::string> async_http_get(std::string_view target);
        std::string http_post(std::string_view target, std::string_view content_type, std::string body);
        clu::task<std::string> async_http_post(std::string_view target, std::string_view content_type, std::string body);
        std::string http_post_json(std::string_view target, std::string body);
        clu::task<std::string> async_http_post_json(std::string_view target, std::string body);

        clu::cancellable_task<bool> async_wait(TimePoint tp);

        WebsocketSession new_websocket_session();
        void connect_websocket(WebsocketSession& ws, std::string_view target);
        clu::task<> async_connect_websocket(WebsocketSession& ws, std::string_view target);

        Impl* pimpl_ptr() const { return impl_.get(); }
    };

    class WebsocketSession final
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
        clu::task<std::string> async_read();
        void close();
        clu::task<> async_close();

        Impl* pimpl_ptr() const { return impl_.get(); }
    };
}
