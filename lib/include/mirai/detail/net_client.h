#pragma once

#include <coroutine>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>

namespace mpp::detail
{
    namespace asio = boost::asio;
    namespace beast = boost::beast;
    namespace http = beast::http;
    using tcp = asio::ip::tcp;
    using error_code = boost::system::error_code;
    using endpoints = tcp::resolver::results_type;
    using time_point = std::chrono::steady_clock::time_point;

    inline void throw_on_error(const error_code ec) { if (ec) throw boost::system::system_error(ec); }

    class [[nodiscard]] HttpRequestAwaitable final
    {
    public:
        using Request = http::request<http::string_body>;
        using Response = http::response<http::dynamic_body>;

    private:
        const endpoints& endpoints_;
        beast::tcp_stream stream_;
        beast::flat_buffer buffer_;
        Request request_;
        error_code ec_;
        Response response_;
        std::coroutine_handle<> handle_;

        void resume_with_error(error_code ec);
        void on_connect(error_code ec, const tcp::endpoint&);
        void on_write(error_code ec, size_t);
        void on_read(error_code ec, size_t);

    public:
        explicit HttpRequestAwaitable(asio::io_context& ctx, const endpoints& eps, Request&& req):
            endpoints_(eps), stream_(ctx), request_(std::move(req)) {}

        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle);
        Response await_resume();
    };

    class [[nodiscard]] WaitUntilAwaitable final
    {
    private:
        asio::steady_timer timer_;
        time_point tp_;
        error_code ec_;

    public:
        WaitUntilAwaitable(asio::io_context& ctx, const time_point tp): timer_(ctx, tp) {}

        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle);
        void await_resume() const { throw_on_error(ec_); }
    };

    class NetClient final
    {
    private:
        asio::io_context io_ctx_;
        std::string host_;
        endpoints endpoints_;

    public:
        NetClient(std::string_view host, std::string_view port);

        HttpRequestAwaitable async_get(std::string_view target);
        HttpRequestAwaitable async_post(std::string_view target, std::string body);
        WaitUntilAwaitable async_wait(const time_point tp) { return WaitUntilAwaitable(io_ctx_, tp); }

        void run() { io_ctx_.run(); }
    };
}
