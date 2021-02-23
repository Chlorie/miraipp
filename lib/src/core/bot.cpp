#include "mirai/core/bot.h"

#include <simdjson.h>
#include <clu/coroutine/coroutine_scope.h>

#include "mirai/core/exceptions.h"
#include "mirai/message/message.h"
#include "mirai/detail/multipart_builder.h"
#include "mirai/event/event_types.h"

namespace mpp
{
    namespace
    {
        thread_local simdjson::dom::parser parser;
        
        void check_json(const detail::JsonRes json)
        {
            const auto code = json["code"];
            if (code.error() != simdjson::NO_SUCH_FIELD)
            {
                const int64_t int_code = code;
                check_status_code(static_cast<MiraiStatus>(int_code));
            }
        }

        auto get_checked_response_json(const std::string& response)
        {
            const auto json = parser.parse(response);
            check_json(json);
            return json;
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

    Event Bot::parse_event(const detail::JsonElem json)
    {
        Event ev = Event::from_json(json);
        ev.event_base().bot_ = this;
        return ev;
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
        const auto json = get_checked_response_json(co_await net_client_.async_http_get("/about"));
        co_return std::string(json.at_pointer("/data/version"));
    }

    clu::task<> Bot::async_authorize(const std::string_view auth_key, const UserId id)
    {
        if (authorized()) throw std::runtime_error("一个 Bot 实例只能绑定一个 bot 账号");

        auto auth_body = fmt::format(R"({{"authKey":{}}})", detail::JsonQuoted{ auth_key });
        const auto auth_json = get_checked_response_json(
            co_await net_client_.async_http_post_json("/auth", std::move(auth_body)));
        sess_key_ = std::string(auth_json["session"]);

        auto verify_body = fmt::format(R"({{"sessionKey":{},"qq":{}}})", detail::JsonQuoted{ sess_key_ }, id.id);
        (void)get_checked_response_json(co_await net_client_.async_http_post_json("/verify", std::move(verify_body)));

        bot_id_ = id;
    }

    void Bot::release()
    {
        (void)get_checked_response_json(net_client_.http_post_json("/release", release_body()));
        bot_id_ = {};
        sess_key_.clear();
    }

    clu::task<> Bot::async_release()
    {
        (void)get_checked_response_json(co_await net_client_.async_http_post_json("/release", release_body()));
        bot_id_ = {};
        sess_key_.clear();
    }

    clu::task<MessageId> Bot::async_send_message(
        const UserId id, const Message& message, const clu::optional_param<MessageId> quote)
    {
        const auto res = get_checked_response_json(
            co_await net_client_.async_http_post_json("/sendFriendMessage", send_message_body(id.id, message, quote)));
        co_return MessageId(detail::from_json<int32_t>(res["messageId"]));
    }

    clu::task<MessageId> Bot::async_send_message(
        const GroupId id, const Message& message, const clu::optional_param<MessageId> quote)
    {
        const auto res = get_checked_response_json(
            co_await net_client_.async_http_post_json("/sendGroupMessage", send_message_body(id.id, message, quote)));
        co_return MessageId(detail::from_json<int32_t>(res["messageId"]));
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
        const auto res = get_checked_response_json(
            co_await net_client_.async_http_post_json("/sendTempMessage", std::move(body)));
        co_return MessageId(detail::from_json<int32_t>(res["messageId"]));
    }

    clu::task<> Bot::async_recall(const MessageId id)
    {
        auto body = detail::perform_format([&](fmt::format_context& ctx)
        {
            detail::JsonObjScope scope(ctx);
            scope.add_entry("sessionKey", sess_key_);
            scope.add_entry("target", id.id);
        });
        (void)get_checked_response_json(co_await net_client_.async_http_post_json("/recall", std::move(body)));
    }

    clu::task<std::vector<std::string>> Bot::async_send_image_message(
        const UserId id, const std::span<const std::string> urls)
    {
        auto body = detail::perform_format([&](fmt::format_context& ctx)
        {
            detail::JsonObjScope scope(ctx);
            scope.add_entry("sessionKey", sess_key_);
            scope.add_entry("qq", id.id);
            scope.add_entry("urls", urls);
        });
        const auto json = get_checked_response_json(
            co_await net_client_.async_http_post_json("/sendImageMessage", std::move(body)));
        co_return detail::from_json<std::vector<std::string>>(json);
    }

    clu::task<std::vector<std::string>> Bot::async_send_image_message(
        const GroupId id, const std::span<const std::string> urls)
    {
        auto body = detail::perform_format([&](fmt::format_context& ctx)
        {
            detail::JsonObjScope scope(ctx);
            scope.add_entry("sessionKey", sess_key_);
            scope.add_entry("group", id.id);
            scope.add_entry("urls", urls);
        });
        const auto json = get_checked_response_json(
            co_await net_client_.async_http_post_json("/sendImageMessage", std::move(body)));
        co_return detail::from_json<std::vector<std::string>>(json);
    }

    clu::task<std::vector<std::string>> Bot::async_send_image_message(
        const TempId id, const std::span<const std::string> urls)
    {
        auto body = detail::perform_format([&](fmt::format_context& ctx)
        {
            detail::JsonObjScope scope(ctx);
            scope.add_entry("sessionKey", sess_key_);
            scope.add_entry("qq", id.uid.id);
            scope.add_entry("group", id.gid.id);
            scope.add_entry("urls", urls);
        });
        const auto json = get_checked_response_json(
            co_await net_client_.async_http_post_json("/sendImageMessage", std::move(body)));
        co_return detail::from_json<std::vector<std::string>>(json);
    }

    clu::task<Image> Bot::async_upload_image(const TargetType type, const std::filesystem::path& path)
    {
        using detail::MultipartBuilder;

        MultipartBuilder builder;
        builder.add_key_value("sessionKey", sess_key_);
        builder.add_key_value("type", to_string_view(type));
        builder.add_file("img", path);

        auto res = get_checked_response_json(co_await net_client_.async_http_post(
            "/uploadImage", MultipartBuilder::content_type, builder.take_string()));
        co_return Image::from_json(res.value());
    }

    clu::task<Voice> Bot::async_upload_voice(const TargetType type, const std::filesystem::path& path)
    {
        using detail::MultipartBuilder;

        MultipartBuilder builder;
        builder.add_key_value("sessionKey", sess_key_);
        builder.add_key_value("type", to_string_view(type));
        builder.add_file("voice", path);
        
        auto res = get_checked_response_json(co_await net_client_.async_http_post(
            "/uploadVoice", MultipartBuilder::content_type, builder.take_string()));
        co_return Voice::from_json(res.value());
    }

    clu::task<std::vector<Event>> Bot::async_pop_events(const size_t count)
    {
        const auto json = get_checked_response_json(co_await net_client_.async_http_get(
            fmt::format("/fetchMessage?sessionKey={}&count={}", sess_key_, count)));
        co_return parse_events(json["data"]);
    }

    clu::task<std::vector<Event>> Bot::async_pop_latest_events(const size_t count)
    {
        const auto json = get_checked_response_json(co_await net_client_.async_http_get(
            fmt::format("/fetchLatestMessage?sessionKey={}&count={}", sess_key_, count)));
        co_return parse_events(json["data"]);
    }

    clu::task<std::vector<Event>> Bot::async_peek_events(const size_t count)
    {
        const auto json = get_checked_response_json(co_await net_client_.async_http_get(
            fmt::format("/peekMessage?sessionKey={}&count={}", sess_key_, count)));
        co_return parse_events(json["data"]);
    }

    clu::task<std::vector<Event>> Bot::async_peek_latest_events(const size_t count)
    {
        const auto json = get_checked_response_json(co_await net_client_.async_http_get(
            fmt::format("/peekLatestMessage?sessionKey={}&count={}", sess_key_, count)));
        co_return parse_events(json["data"]);
    }

    clu::task<Event> Bot::async_retrieve_message(const MessageId id)
    {
        const auto json = get_checked_response_json(co_await net_client_.async_http_get(
            fmt::format("/messageFromId?sessionKey={}&id={}", sess_key_, id.id)));
        co_return Event::from_json(json["data"]);
    }

    clu::task<size_t> Bot::async_count_message()
    {
        const auto json = get_checked_response_json(co_await net_client_.async_http_get(
            fmt::format("/countMessage?sessionKey={}", sess_key_)));
        co_return detail::from_json<size_t>(json["data"]);
    }

    clu::task<std::vector<Friend>> Bot::async_list_friends()
    {
        const auto json = get_checked_response_json(co_await net_client_.async_http_get(
            fmt::format("/friendList?sessionKey={}", sess_key_)));
        co_return detail::from_json<std::vector<Friend>>(json["data"]);
    }

    clu::task<std::vector<Group>> Bot::async_list_groups()
    {
        const auto json = get_checked_response_json(co_await net_client_.async_http_get(
            fmt::format("/groupList?sessionKey={}", sess_key_)));
        co_return detail::from_json<std::vector<Group>>(json["data"]);
    }

    clu::task<std::vector<Member>> Bot::async_list_members(const GroupId id)
    {
        const auto json = get_checked_response_json(co_await net_client_.async_http_get(
            fmt::format("/memberList?sessionKey={}&target={}", sess_key_, id.id)));
        co_return detail::from_json<std::vector<Member>>(json["data"]);
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
        (void)get_checked_response_json(co_await net_client_.async_http_post_json("/mute", std::move(body)));
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
        (void)get_checked_response_json(co_await net_client_.async_http_post_json("/unmute", std::move(body)));
    }

    clu::task<> Bot::async_mute_all(const GroupId group)
    {
        (void)get_checked_response_json(co_await net_client_.async_http_post_json(
            "/muteAll", group_target_body(group)));
    }

    clu::task<> Bot::async_unmute_all(const GroupId group)
    {
        (void)get_checked_response_json(co_await net_client_.async_http_post_json(
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
        (void)get_checked_response_json(co_await net_client_.async_http_post_json("/kick", std::move(body)));
    }

    clu::task<> Bot::async_quit(const GroupId group)
    {
        (void)get_checked_response_json(co_await net_client_.async_http_post_json(
            "/quit", group_target_body(group)));
    }

    clu::task<> Bot::async_respond(
        const NewFriendRequestEvent& ev, const NewFriendResponseType type, const std::string_view reason)
    {
        auto body = detail::perform_format([&](fmt::format_context& ctx)
        {
            detail::JsonObjScope scope(ctx);
            scope.add_entry("sessionKey", sess_key_);
            scope.add_entry("eventId", ev.id);
            scope.add_entry("fromId", ev.from_id.id);
            scope.add_entry("groupId", ev.group_id.id);
            scope.add_entry("operate", static_cast<uint64_t>(type));
            scope.add_entry("message", reason);
        });
        (void)get_checked_response_json(
            co_await net_client_.async_http_post_json("/resp/newFriendRequestEvent", std::move(body)));
    }

    clu::task<> Bot::async_respond(
        const MemberJoinRequestEvent& ev, const MemberJoinResponseType type, const std::string_view reason)
    {
        auto body = detail::perform_format([&](fmt::format_context& ctx)
        {
            detail::JsonObjScope scope(ctx);
            scope.add_entry("sessionKey", sess_key_);
            scope.add_entry("eventId", ev.id);
            scope.add_entry("fromId", ev.from_id.id);
            scope.add_entry("groupId", ev.group_id.id);
            scope.add_entry("operate", static_cast<uint64_t>(type));
            scope.add_entry("message", reason);
        });
        (void)get_checked_response_json(
            co_await net_client_.async_http_post_json("/resp/memberJoinRequestEvent", std::move(body)));
    }

    clu::task<> Bot::async_respond(
        const BotInvitedJoinGroupRequestEvent& ev, const BotInvitedJoinGroupResponseType type, const std::string_view reason)
    {
        auto body = detail::perform_format([&](fmt::format_context& ctx)
        {
            detail::JsonObjScope scope(ctx);
            scope.add_entry("sessionKey", sess_key_);
            scope.add_entry("eventId", ev.id);
            scope.add_entry("fromId", ev.from_id.id);
            scope.add_entry("groupId", ev.group_id.id);
            scope.add_entry("operate", static_cast<uint64_t>(type));
            scope.add_entry("message", reason);
        });
        (void)get_checked_response_json(
            co_await net_client_.async_http_post_json("/resp/botInvitedJoinGroupRequestEvent", std::move(body)));
    }

    clu::task<> Bot::async_monitor_events(const clu::function_ref<clu::task<bool>(const Event&)> callback)
    {
        net::WebsocketSession ws = net_client_.new_websocket_session();
        co_await net_client_.async_connect_websocket(ws, fmt::format("/all?sessionKey={}", sess_key_));

        std::atomic_bool close{ false };
        clu::coroutine_scope scope;

        while (!close.load(std::memory_order_acquire))
        {
            try
            {
                auto json = parser.parse(co_await ws.async_read());
                if (close.load(std::memory_order_acquire)) break;
                check_json(json);
                scope.spawn([&](const Event ev) -> clu::task<>
                {
                    try
                    {
                        if (co_await pm_queue_.async_match_event(ev)) co_return;
                        const bool result = co_await callback(ev);
                        close.store(result, std::memory_order_release);
                    }
                    catch (...) { log_exception(); }
                }(parse_event(json.value())));
            }
            catch (...) { log_exception(); }
        }

        co_await scope.join();
        co_await ws.async_close();
    }

    clu::task<> Bot::async_config(const clu::optional_param<int32_t> cache_size, const clu::optional_param<bool> enable_websocket)
    {
        auto body = detail::perform_format([&](fmt::format_context& ctx)
        {
            detail::JsonObjScope scope(ctx);
            scope.add_entry("sessionKey", sess_key_);
            if (cache_size) scope.add_entry("cacheSize", *cache_size);
            if (enable_websocket) scope.add_entry("enableWebsocket", *enable_websocket);
        });
        (void)get_checked_response_json(co_await net_client_.async_http_post_json("/config", std::move(body)));
    }

    clu::task<> Bot::async_config(const SessionConfig config) { return async_config(config.cache_size, config.enable_websocket); }
}
