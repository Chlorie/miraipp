#include "mirai/core/bot.h"

#include <thread>
#include <simdjson.h>
#include <clu/scope.h>
#include <unifex/async_scope.hpp>
#include <unifex/inline_scheduler.hpp>
#include <unifex/on.hpp>

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

    ex::task<std::string> Bot::get_version_async()
    {
        const auto json = get_checked_response_json(co_await net_client_.http_get_async("/about"));
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

    ex::task<void> Bot::authorize_async(const std::string_view auth_key, const UserId id)
    {
        const auto auth_json = get_checked_response_json(
            co_await net_client_.http_post_json_async("/auth", check_auth_gen_body(auth_key)));
        sess_key_ = std::string(auth_json["session"]);

        auto verify_body = fmt::format(R"({{"sessionKey":{},"qq":{}}})", detail::JsonQuoted{ sess_key_ }, id.id);
        (void)get_checked_response_json(co_await net_client_.http_post_json_async("/verify", std::move(verify_body)));
        bot_id_ = id;
    }

    void Bot::release()
    {
        (void)get_checked_response_json(
            net_client_.http_post_json("/release", release_body(this)));
        bot_id_ = {};
        sess_key_.clear();
    }

    ex::task<void> Bot::release_async()
    {
        (void)get_checked_response_json(
            co_await net_client_.http_post_json_async("/release", release_body(this)));
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

    ex::task<MessageId> Bot::send_message_async(
        const UserId id, const Message& message, const clu::optional_param<MessageId> quote)
    {
        const auto res = get_checked_response_json(
            co_await net_client_.http_post_json_async("/sendFriendMessage", send_message_body(this, id.id, message, quote)));
        co_return MessageId(detail::from_json<int32_t>(res["messageId"]));
    }

    ex::task<MessageId> Bot::send_message_async(
        const GroupId id, const Message& message, const clu::optional_param<MessageId> quote)
    {
        const auto res = get_checked_response_json(
            co_await net_client_.http_post_json_async("/sendGroupMessage", send_message_body(this, id.id, message, quote)));
        co_return MessageId(detail::from_json<int32_t>(res["messageId"]));
    }

    ex::task<MessageId> Bot::send_message_async(
        const TempId id, const Message& message, const clu::optional_param<MessageId> quote)
    {
        const auto res = get_checked_response_json(
            co_await net_client_.http_post_json_async("/sendTempMessage", send_message_body(this, id, message, quote)));
        co_return MessageId(detail::from_json<int32_t>(res["messageId"]));
    }

    void Bot::recall(const MessageId id)
    {
        (void)get_checked_response_json(net_client_.http_post_json(
            "/recall", target_id_body(this, id)));
    }

    ex::task<void> Bot::recall_async(const MessageId id)
    {
        (void)get_checked_response_json(co_await net_client_.http_post_json_async(
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

    ex::task<std::vector<std::string>> Bot::send_image_message_async(
        const UserId id, const std::span<const std::string> urls)
    {
        const auto json = get_checked_response_json(co_await net_client_.http_post_json_async(
            "/sendImageMessage", send_image_message_body(this, id, urls)));
        co_return detail::from_json<std::vector<std::string>>(json);
    }

    ex::task<std::vector<std::string>> Bot::send_image_message_async(
        const GroupId id, const std::span<const std::string> urls)
    {
        const auto json = get_checked_response_json(co_await net_client_.http_post_json_async(
            "/sendImageMessage", send_image_message_body(this, id, urls)));
        co_return detail::from_json<std::vector<std::string>>(json);
    }

    ex::task<std::vector<std::string>> Bot::send_image_message_async(
        const TempId id, const std::span<const std::string> urls)
    {
        const auto json = get_checked_response_json(co_await net_client_.http_post_json_async(
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

    ex::task<Image> Bot::upload_image_async(const TargetType type, const std::filesystem::path& path)
    {
        auto res = get_checked_response_json(co_await net_client_.http_post_async(
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

    ex::task<Voice> Bot::upload_voice_async(const TargetType type, const std::filesystem::path& path)
    {
        auto res = get_checked_response_json(co_await net_client_.http_post_async(
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

    ex::task<std::vector<Event>> Bot::pop_events_async(const size_t count)
    {
        const auto json = get_checked_response_json(co_await net_client_.http_get_async(
            fmt::format("/fetchMessage?sessionKey={}&count={}", sess_key_, count)));
        co_return parse_events(json["data"]);
    }

    ex::task<std::vector<Event>> Bot::pop_latest_events_async(const size_t count)
    {
        const auto json = get_checked_response_json(co_await net_client_.http_get_async(
            fmt::format("/fetchLatestMessage?sessionKey={}&count={}", sess_key_, count)));
        co_return parse_events(json["data"]);
    }

    ex::task<std::vector<Event>> Bot::peek_events_async(const size_t count)
    {
        const auto json = get_checked_response_json(co_await net_client_.http_get_async(
            fmt::format("/peekMessage?sessionKey={}&count={}", sess_key_, count)));
        co_return parse_events(json["data"]);
    }

    ex::task<std::vector<Event>> Bot::peek_latest_events_async(const size_t count)
    {
        const auto json = get_checked_response_json(co_await net_client_.http_get_async(
            fmt::format("/peekLatestMessage?sessionKey={}&count={}", sess_key_, count)));
        co_return parse_events(json["data"]);
    }

    ex::task<Event> Bot::retrieve_message_async(const MessageId id)
    {
        const auto json = get_checked_response_json(co_await net_client_.http_get_async(
            fmt::format("/messageFromId?sessionKey={}&id={}", sess_key_, id.id)));
        co_return Event::from_json(json["data"]);
    }

    ex::task<size_t> Bot::count_message_async()
    {
        const auto json = get_checked_response_json(co_await net_client_.http_get_async(
            fmt::format("/countMessage?sessionKey={}", sess_key_)));
        co_return detail::from_json<size_t>(json["data"]);
    }

    ex::task<std::vector<Friend>> Bot::list_friends_async()
    {
        const auto json = get_checked_response_json(co_await net_client_.http_get_async(
            fmt::format("/friendList?sessionKey={}", sess_key_)));
        co_return detail::from_json<std::vector<Friend>>(json);
    }

    ex::task<std::vector<Group>> Bot::list_groups_async()
    {
        const auto json = get_checked_response_json(co_await net_client_.http_get_async(
            fmt::format("/groupList?sessionKey={}", sess_key_)));
        co_return detail::from_json<std::vector<Group>>(json);
    }

    ex::task<std::vector<Member>> Bot::list_members_async(const GroupId id)
    {
        const auto json = get_checked_response_json(co_await net_client_.http_get_async(
            fmt::format("/memberList?sessionKey={}&target={}", sess_key_, id.id)));
        co_return detail::from_json<std::vector<Member>>(json);
    }

    ex::task<void> Bot::mute_async(const GroupId group, const UserId user, std::chrono::seconds duration)
    {
        auto body = detail::perform_format([&](fmt::format_context& ctx)
        {
            detail::JsonObjScope scope(ctx);
            scope.add_entry("sessionKey", sess_key_);
            scope.add_entry("target", group.id);
            scope.add_entry("memberId", user.id);
            scope.add_entry("time", duration.count());
        });
        (void)get_checked_response_json(co_await net_client_.http_post_json_async("/mute", std::move(body)));
    }

    ex::task<void> Bot::unmute_async(const GroupId group, const UserId user)
    {
        auto body = detail::perform_format([&](fmt::format_context& ctx)
        {
            detail::JsonObjScope scope(ctx);
            scope.add_entry("sessionKey", sess_key_);
            scope.add_entry("target", group.id);
            scope.add_entry("memberId", user.id);
        });
        (void)get_checked_response_json(co_await net_client_.http_post_json_async("/unmute", std::move(body)));
    }

    ex::task<void> Bot::mute_all_async(const GroupId group)
    {
        (void)get_checked_response_json(co_await net_client_.http_post_json_async(
            "/muteAll", target_id_body(this, group)));
    }

    ex::task<void> Bot::unmute_all_async(const GroupId group)
    {
        (void)get_checked_response_json(co_await net_client_.http_post_json_async(
            "/unmuteAll", target_id_body(this, group)));
    }

    ex::task<void> Bot::kick_async(const GroupId group, const UserId user, const std::string_view reason)
    {
        auto body = detail::perform_format([&](fmt::format_context& ctx)
        {
            detail::JsonObjScope scope(ctx);
            scope.add_entry("sessionKey", sess_key_);
            scope.add_entry("target", group.id);
            scope.add_entry("memberId", user.id);
            scope.add_entry("msg", reason);
        });
        (void)get_checked_response_json(co_await net_client_.http_post_json_async("/kick", std::move(body)));
    }

    ex::task<void> Bot::quit_async(const GroupId group)
    {
        (void)get_checked_response_json(co_await net_client_.http_post_json_async(
            "/quit", target_id_body(this, group)));
    }

    ex::task<void> Bot::respond_async(
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
            co_await net_client_.http_post_json_async("/resp/newFriendRequestEvent", std::move(body)));
    }

    ex::task<void> Bot::respond_async(
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
            co_await net_client_.http_post_json_async("/resp/memberJoinRequestEvent", std::move(body)));
    }

    ex::task<void> Bot::respond_async(
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
            co_await net_client_.http_post_json_async("/resp/botInvitedJoinGroupRequestEvent", std::move(body)));
    }

    ex::task<GroupConfig> Bot::get_group_config_async(const GroupId group)
    {
        const auto json = get_checked_response_json(co_await net_client_.http_get_async(
            fmt::format("/groupConfig?sessionKey={}&target={}", sess_key_, group.id)));
        co_return detail::from_json<GroupConfig>(json);
    }

    ex::task<void> Bot::config_group_async(const GroupId group, const GroupConfig& config)
    {
        auto body = detail::perform_format([&](fmt::format_context& ctx)
        {
            detail::JsonObjScope scope(ctx);
            scope.add_entry("sessionKey", sess_key_);
            scope.add_entry("target", group.id);
            scope.add_entry("config", config);
        });
        (void)get_checked_response_json(co_await net_client_.http_post_json_async("/groupConfig", std::move(body)));
    }

    ex::task<MemberInfo> Bot::get_member_info_async(const GroupId group, const UserId user)
    {
        const auto json = get_checked_response_json(co_await net_client_.http_get_async(
            fmt::format("/groupConfig?sessionKey={}&target={}&memberId={}", sess_key_, group.id, user.id)));
        co_return detail::from_json<MemberInfo>(json);
    }

    ex::task<void> Bot::set_member_info_async(const GroupId group, const UserId user, const MemberInfo& info)
    {
        auto body = detail::perform_format([&](fmt::format_context& ctx)
        {
            detail::JsonObjScope scope(ctx);
            scope.add_entry("sessionKey", sess_key_);
            scope.add_entry("target", group.id);
            scope.add_entry("memberId", user.id);
            scope.add_entry("info", info);
        });
        (void)get_checked_response_json(co_await net_client_.http_post_json_async("/memberInfo", std::move(body)));
    }

    void Bot::monitor_events(const clu::function_ref<bool(const Event&)> callback)
    {
        net::WebsocketSession ws = net_client_.new_websocket_session();
        net_client_.connect_websocket(ws, fmt::format("/all?sessionKey={}", sess_key_));
        clu::scope_exit _([&] { ws.close(); });

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

    ex::task<void> Bot::monitor_events_async(const clu::function_ref<ex::task<bool>(const Event&)> callback)
    {
        net::WebsocketSession ws = net_client_.new_websocket_session();
        co_await net_client_.connect_websocket_async(ws, fmt::format("/all?sessionKey={}", sess_key_));

        std::atomic_bool close{ false };
        ex::inline_scheduler sch;
        ex::async_scope scope;

        while (!close.load(std::memory_order_acquire))
        {
            try
            {
                auto json = parser.parse(co_await ws.read_async());
                if (close.load(std::memory_order_acquire)) break;
                check_json(json);
                scope.spawn([&](const Event ev) -> ex::task<void>
                {
                    try
                    {
                        // if (co_await pm_queue_.match_event_async(ev)) co_return;
                        const bool result = co_await callback(ev);
                        close.store(result, std::memory_order_release);
                    }
                    catch (...) { log_exception(); }
                }(parse_event(json.value())), sch);
            }
            catch (...) { log_exception(); }
        }

        co_await ex::on(scope.cleanup(), sch);
        co_await ws.close_async();
    }

    SessionConfig Bot::get_config()
    {
        const auto json = get_checked_response_json(net_client_.http_get(
            fmt::format("/config?sessionKey={}", sess_key_)));
        return detail::from_json<SessionConfig>(json);
    }

    ex::task<SessionConfig> Bot::get_config_async()
    {
        const auto json = get_checked_response_json(co_await net_client_.http_get_async(
            fmt::format("/config?sessionKey={}", sess_key_)));
        co_return detail::from_json<SessionConfig>(json);
    }

    void Bot::config(const SessionConfig config)
    {
        (void)get_checked_response_json(net_client_.http_post_json(
            "/config", config_body(this, config)));
    }

    ex::task<void> Bot::config_async(const SessionConfig config)
    {
        (void)get_checked_response_json(co_await net_client_.http_post_json_async(
            "/config", config_body(this, config)));
    }

    void launch_async_bot(const clu::function_ref<ex::task<void>(Bot&)> task, const size_t thread_count,
        const std::string_view host, const std::string_view port)
    {
        if (thread_count == 0) throw std::runtime_error("At least one thread");

        Bot bot(host, port);
        ex::async_scope scope;
        ex::inline_scheduler scheduler;
        scope.spawn(task(bot), scheduler);
        bot.run();

        std::vector<std::jthread> threads;
        threads.reserve(thread_count - 1);
        for (size_t i = 1; i < thread_count; i++)
            threads.emplace_back([&] { bot.run(); });
    }
}
