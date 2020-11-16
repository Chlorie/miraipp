#include "mirai/core/bot.h"

#include <simdjson.h>

#include "mirai/core/exceptions.h"
#include "mirai/message/message.h"
#include "mirai/detail/multipart_builder.h"
#include "mirai/event/event.h"

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
            const auto code = json["code"];
            if (code.error() != simdjson::NO_SUCH_FIELD)
            {
                const int64_t int_code = code;
                check_status_code(static_cast<MiraiStatus>(int_code));
            }
            return json;
        }

        auto to_beast_sv(const std::string_view sv)
        {
            return boost::beast::string_view{ sv.data(), sv.size() };
        }
    }

    std::string Bot::release_body() const
    {
        return fmt::format(R"({{"sessionKey":{},"qq":{}}})",
            detail::JsonQuoted{ sess_key_ }, bot_id_.id);
    }

    std::string Bot::send_message_body(
        const int64_t id, const Message& message, const clu::optional_param<MessageId> quote) const
    {
        return detail::perform_format([&](fmt::format_context& ctx)
        {
            detail::JsonObjScope scope(ctx);
            scope.add_entry("sessionKey", sess_key_);
            scope.add_entry("target", id);
            scope.add_entry("messageChain", message);
            if (quote) scope.add_entry("quote", quote->id);
        });
    }

    std::string Bot::group_target_body(GroupId group) const
    {
        return detail::perform_format([&](fmt::format_context& ctx)
        {
            detail::JsonObjScope scope(ctx);
            scope.add_entry("sessionKey", sess_key_);
            scope.add_entry("target", group.id);
        });
    }

    std::vector<Event> Bot::parse_events(const detail::JsonElem json)
    {
        std::vector<Event> events;
        for (const auto elem : json) 
            events.emplace_back(Event::from_json(elem)).event_base().bot_ = this;
        return events;
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
        const auto res = get_response_json(
            co_await net_client_.async_post_json("/sendFriendMessage", send_message_body(id.id, message, quote)));
        co_return MessageId(detail::from_json(res["messageId"]));
    }

    clu::task<MessageId> Bot::async_send_message(
        const GroupId id, const Message& message, const clu::optional_param<MessageId> quote)
    {
        const auto res = get_response_json(
            co_await net_client_.async_post_json("/sendGroupMessage", send_message_body(id.id, message, quote)));
        co_return MessageId(detail::from_json(res["messageId"]));
    }

    clu::task<MessageId> Bot::async_send_message(
        const TempId id, const Message& message, const clu::optional_param<MessageId> quote)
    {
        auto body = detail::perform_format([&](fmt::format_context& ctx)
        {
            detail::JsonObjScope scope(ctx);
            scope.add_entry("sessionKey", sess_key_);
            scope.add_entry("qq", id.uid.id);
            scope.add_entry("group", id.gid.id);
            scope.add_entry("messageChain", message);
            if (quote) scope.add_entry("quote", quote->id);
        });
        const auto res = get_response_json(
            co_await net_client_.async_post_json("/sendTempMessage", std::move(body)));
        co_return MessageId(detail::from_json(res["messageId"]));
    }

    clu::task<> Bot::async_recall(const MessageId id)
    {
        auto body = detail::perform_format([&](fmt::format_context& ctx)
        {
            detail::JsonObjScope scope(ctx);
            scope.add_entry("sessionKey", sess_key_);
            scope.add_entry("target", id.id);
        });
        (void)get_response_json(co_await net_client_.async_post_json("/recall", std::move(body)));
    }

    clu::task<Image> Bot::async_upload_image(const TargetType type, const std::filesystem::path& path)
    {
        using namespace detail;

        request req{ http::verb::post, "/uploadImage", 11 };
        req.set(http::field::host, net_client_.host());
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, to_beast_sv(MultipartBuilder::content_type));

        MultipartBuilder builder;
        builder.add_key_value("sessionKey", sess_key_);
        builder.add_key_value("type", to_string_view(type));
        builder.add_file("img", path);
        req.body() = builder.take_string();
        req.content_length(req.body().size());

        const auto res = get_response_json(co_await net_client_.async_request(std::move(req)));
        co_return Image::from_json(res);
    }

    clu::task<Voice> Bot::async_upload_voice(const TargetType type, const std::filesystem::path& path)
    {
        using namespace detail;

        request req{ http::verb::post, "/uploadVoice", 11 };
        req.set(http::field::host, net_client_.host());
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, to_beast_sv(MultipartBuilder::content_type));

        MultipartBuilder builder;
        builder.add_key_value("sessionKey", sess_key_);
        builder.add_key_value("type", to_string_view(type));
        builder.add_file("voice", path);
        req.body() = builder.take_string();
        req.content_length(req.body().size());

        const auto res = get_response_json(co_await net_client_.async_request(std::move(req)));
        co_return Voice::from_json(res);
    }

    clu::task<std::vector<Event>> Bot::async_pop_events(const size_t count)
    {
        const auto json = get_response_json(co_await net_client_.async_get(
            fmt::format("/fetchMessage?sessionKey={}&count={}", sess_key_, count)));
        co_return parse_events(json["data"]);
    }

    clu::task<std::vector<Event>> Bot::async_pop_latest_events(const size_t count)
    {
        const auto json = get_response_json(co_await net_client_.async_get(
            fmt::format("/fetchLatestMessage?sessionKey={}&count={}", sess_key_, count)));
        co_return parse_events(json["data"]);
    }

    clu::task<std::vector<Event>> Bot::async_peek_events(const size_t count)
    {
        const auto json = get_response_json(co_await net_client_.async_get(
            fmt::format("/peekMessage?sessionKey={}&count={}", sess_key_, count)));
        co_return parse_events(json["data"]);
    }

    clu::task<std::vector<Event>> Bot::async_peek_latest_events(const size_t count)
    {
        const auto json = get_response_json(co_await net_client_.async_get(
            fmt::format("/peekLatestMessage?sessionKey={}&count={}", sess_key_, count)));
        co_return parse_events(json["data"]);
    }

    clu::task<> Bot::async_mute(const GroupId group, const UserId user, std::chrono::seconds duration)
    {
        auto body = detail::perform_format([&](fmt::format_context& ctx)
        {
            detail::JsonObjScope scope(ctx);
            scope.add_entry("sessionKey", sess_key_);
            scope.add_entry("target", group.id);
            scope.add_entry("memberId", user.id);
            scope.add_entry("time", duration.count());
        });
        (void)get_response_json(co_await net_client_.async_post_json("/mute", std::move(body)));
    }

    clu::task<> Bot::async_unmute(const GroupId group, const UserId user)
    {
        auto body = detail::perform_format([&](fmt::format_context& ctx)
        {
            detail::JsonObjScope scope(ctx);
            scope.add_entry("sessionKey", sess_key_);
            scope.add_entry("target", group.id);
            scope.add_entry("memberId", user.id);
        });
        (void)get_response_json(co_await net_client_.async_post_json("/unmute", std::move(body)));
    }

    clu::task<> Bot::async_mute_all(const GroupId group)
    {
        (void)get_response_json(co_await net_client_.async_post_json(
            "/muteAll", group_target_body(group)));
    }

    clu::task<> Bot::async_unmute_all(const GroupId group)
    {
        (void)get_response_json(co_await net_client_.async_post_json(
            "/unmuteAll", group_target_body(group)));
    }

    clu::task<> Bot::async_kick(const GroupId group, const UserId user, const std::string_view reason)
    {
        auto body = detail::perform_format([&](fmt::format_context& ctx)
        {
            detail::JsonObjScope scope(ctx);
            scope.add_entry("sessionKey", sess_key_);
            scope.add_entry("target", group.id);
            scope.add_entry("memberId", user.id);
            scope.add_entry("msg", reason);
        });
        (void)get_response_json(co_await net_client_.async_post_json("/kick", std::move(body)));
    }

    clu::task<> Bot::async_quit(const GroupId group)
    {
        (void)get_response_json(co_await net_client_.async_post_json(
            "/quit", group_target_body(group)));
    }
}
