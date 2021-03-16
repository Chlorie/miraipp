#include "mirai/core/bot.h"

#include <simdjson.h>
#include <clu/coroutine/coroutine_scope.h>

#include "mirai/core/exceptions.h"
#include "mirai/message/message.h"
#include "mirai/detail/multipart_builder.h"
#include "mirai/event/event_types.h"

namespace mpp
{
    // Utilities
    namespace
    {
        thread_local simdjson::dom::parser parser;

        void check_json(const detail::JsonRes json)
        {
            const auto code = json["code"];
            using enum simdjson::error_code;
            if (code.error() != NO_SUCH_FIELD && code.error() != INCORRECT_TYPE)
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

    // Request body
    namespace
    {
        std::string release_body(const Bot* bot)
        {
            return fmt::format(R"({{"sessionKey":{},"qq":{}}})",
                detail::JsonQuoted{ bot->session_key() }, bot->id().id);
        }

        std::string send_message_body(const Bot* bot,
            const int64_t id, const Message& message, const clu::optional_param<MessageId> quote)
        {
            return detail::perform_format([&](fmt::format_context& ctx)
            {
                detail::JsonObjScope scope(ctx);
                scope.add_entry("sessionKey", bot->session_key());
                scope.add_entry("target", id);
                scope.add_entry("messageChain", message);
                if (quote) scope.add_entry("quote", quote->id);
            });
        }

        std::string send_message_body(const Bot* bot,
            const TempId id, const Message& message, const clu::optional_param<MessageId> quote)
        {
            return detail::perform_format([&](fmt::format_context& ctx)
            {
                detail::JsonObjScope scope(ctx);
                scope.add_entry("sessionKey", bot->session_key());
                scope.add_entry("qq", id.uid.id);
                scope.add_entry("group", id.gid.id);
                scope.add_entry("messageChain", message);
                if (quote) scope.add_entry("quote", quote->id);
            });
        }

        template <typename Id>
        std::string target_id_body(const Bot* bot, const Id id)
        {
            return detail::perform_format([&](fmt::format_context& ctx)
            {
                detail::JsonObjScope scope(ctx);
                scope.add_entry("sessionKey", bot->session_key());
                scope.add_entry("target", id.id);
            });
        }

        std::string send_image_message_body(const Bot* bot, const UserId id, const std::span<const std::string> urls)
        {
            return detail::perform_format([&](fmt::format_context& ctx)
            {
                detail::JsonObjScope scope(ctx);
                scope.add_entry("sessionKey", bot->session_key());
                scope.add_entry("qq", id.id);
                scope.add_entry("urls", urls);
            });
        }

        std::string send_image_message_body(const Bot* bot, const GroupId id, const std::span<const std::string> urls)
        {
            return detail::perform_format([&](fmt::format_context& ctx)
            {
                detail::JsonObjScope scope(ctx);
                scope.add_entry("sessionKey", bot->session_key());
                scope.add_entry("group", id.id);
                scope.add_entry("urls", urls);
            });
        }

        std::string send_image_message_body(const Bot* bot, const TempId id, const std::span<const std::string> urls)
        {
            return detail::perform_format([&](fmt::format_context& ctx)
            {
                detail::JsonObjScope scope(ctx);
                scope.add_entry("sessionKey", bot->session_key());
                scope.add_entry("qq", id.uid.id);
                scope.add_entry("group", id.gid.id);
                scope.add_entry("urls", urls);
            });
        }

        std::string upload_file_body(const Bot* bot,
            const TargetType type, const std::string_view key, const std::filesystem::path& path)
        {
            using detail::MultipartBuilder;

            MultipartBuilder builder;
            builder.add_key_value("sessionKey", bot->session_key());
            builder.add_key_value("type", to_string_view(type));
            builder.add_file(key, path);

            return builder.take_string();
        }

        std::string config_body(const Bot* bot, const SessionConfig config)
        {
            return detail::perform_format([&](fmt::format_context& ctx)
            {
                detail::JsonObjScope scope(ctx);
                scope.add_entry("sessionKey", bot->session_key());
                if (config.cache_size) scope.add_entry("cacheSize", *config.cache_size);
                if (config.enable_websocket) scope.add_entry("enableWebsocket", *config.enable_websocket);
            });
        }
    }

    std::string Bot::check_auth_gen_body(const std::string_view auth_key) const
    {
        if (authorized()) throw std::runtime_error("一个 Bot 实例只能绑定一个 bot 账号");
        return fmt::format(R"({{"authKey":{}}})", detail::JsonQuoted{ auth_key });
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

    std::string Bot::get_version()
    {
        const auto json = get_checked_response_json(net_client_.http_get("/about"));
        return std::string(json.at_pointer("/data/version"));
    }

    clu::task<std::string> Bot::async_get_version()
    {
        const auto json = get_checked_response_json(co_await net_client_.async_http_get("/about"));
        co_return std::string(json.at_pointer("/data/version"));
    }

    void Bot::authorize(const std::string_view auth_key, const UserId id)
    {
        const auto auth_json = get_checked_response_json(
            net_client_.http_post_json("/auth", check_auth_gen_body(auth_key)));
        sess_key_ = std::string(auth_json["session"]);

        auto verify_body = fmt::format(R"({{"sessionKey":{},"qq":{}}})", detail::JsonQuoted{ sess_key_ }, id.id);
        (void)get_checked_response_json(net_client_.http_post_json("/verify", std::move(verify_body)));
        bot_id_ = id;
    }

    clu::task<void> Bot::async_authorize(const std::string_view auth_key, const UserId id)
    {
        const auto auth_json = get_checked_response_json(
            co_await net_client_.async_http_post_json("/auth", check_auth_gen_body(auth_key)));
        sess_key_ = std::string(auth_json["session"]);

        auto verify_body = fmt::format(R"({{"sessionKey":{},"qq":{}}})", detail::JsonQuoted{ sess_key_ }, id.id);
        (void)get_checked_response_json(co_await net_client_.async_http_post_json("/verify", std::move(verify_body)));
        bot_id_ = id;
    }

    void Bot::release()
    {
        (void)get_checked_response_json(
            net_client_.http_post_json("/release", release_body(this)));
        bot_id_ = {};
        sess_key_.clear();
    }

    clu::task<void> Bot::async_release()
    {
        (void)get_checked_response_json(
            co_await net_client_.async_http_post_json("/release", release_body(this)));
        bot_id_ = {};
        sess_key_.clear();
    }

    MessageId Bot::send_message(const UserId id, const Message& message, const clu::optional_param<MessageId> quote)
    {
        const auto res = get_checked_response_json(
            net_client_.http_post_json("/sendFriendMessage", send_message_body(this, id.id, message, quote)));
        return MessageId(detail::from_json<int32_t>(res["messageId"]));
    }

    MessageId Bot::send_message(const GroupId id, const Message& message, const clu::optional_param<MessageId> quote)
    {
        const auto res = get_checked_response_json(
            net_client_.http_post_json("/sendGroupMessage", send_message_body(this, id.id, message, quote)));
        return MessageId(detail::from_json<int32_t>(res["messageId"]));
    }

    MessageId Bot::send_message(const TempId id, const Message& message, const clu::optional_param<MessageId> quote)
    {
        const auto res = get_checked_response_json(
            net_client_.http_post_json("/sendTempMessage", send_message_body(this, id, message, quote)));
        return MessageId(detail::from_json<int32_t>(res["messageId"]));
    }

    clu::task<MessageId> Bot::async_send_message(
        const UserId id, const Message& message, const clu::optional_param<MessageId> quote)
    {
        const auto res = get_checked_response_json(
            co_await net_client_.async_http_post_json("/sendFriendMessage", send_message_body(this, id.id, message, quote)));
        co_return MessageId(detail::from_json<int32_t>(res["messageId"]));
    }

    clu::task<MessageId> Bot::async_send_message(
        const GroupId id, const Message& message, const clu::optional_param<MessageId> quote)
    {
        const auto res = get_checked_response_json(
            co_await net_client_.async_http_post_json("/sendGroupMessage", send_message_body(this, id.id, message, quote)));
        co_return MessageId(detail::from_json<int32_t>(res["messageId"]));
    }

    clu::task<MessageId> Bot::async_send_message(
        const TempId id, const Message& message, const clu::optional_param<MessageId> quote)
    {
        const auto res = get_checked_response_json(
            co_await net_client_.async_http_post_json("/sendTempMessage", send_message_body(this, id, message, quote)));
        co_return MessageId(detail::from_json<int32_t>(res["messageId"]));
    }

    void Bot::recall(const MessageId id)
    {
        (void)get_checked_response_json(net_client_.http_post_json(
            "/recall", target_id_body(this, id)));
    }

    clu::task<void> Bot::async_recall(const MessageId id)
    {
        (void)get_checked_response_json(co_await net_client_.async_http_post_json(
            "/recall", target_id_body(this, id)));
    }

    std::vector<std::string> Bot::send_image_message(const UserId id, const std::span<const std::string> urls)
    {
        const auto json = get_checked_response_json(net_client_.http_post_json(
            "/sendImageMessage", send_image_message_body(this, id, urls)));
        return detail::from_json<std::vector<std::string>>(json);
    }

    std::vector<std::string> Bot::send_image_message(const GroupId id, const std::span<const std::string> urls)
    {
        const auto json = get_checked_response_json(net_client_.http_post_json(
            "/sendImageMessage", send_image_message_body(this, id, urls)));
        return detail::from_json<std::vector<std::string>>(json);
    }

    std::vector<std::string> Bot::send_image_message(const TempId id, const std::span<const std::string> urls)
    {
        const auto json = get_checked_response_json(net_client_.http_post_json(
            "/sendImageMessage", send_image_message_body(this, id, urls)));
        return detail::from_json<std::vector<std::string>>(json);
    }

    clu::task<std::vector<std::string>> Bot::async_send_image_message(
        const UserId id, const std::span<const std::string> urls)
    {
        const auto json = get_checked_response_json(co_await net_client_.async_http_post_json(
            "/sendImageMessage", send_image_message_body(this, id, urls)));
        co_return detail::from_json<std::vector<std::string>>(json);
    }

    clu::task<std::vector<std::string>> Bot::async_send_image_message(
        const GroupId id, const std::span<const std::string> urls)
    {
        const auto json = get_checked_response_json(co_await net_client_.async_http_post_json(
            "/sendImageMessage", send_image_message_body(this, id, urls)));
        co_return detail::from_json<std::vector<std::string>>(json);
    }

    clu::task<std::vector<std::string>> Bot::async_send_image_message(
        const TempId id, const std::span<const std::string> urls)
    {
        const auto json = get_checked_response_json(co_await net_client_.async_http_post_json(
            "/sendImageMessage", send_image_message_body(this, id, urls)));
        co_return detail::from_json<std::vector<std::string>>(json);
    }

    Image Bot::upload_image(const TargetType type, const std::filesystem::path& path)
    {
        auto res = get_checked_response_json(net_client_.http_post(
            "/uploadImage", detail::MultipartBuilder::content_type,
            upload_file_body(this, type, "img", path)));
        return Image::from_json(res.value());
    }

    clu::task<Image> Bot::async_upload_image(const TargetType type, const std::filesystem::path& path)
    {
        auto res = get_checked_response_json(co_await net_client_.async_http_post(
            "/uploadImage", detail::MultipartBuilder::content_type,
            upload_file_body(this, type, "img", path)));
        co_return Image::from_json(res.value());
    }

    Voice Bot::upload_voice(const TargetType type, const std::filesystem::path& path)
    {
        auto res = get_checked_response_json(net_client_.http_post(
            "/uploadVoice", detail::MultipartBuilder::content_type,
            upload_file_body(this, type, "voice", path)));
        return Voice::from_json(res.value());
    }

    clu::task<Voice> Bot::async_upload_voice(const TargetType type, const std::filesystem::path& path)
    {
        auto res = get_checked_response_json(co_await net_client_.async_http_post(
            "/uploadVoice", detail::MultipartBuilder::content_type,
            upload_file_body(this, type, "voice", path)));
        co_return Voice::from_json(res.value());
    }

    std::vector<Event> Bot::pop_events(const size_t count)
    {
        const auto json = get_checked_response_json(net_client_.http_get(
            fmt::format("/fetchMessage?sessionKey={}&count={}", sess_key_, count)));
        return parse_events(json["data"]);
    }

    std::vector<Event> Bot::pop_latest_events(const size_t count)
    {
        const auto json = get_checked_response_json(net_client_.http_get(
            fmt::format("/fetchLatestMessage?sessionKey={}&count={}", sess_key_, count)));
        return parse_events(json["data"]);
    }

    std::vector<Event> Bot::peek_events(const size_t count)
    {
        const auto json = get_checked_response_json(net_client_.http_get(
            fmt::format("/peekMessage?sessionKey={}&count={}", sess_key_, count)));
        return parse_events(json["data"]);
    }

    std::vector<Event> Bot::peek_latest_events(const size_t count)
    {
        const auto json = get_checked_response_json(net_client_.http_get(
            fmt::format("/peekLatestMessage?sessionKey={}&count={}", sess_key_, count)));
        return parse_events(json["data"]);
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
        co_return detail::from_json<std::vector<Friend>>(json);
    }

    clu::task<std::vector<Group>> Bot::async_list_groups()
    {
        const auto json = get_checked_response_json(co_await net_client_.async_http_get(
            fmt::format("/groupList?sessionKey={}", sess_key_)));
        co_return detail::from_json<std::vector<Group>>(json);
    }

    clu::task<std::vector<Member>> Bot::async_list_members(const GroupId id)
    {
        const auto json = get_checked_response_json(co_await net_client_.async_http_get(
            fmt::format("/memberList?sessionKey={}&target={}", sess_key_, id.id)));
        co_return detail::from_json<std::vector<Member>>(json);
    }

    clu::task<void> Bot::async_mute(const GroupId group, const UserId user, std::chrono::seconds duration)
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

    clu::task<void> Bot::async_unmute(const GroupId group, const UserId user)
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

    clu::task<void> Bot::async_mute_all(const GroupId group)
    {
        (void)get_checked_response_json(co_await net_client_.async_http_post_json(
            "/muteAll", target_id_body(this, group)));
    }

    clu::task<void> Bot::async_unmute_all(const GroupId group)
    {
        (void)get_checked_response_json(co_await net_client_.async_http_post_json(
            "/unmuteAll", target_id_body(this, group)));
    }

    clu::task<void> Bot::async_kick(const GroupId group, const UserId user, const std::string_view reason)
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

    clu::task<void> Bot::async_quit(const GroupId group)
    {
        (void)get_checked_response_json(co_await net_client_.async_http_post_json(
            "/quit", target_id_body(this, group)));
    }

    clu::task<void> Bot::async_respond(
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

    clu::task<void> Bot::async_respond(
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

    clu::task<void> Bot::async_respond(
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

    clu::task<GroupConfig> Bot::async_get_group_config(const GroupId group)
    {
        const auto json = get_checked_response_json(co_await net_client_.async_http_get(
            fmt::format("/groupConfig?sessionKey={}&target={}", sess_key_, group.id)));
        co_return detail::from_json<GroupConfig>(json);
    }

    clu::task<void> Bot::async_config_group(const GroupId group, const GroupConfig& config)
    {
        auto body = detail::perform_format([&](fmt::format_context& ctx)
        {
            detail::JsonObjScope scope(ctx);
            scope.add_entry("sessionKey", sess_key_);
            scope.add_entry("target", group.id);
            scope.add_entry("config", config);
        });
        (void)get_checked_response_json(co_await net_client_.async_http_post_json("/groupConfig", std::move(body)));
    }

    clu::task<MemberInfo> Bot::async_get_member_info(const GroupId group, const UserId user)
    {
        const auto json = get_checked_response_json(co_await net_client_.async_http_get(
            fmt::format("/groupConfig?sessionKey={}&target={}&memberId={}", sess_key_, group.id, user.id)));
        co_return detail::from_json<MemberInfo>(json);
    }

    clu::task<void> Bot::async_set_member_info(const GroupId group, const UserId user, const MemberInfo& info)
    {
        auto body = detail::perform_format([&](fmt::format_context& ctx)
        {
            detail::JsonObjScope scope(ctx);
            scope.add_entry("sessionKey", sess_key_);
            scope.add_entry("target", group.id);
            scope.add_entry("memberId", user.id);
            scope.add_entry("info", info);
        });
        (void)get_checked_response_json(co_await net_client_.async_http_post_json("/memberInfo", std::move(body)));
    }

    void Bot::monitor_events(const clu::function_ref<bool(const Event&)> callback)
    {
        net::WebsocketSession ws = net_client_.new_websocket_session();
        net_client_.connect_websocket(ws, fmt::format("/all?sessionKey={}", sess_key_));
        clu::scope_exit _ = [&] { ws.close(); };

        while (true)
        {
            try
            {
                auto json = parser.parse(ws.read());
                check_json(json);
                if (callback(parse_event(json.value()))) break;
            }
            catch (...) { log_exception(); }
        }
    }

    clu::task<void> Bot::async_monitor_events(const clu::function_ref<clu::task<bool>(const Event&)> callback)
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
                scope.spawn([&](const Event ev) -> clu::task<void>
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

    SessionConfig Bot::get_config()
    {
        const auto json = get_checked_response_json(net_client_.http_get(
            fmt::format("/config?sessionKey={}", sess_key_)));
        return detail::from_json<SessionConfig>(json);
    }

    clu::task<SessionConfig> Bot::async_get_config()
    {
        const auto json = get_checked_response_json(co_await net_client_.async_http_get(
            fmt::format("/config?sessionKey={}", sess_key_)));
        co_return detail::from_json<SessionConfig>(json);
    }

    void Bot::config(const SessionConfig config)
    {
        (void)get_checked_response_json(net_client_.http_post_json(
            "/config", config_body(this, config)));
    }

    clu::task<void> Bot::async_config(const SessionConfig config)
    {
        (void)get_checked_response_json(co_await net_client_.async_http_post_json(
            "/config", config_body(this, config)));
    }
}
