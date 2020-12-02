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
    namespace ws = beast::websocket;
    using tcp = asio::ip::tcp;
    using error_code = boost::system::error_code;
    using endpoints = tcp::resolver::results_type;
    using time_point = std::chrono::steady_clock::time_point;
    using request = http::request<http::string_body>;
    using response = http::response<http::string_body>;

    inline void throw_on_error(const error_code ec) { if (ec) throw boost::system::system_error(ec); }

    class [[nodiscard]] HttpSession final
    {
    private:
        const endpoints& endpoints_;
        beast::tcp_stream stream_;
        beast::flat_buffer buffer_;
        request request_;
        error_code ec_;
        response response_;
        std::coroutine_handle<> awaiting_{};

        void resume_with_error(error_code ec);
        void on_connect(error_code ec, const tcp::endpoint&);
        void on_write(error_code ec, size_t);
        void on_read(error_code ec, size_t);

    public:
        HttpSession(asio::io_context& ctx, const endpoints& eps, request&& req):
            endpoints_(eps), stream_(ctx), request_(std::move(req)) {}

        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle);
        response await_resume();
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

        void cancel() { timer_.cancel(); }
    };

    class WebsocketSession final // NOLINT(cppcoreguidelines-special-member-functions)
    {
    private:
        ws::stream<beast::tcp_stream> stream_;
        beast::flat_buffer buffer_;

        class ConnectAwaiter
        {
        private:
            WebsocketSession& parent_;
            std::string host_;
            const endpoints& eps_;
            std::string target_;
            error_code ec_{};
            std::coroutine_handle<> awaiting_{};

            void resume_with_error(error_code ec);
            void on_connect(error_code ec, const tcp::endpoint& ep);

        public:
            ConnectAwaiter(WebsocketSession& parent, const std::string_view host,
                const endpoints& eps, const std::string_view target):
                parent_(parent), host_(host), eps_(eps), target_(target) {}

            bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<> handle);
            void await_resume() const { throw_on_error(ec_); }
        };

        class ReadAwaiter
        {
        private:
            WebsocketSession& parent_;
            error_code ec_{};

        public:
            explicit ReadAwaiter(WebsocketSession& parent): parent_(parent) {}

            bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<> handle);
            std::string await_resume() const;
        };

        class CloseAwaiter
        {
        private:
            WebsocketSession& parent_;
            error_code ec_{};

        public:
            explicit CloseAwaiter(WebsocketSession& parent): parent_(parent) {}

            bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<> handle);
            void await_resume() const { throw_on_error(ec_); }
        };

    public:
        explicit WebsocketSession(asio::io_context& ctx): stream_(make_strand(ctx)) {}
        ~WebsocketSession() noexcept;

        ConnectAwaiter async_connect(std::string_view host, const endpoints& eps, std::string_view target);
        auto async_read() { return ReadAwaiter(*this); }
        auto async_close() { return CloseAwaiter(*this); }
    };

    class NetClient final
    {
    private:
        asio::io_context io_ctx_;
        std::string host_;
        endpoints eps_;

        request generate_json_post_req(std::string_view target, std::string&& body) const;

    public:
        NetClient(std::string_view host, std::string_view port);

        HttpSession async_get(std::string_view target);
        auto async_request(request&& req) { return HttpSession(io_ctx_, eps_, std::move(req)); }
        HttpSession async_post_json(std::string_view target, std::string&& body);
        response post_json(std::string_view target, std::string&& body);
        auto async_wait(const time_point tp) { return WaitUntilAwaitable(io_ctx_, tp); }

        auto get_ws_session() { return WebsocketSession(io_ctx_); }
        auto async_connect_ws(WebsocketSession& ws, const std::string_view target) const { return ws.async_connect(host_, eps_, target); }

        asio::io_context& io_context() { return io_ctx_; }
        const std::string& host() const noexcept { return host_; }
        void run() { io_ctx_.run(); }
    };
}
