#include "mirai/core/bot.h"

#include <simdjson.h>

#include "mirai/core/exceptions.h"
#include "mirai/message/message.h"
#include "mirai/detail/json.h"

namespace mpp
{
    namespace
    {
        thread_local simdjson::dom::parser parser;

        auto get_response_json(const detail::response& response)
        {
            using enum boost::beast::http::status_class;
            const auto status_class = to_status_class(response.result());
            if (status_class == client_error || status_class == server_error || status_class == unknown)
                throw HttpStatusException(response.result(), std::string(response.reason()));
            const detail::JsonElem json = parser.parse(response.body());
            const int64_t code = json["code"];
            check_status_code(static_cast<MiraiStatus>(code));
            return json;
        }
    }

    std::string Bot::release_body() const
    {
        return fmt::format(R"({{"sessionKey":{},"qq":{}}})",
            detail::JsonQuoted{ sess_key_ }, bot_id_.id);
    }

    Bot::~Bot() noexcept
    {
        try
        {
            if (authorized())
                release();
        }
        catch (...) { std::terminate(); }
    }

    clu::task<std::string> Bot::async_get_version()
    {
        const auto json = get_response_json(co_await net_client_.async_get("/about"));
        co_return std::string(json.at_pointer("/data/version"));
    }

    clu::task<> Bot::async_auth(const std::string_view auth_key, const UserId id)
    {
        if (authorized()) throw std::runtime_error("一个 Bot 实例只能绑定一个 bot 账号");

        auto auth_body = fmt::format(R"({{"authKey":{}}})", detail::JsonQuoted{ auth_key });
        const auto auth_json = get_response_json(
            co_await net_client_.async_post_json("/auth", std::move(auth_body)));
        sess_key_ = std::string(auth_json["session"]);

        auto verify_body = fmt::format(R"({{"sessionKey":{},"qq":{}}})", detail::JsonQuoted{ sess_key_ }, id.id);
        (void)get_response_json(co_await net_client_.async_post_json("/verify", std::move(verify_body)));

        bot_id_ = id;
    }

    void Bot::release()
    {
        (void)get_response_json(net_client_.post_json("/release", release_body()));
        bot_id_ = {};
        sess_key_.clear();
    }

    clu::task<> Bot::async_release()
    {
        (void)get_response_json(co_await net_client_.async_post_json("/release", release_body()));
        bot_id_ = {};
        sess_key_.clear();
    }

    clu::task<MessageId> Bot::async_send_message(
        const UserId id, const Message& message, const clu::optional_param<MessageId> quote)
    {
        auto body = detail::perform_format([&](fmt::format_context& ctx)
        {
            detail::JsonObjScope scope(ctx);
            scope.add_entry("sessionKey", sess_key_);
            scope.add_entry("target", id.id);
            scope.add_entry("messageChain", message);
            if (quote) scope.add_entry("quote", quote->id);
        });
        const auto res = get_response_json(
            co_await net_client_.async_post_json("/sendFriendMessage", std::move(body)));
        co_return MessageId(detail::from_json(res["messageId"]));
    }
}
