#include "mirai/core/net_client.h"

#ifdef __RESHARPER__ // Resharper workaround
#   define BOOST_ASIO_HAS_CO_AWAIT 1
#   define BOOST_ASIO_HAS_STD_COROUTINE 1
#endif

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <fmt/core.h>

#include "mirai/core/exceptions.h"

namespace mpp::net
{
    namespace sys = boost::system;
    namespace asio = boost::asio;
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace ws = beast::websocket;

    using tcp = asio::ip::tcp;
    using error_code = boost::system::error_code;
    using endpoints = tcp::resolver::results_type;
    using request = http::request<http::string_body>;
    using response = http::response<http::string_body>;
    using ws_stream = ws::stream<beast::tcp_stream>;

    namespace
    {
        template <typename T>
        class AsioAwaiter
        {
        private:
            asio::io_context& ctx_;
            asio::awaitable<T> awt_;
            clu::outcome<T> result_;

        public:
            AsioAwaiter(asio::io_context& context, asio::awaitable<T> awaitable):
                ctx_(context), awt_(std::move(awaitable)) {}

            bool await_ready() const { return false; }

            void await_suspend(const std::coroutine_handle<> hdl)
            {
                co_spawn(ctx_, std::move(awt_), [&, hdl](const std::exception_ptr eptr, T value)
                {
                    if (eptr)
                        result_ = eptr;
                    else
                        result_ = std::move(value);
                    hdl.resume();
                });
            }

            T await_resume()
            {
                result_.throw_if_exceptional();
                return *std::move(result_);
            }
        };

        template <>
        class AsioAwaiter<void>
        {
        private:
            asio::io_context& ctx_;
            asio::awaitable<void> awt_;
            std::exception_ptr eptr_;

        public:
            AsioAwaiter(asio::io_context& context, asio::awaitable<void> awaitable):
                ctx_(context), awt_(std::move(awaitable)) {}

            bool await_ready() const { return false; }

            void await_suspend(const std::coroutine_handle<> hdl)
            {
                co_spawn(ctx_, std::move(awt_), [&, hdl](const std::exception_ptr eptr)
                {
                    if (eptr) eptr_ = eptr;
                    hdl.resume();
                });
            }

            void await_resume() const
            {
                if (eptr_)
                    std::rethrow_exception(eptr_);
            }
        };

        auto to_beast_sv(const std::string_view sv) { return beast::string_view(sv.data(), sv.size()); }

        void shutdown_socket(beast::tcp_stream& stream)
        {
            error_code ec;
            stream.socket().shutdown(asio::socket_base::shutdown_both, ec);
            if (ec && ec != beast::errc::not_connected) throw sys::system_error(ec);
        }

        void check_response_status(const response& res)
        {
            using enum http::status_class;
            const auto status_class = to_status_class(res.result());
            if (status_class == client_error || status_class == server_error || status_class == unknown)
                throw HttpStatusException(res.result(), std::string(res.reason()));
        }

        constexpr std::string_view json_content_type = "application/json; charset=utf-8";
    }

    class Client::Impl final
    {
    private:
        asio::io_context ctx_;
        std::string host_;
        endpoints eps_;

        class WaitUntilAwaiter final
        {
        private:
            asio::steady_timer timer_;
            TimePoint tp_;
            error_code ec_;

        public:
            WaitUntilAwaiter(asio::io_context& ctx, const TimePoint tp): timer_(ctx, tp) {}

            bool await_ready() const noexcept { return false; }

            void await_suspend(const std::coroutine_handle<> handle)
            {
                timer_.async_wait([=, this](const error_code ec)
                {
                    ec_ = ec;
                    handle.resume();
                });
            }

            bool await_resume() const
            {
                if (ec_ == sys::errc::operation_canceled)
                    return false;
                else if (ec_)
                    throw sys::system_error(ec_);
                return true;
            }

            void cancel() { timer_.cancel(); }
        };

    public:
        Impl(const std::string_view host, const std::string_view port): host_(host)
        {
            tcp::resolver resolver(ctx_);
            eps_ = resolver.resolve(host, port);
        }

        asio::io_context& io_context() { return ctx_; }
        std::string_view host() const { return host_; }

        std::string http_request(const request& req)
        {
            beast::tcp_stream stream(ctx_);
            stream.connect(eps_);
            http::write(stream, req);

            beast::flat_buffer buffer;
            response res;
            http::read(stream, buffer, res);

            shutdown_socket(stream);
            check_response_status(res);
            return std::move(res).body();
        }

        clu::task<std::string> async_http_request(request req)
        {
            const auto impl = [&]() -> asio::awaitable<std::string>
            {
                beast::tcp_stream stream(ctx_);
                co_await stream.async_connect(eps_, asio::use_awaitable);
                co_await http::async_write(stream, req, asio::use_awaitable);

                beast::flat_buffer buffer;
                response res;
                co_await http::async_read(stream, buffer, res, asio::use_awaitable);

                shutdown_socket(stream);
                check_response_status(res);
                co_return std::move(res).body();
            };
            co_return co_await AsioAwaiter(ctx_, impl());
        }

        request generate_http_get_request(const std::string_view target) const
        {
            request req{ http::verb::get, to_beast_sv(target), 11 };
            req.set(http::field::host, host_);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
            return req;
        }

        request generate_http_post_request(const std::string_view target,
            const std::string_view content_type, std::string&& body) const
        {
            http::request<http::string_body> req{ http::verb::post, to_beast_sv(target), 11 };
            req.set(http::field::host, host_);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
            req.set(http::field::content_type, to_beast_sv(content_type));
            req.body() = std::move(body);
            req.prepare_payload();
            return req;
        }

        auto async_wait(const TimePoint tp) { return WaitUntilAwaiter(ctx_, tp); }

        clu::task<> async_connect_websocket(ws_stream& stream, const std::string_view target)
        {
            const auto impl = [&]() -> asio::awaitable<void>
            {
                const auto ep = co_await get_lowest_layer(stream).async_connect(eps_, asio::use_awaitable);
                stream.set_option(ws::stream_base::decorator([](ws::request_type& req)
                {
                    req.set(http::field::user_agent,
                        BOOST_BEAST_VERSION_STRING);
                }));
                co_await stream.async_handshake(
                    fmt::format("{}:{}", host_, ep.port()), std::string(target), asio::use_awaitable);
            };
            co_return co_await AsioAwaiter(ctx_, impl());
        }
    };

    class WebsocketSession::Impl final
    {
        friend class Client;
    private:
        asio::io_context& ctx_;
        ws_stream stream_;
        beast::flat_buffer buffer_;

    public:
        explicit Impl(asio::io_context& ctx): ctx_(ctx), stream_(make_strand(ctx)) {}

        clu::task<std::string> async_read()
        {
            const auto impl = [&]() -> asio::awaitable<std::string>
            {
                co_await stream_.async_read(buffer_, asio::use_awaitable);
                const size_t size = buffer_.size();
                std::string result(static_cast<const char*>(buffer_.data().data()), size);
                buffer_.consume(size);
                co_return std::move(result);
            };
            co_return co_await AsioAwaiter(ctx_, impl());
        }

        clu::task<> async_close()
        {
            const auto impl = [&]() -> asio::awaitable<void>
            {
                co_await stream_.async_close(ws::normal, asio::use_awaitable);
            };
            co_await AsioAwaiter(ctx_, impl());
        }
    };

    Client::Client(const std::string_view host, const std::string_view port): impl_(std::make_unique<Impl>(host, port)) {}
    Client::~Client() noexcept = default;
    Client::Client(Client&&) noexcept = default;
    Client& Client::operator=(Client&&) noexcept = default;

    // ReSharper disable CppMemberFunctionMayBeConst
    void Client::run() { io_context().run(); }
    asio::io_context& Client::io_context() { return impl_->io_context(); }
    std::string_view Client::host() const { return impl_->host(); }

    std::string Client::http_get(const std::string_view target)
    {
        return impl_->http_request(
            impl_->generate_http_get_request(target));
    }

    clu::task<std::string> Client::async_http_get(const std::string_view target)
    {
        return impl_->async_http_request(
            impl_->generate_http_get_request(target));
    }

    std::string Client::http_post(const std::string_view target,
        const std::string_view content_type, std::string body)
    {
        return impl_->http_request(
            impl_->generate_http_post_request(target, content_type, std::move(body)));
    }

    clu::task<std::string> Client::async_http_post(const std::string_view target,
        const std::string_view content_type, std::string body)
    {
        return impl_->async_http_request(
            impl_->generate_http_post_request(target, content_type, std::move(body)));
    }

    std::string Client::http_post_json(const std::string_view target, std::string body)
    {
        return impl_->http_request(
            impl_->generate_http_post_request(target, json_content_type, std::move(body)));
    }

    clu::task<std::string> Client::async_http_post_json(const std::string_view target, std::string body)
    {
        return impl_->async_http_request(
            impl_->generate_http_post_request(target, json_content_type, std::move(body)));
    }

    clu::cancellable_task<bool> Client::async_wait(const TimePoint tp) { co_return co_await impl_->async_wait(tp); }

    WebsocketSession Client::new_websocket_session() { return WebsocketSession(io_context()); }

    clu::task<> Client::async_connect_websocket(WebsocketSession& ws, const std::string_view target)
    {
        return impl_->async_connect_websocket(
            ws.pimpl_ptr()->stream_, target);
    }

    WebsocketSession::WebsocketSession(asio::io_context& ctx): impl_(std::make_unique<Impl>(ctx)) {}
    WebsocketSession::~WebsocketSession() noexcept = default;
    WebsocketSession::WebsocketSession(WebsocketSession&&) noexcept = default;
    WebsocketSession& WebsocketSession::operator=(WebsocketSession&&) noexcept = default;

    clu::task<std::string> WebsocketSession::async_read() { return impl_->async_read(); }
    clu::task<> WebsocketSession::async_close() { return impl_->async_close(); }
    // ReSharper restore CppMemberFunctionMayBeConst
}
