#include "mirai/detail/net_client.h"

namespace mpp::detail
{
    namespace
    {
        auto to_beast_sv(const std::string_view sv)
        {
            return beast::string_view{ sv.data(), sv.size() };
        }
    }

    void HttpRequestAwaitable::resume_with_error(const error_code ec)
    {
        ec_ = ec;
        handle_();
    }

    void HttpRequestAwaitable::on_connect(const error_code ec, const tcp::endpoint&)
    {
        ec ? resume_with_error(ec) : http::async_write(stream_, request_,
            std::bind_front(&HttpRequestAwaitable::on_write, this));
    }

    void HttpRequestAwaitable::on_write(const error_code ec, size_t)
    {
        ec ? resume_with_error(ec) : http::async_read(stream_, buffer_, response_,
            std::bind_front(&HttpRequestAwaitable::on_read, this));
    }

    void HttpRequestAwaitable::on_read(error_code ec, size_t)
    {
        if (ec)
        {
            resume_with_error(ec);
            return;
        }
        stream_.socket().shutdown(tcp::socket::shutdown_both, ec);
        if (ec && ec != beast::errc::not_connected) ec_ = ec;
        handle_();
    }

    void HttpRequestAwaitable::await_suspend(const std::coroutine_handle<> handle)
    {
        handle_ = handle;
        stream_.async_connect(endpoints_,
            std::bind_front(&HttpRequestAwaitable::on_connect, this));
    }

    HttpRequestAwaitable::Response HttpRequestAwaitable::await_resume()
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

    NetClient::NetClient(const std::string_view host, const std::string_view port): host_(host)
    {
        tcp::resolver resolver(io_ctx_);
        endpoints_ = resolver.resolve(host, port);
    }

    HttpRequestAwaitable NetClient::async_get(const std::string_view target)
    {
        http::request<http::string_body> req{ http::verb::get, to_beast_sv(target), 11 };
        req.set(http::field::host, host_);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        return HttpRequestAwaitable(io_ctx_, endpoints_, std::move(req));
    }

    HttpRequestAwaitable NetClient::async_post(const std::string_view target, std::string body)
    {
        http::request<http::string_body> req{ http::verb::post, to_beast_sv(target), 11 };
        req.set(http::field::host, host_);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json; charset=utf-8");
        req.body() = std::move(body);
        return HttpRequestAwaitable(io_ctx_, endpoints_, std::move(req));
    }
}
