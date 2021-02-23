#include "mirai/detail/net_client.h"

#ifdef __RESHARPER__ // Resharper incorrect _MSC_VER workaround
#   define BOOST_ASIO_HAS_CO_AWAIT 1
#endif
#include <boost/asio/use_awaitable.hpp>

namespace mpp::detail
{
    namespace
    {
        auto to_beast_sv(const std::string_view sv)
        {
            return beast::string_view{ sv.data(), sv.size() };
        }
    }

    void HttpSession::resume_with_error(const error_code ec)
    {
        ec_ = ec;
        awaiting_();
    }

    void HttpSession::on_connect(const error_code ec, const tcp::endpoint&)
    {
        ec ? resume_with_error(ec) : http::async_write(stream_, request_,
            std::bind_front(&HttpSession::on_write, this));
    }

    void HttpSession::on_write(const error_code ec, size_t)
    {
        ec ? resume_with_error(ec) : http::async_read(stream_, buffer_, response_,
            std::bind_front(&HttpSession::on_read, this));
    }

    void HttpSession::on_read(error_code ec, size_t)
    {
        if (ec)
        {
            resume_with_error(ec);
            return;
        }
        stream_.socket().shutdown(tcp::socket::shutdown_both, ec);
        if (ec && ec != beast::errc::not_connected) ec_ = ec;
        awaiting_();
    }

    void HttpSession::await_suspend(const std::coroutine_handle<> handle)
    {
        awaiting_ = handle;
        stream_.async_connect(endpoints_,
            std::bind_front(&HttpSession::on_connect, this));
    }

    response HttpSession::await_resume()
    {
        throw_on_error(ec_);
        return std::move(response_);
    }

    void WaitUntilAwaitable::await_suspend(const std::coroutine_handle<> handle)
    {
        timer_.async_wait([=, this](const error_code ec)
        {
            ec_ = ec;
            handle.resume();
        });
    }

    bool WaitUntilAwaitable::await_resume() const
    {
        if (ec_ == boost::system::errc::operation_canceled)
            return false;
        else if (ec_)
            throw boost::system::system_error(ec_);
        return true;
    }

    void WebsocketSession::ConnectAwaiter::resume_with_error(const error_code ec)
    {
        ec_ = ec;
        awaiting_();
    }

    void WebsocketSession::ConnectAwaiter::on_connect(const error_code ec, const tcp::endpoint& ep)
    {
        if (ec)
        {
            resume_with_error(ec);
            return;
        }
        parent_.stream_.set_option(ws::stream_base::decorator([](ws::request_type& req)
        {
            req.set(http::field::user_agent,
                BOOST_BEAST_VERSION_STRING);
        }));

        host_ += ':' + std::to_string(ep.port());
        parent_.stream_.async_handshake(host_, target_,
            std::bind_front(&ConnectAwaiter::resume_with_error, this));
    }

    void WebsocketSession::ConnectAwaiter::await_suspend(const std::coroutine_handle<> handle)
    {
        awaiting_ = handle;
        get_lowest_layer(parent_.stream_).async_connect(eps_,
            std::bind_front(&ConnectAwaiter::on_connect, this));
    }

    void WebsocketSession::ReadAwaiter::await_suspend(const std::coroutine_handle<> handle)
    {
        parent_.stream_.async_read(parent_.buffer_,
            [=, this](const error_code ec, size_t)
            {
                ec_ = ec;
                handle();
            });
    }

    std::string WebsocketSession::ReadAwaiter::await_resume() const
    {
        const size_t size = parent_.buffer_.size();
        std::string result(static_cast<const char*>(parent_.buffer_.data().data()), size);
        parent_.buffer_.consume(size);
        return result;
    }

    void WebsocketSession::CloseAwaiter::await_suspend(const std::coroutine_handle<> handle)
    {
        parent_.stream_.async_close(ws::normal,
            [=, this](const error_code ec)
            {
                ec_ = ec;
                handle();
            });
    }

    WebsocketSession::~WebsocketSession() noexcept
    {
        if (stream_.is_open())
            try { stream_.close(ws::normal); }
            catch (...) { std::terminate(); }
    }

    WebsocketSession::ConnectAwaiter WebsocketSession::async_connect(
        const std::string_view host, const endpoints& eps, const std::string_view target)
    {
        return ConnectAwaiter(*this, host, eps, target);
    }

    request NetClient::generate_json_post_req(const std::string_view target, std::string&& body) const
    {
        http::request<http::string_body> req{ http::verb::post, to_beast_sv(target), 11 };
        req.set(http::field::host, host_);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json; charset=utf-8");
        req.content_length(body.size());
        req.body() = std::move(body);
        return req;
    }

    NetClient::NetClient(const std::string_view host, const std::string_view port): host_(host)
    {
        tcp::resolver resolver(io_ctx_);
        eps_ = resolver.resolve(host, port);
    }

    HttpSession NetClient::async_get(const std::string_view target)
    {
        http::request<http::string_body> req{ http::verb::get, to_beast_sv(target), 11 };
        req.set(http::field::host, host_);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        return HttpSession(io_ctx_, eps_, std::move(req));
    }

    HttpSession NetClient::async_post_json(const std::string_view target, std::string&& body)
    {
        return HttpSession(io_ctx_, eps_,
            generate_json_post_req(target, std::move(body)));
    }

    response NetClient::post_json(const std::string_view target, std::string&& body)
    {
        beast::tcp_stream stream(io_ctx_);
        beast::flat_buffer buffer;
        response result;
        stream.connect(eps_);
        http::write(stream, generate_json_post_req(target, std::move(body)));
        http::read(stream, buffer, result);
        error_code ec;
        stream.socket().shutdown(asio::socket_base::shutdown_both, ec);
        if (ec && ec != beast::errc::not_connected) throw_on_error(ec);
        return result;
    }
}
