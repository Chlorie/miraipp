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
#include <clu/outcome.h>
#include <fmt/core.h>
#include <unifex/just.hpp>

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

        class WaitUntilSender final
        {
        public:
            template <
                template <typename...> typename Var,
                template <typename...> typename Tup>
            using value_types = Var<Tup<>>;

            template <template <typename...> typename Var>
            using error_types = Var<std::exception_ptr>;

            static constexpr bool sends_done = true;

            template <typename Recv>
            class Operation final
            {
            private:
                struct Stopper final
                {
                    WaitUntilSender& sender;
                    void operator()() const
                    {
                        asio::execution::execute(sender.strand_,
                            [&t = sender.timer_]() { t.cancel(); });
                    }
                };

                using Callback = typename ex::stop_token_type_t<Recv&>::template callback_type<Stopper>;

                WaitUntilSender& send_;
                Recv recv_;
                std::optional<Callback> cb_;

                Recv&& receiver() { return static_cast<Recv&&>(recv_); }

            public:
                Operation(WaitUntilSender& send, Recv&& recv):
                    send_(send), recv_(static_cast<Recv&&>(recv)) {}

                void start() & noexcept
                {
                    try
                    {
                        cb_.emplace(ex::get_stop_token(recv_), Stopper{ send_ });
                        send_.timer_.async_wait(asio::bind_executor(send_.strand_,
                            [this](const sys::error_code ec)
                            {
                                if (ec == asio::error::operation_aborted)
                                    ex::set_done(receiver());
                                else if (ec)
                                    ex::set_error(receiver(), std::make_exception_ptr(std::system_error(ec)));
                                else
                                    ex::set_value(receiver());
                            }));
                    }
                    catch (...) { ex::set_error(receiver(), std::current_exception()); }
                }
            };

        private:
            asio::strand<asio::io_context::executor_type> strand_;
            asio::steady_timer timer_;

        public:
            WaitUntilSender(asio::io_context& ctx, const TimePoint tp):
                strand_(make_strand(ctx)), timer_(ctx, tp) {}

            template <ex::receiver Recv>
            Operation<Recv> connect(Recv&& recv) noexcept { return { *this, static_cast<Recv&&>(recv) }; }
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
            if (const auto status_class = to_status_class(res.result());
                status_class == client_error || status_class == server_error || status_class == unknown)
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

        static auto get_ws_stream_decorator()
        {
            return ws::stream_base::decorator([](ws::request_type& req)
            {
                req.set(http::field::user_agent,
                    BOOST_BEAST_VERSION_STRING);
            });
        }

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

        ex::task<std::string> http_request_async(request req)
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

        auto wait_async(const TimePoint tp) { return WaitUntilSender(ctx_, tp); }

        void connect_websocket(ws_stream& stream, const std::string_view target)
        {
            const auto ep = get_lowest_layer(stream).connect(eps_);
            stream.set_option(get_ws_stream_decorator());
            stream.handshake(fmt::format("{}:{}", host_, ep.port()), std::string(target));
        }

        ex::task<void> connect_websocket_async(ws_stream& stream, const std::string_view target)
        {
            const auto impl = [&]() -> asio::awaitable<void>
            {
                const auto ep = co_await get_lowest_layer(stream).async_connect(eps_, asio::use_awaitable);
                stream.set_option(get_ws_stream_decorator());
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

        std::string read()
        {
            stream_.read(buffer_);
            const size_t size = buffer_.size();
            std::string result(static_cast<const char*>(buffer_.data().data()), size);
            buffer_.consume(size);
            return std::move(result);
        }

        ex::task<std::string> read_async()
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

        void close() { stream_.close(ws::normal); }

        ex::task<void> close_async()
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

    ex::task<std::string> Client::http_get_async(const std::string_view target)
    {
        return impl_->http_request_async(
            impl_->generate_http_get_request(target));
    }

    std::string Client::http_post(const std::string_view target,
        const std::string_view content_type, std::string body)
    {
        return impl_->http_request(
            impl_->generate_http_post_request(target, content_type, std::move(body)));
    }

    ex::task<std::string> Client::http_post_async(const std::string_view target,
        const std::string_view content_type, std::string body)
    {
        return impl_->http_request_async(
            impl_->generate_http_post_request(target, content_type, std::move(body)));
    }

    std::string Client::http_post_json(const std::string_view target, std::string body)
    {
        return impl_->http_request(
            impl_->generate_http_post_request(target, json_content_type, std::move(body)));
    }

    ex::task<std::string> Client::http_post_json_async(const std::string_view target, std::string body)
    {
        return impl_->http_request_async(
            impl_->generate_http_post_request(target, json_content_type, std::move(body)));
    }

    ex::task<void> Client::wait_async(const TimePoint tp) { co_await impl_->wait_async(tp); }

    WebsocketSession Client::new_websocket_session() { return WebsocketSession(io_context()); }

    void Client::connect_websocket(WebsocketSession& ws, const std::string_view target)
    {
        return impl_->connect_websocket(
            ws.pimpl_ptr()->stream_, target);
    }

    ex::task<void> Client::connect_websocket_async(WebsocketSession& ws, const std::string_view target)
    {
        return impl_->connect_websocket_async(
            ws.pimpl_ptr()->stream_, target);
    }

    WebsocketSession::WebsocketSession(asio::io_context& ctx): impl_(std::make_unique<Impl>(ctx)) {}
    WebsocketSession::~WebsocketSession() noexcept = default;
    WebsocketSession::WebsocketSession(WebsocketSession&&) noexcept = default;
    WebsocketSession& WebsocketSession::operator=(WebsocketSession&&) noexcept = default;

    std::string WebsocketSession::read() { return impl_->read(); }
    ex::task<std::string> WebsocketSession::read_async() { return impl_->read_async(); }
    void WebsocketSession::close() { return impl_->close(); }
    ex::task<void> WebsocketSession::close_async() { return impl_->close_async(); }
    // ReSharper restore CppMemberFunctionMayBeConst
}
