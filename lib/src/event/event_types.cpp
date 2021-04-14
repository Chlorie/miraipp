#include "mirai/event/event_types.h"

#include <clu/hash.h>

#include "mirai/core/bot.h"
#include "../detail/json.h"

namespace mpp
{
    using namespace clu::literals;

    namespace
    {
        UserId get_uid(const detail::JsonRes json) { return UserId(json.get_int64()); }
        GroupId get_gid(const detail::JsonRes json) { return GroupId(json.get_int64()); }
        MessageId get_mid(const detail::JsonRes json) { return MessageId(detail::from_json<int32_t>(json)); }
    }

    MessageId FriendMessageEvent::send_message(
        const Message& message, const clu::optional_param<MessageId> quote) const
    {
        return bot().send_message(sender.id, message, quote);
    }

    ex::task<MessageId> FriendMessageEvent::send_message_async(
        const Message& message, const clu::optional_param<MessageId> quote) const
    {
        return bot().send_message_async(sender.id, message, quote);
    }

    FriendMessageEvent FriendMessageEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            MessageEventBase::from_json(json),
            Friend::from_json(json["sender"])
        };
    }

    MessageId GroupMessageEvent::send_message(
        const Message& message, const clu::optional_param<MessageId> quote) const
    {
        return bot().send_message(sender.group.id, message, quote);
    }

    ex::task<MessageId> GroupMessageEvent::send_message_async(
        const Message& message, const clu::optional_param<MessageId> quote) const
    {
        return bot().send_message_async(sender.group.id, message, quote);
    }

    void GroupMessageEvent::recall() const { bot().recall(msg.source.id); }
    ex::task<void> GroupMessageEvent::recall_async() const { return bot().recall_async(msg.source.id); }

    void GroupMessageEvent::mute_sender(const std::chrono::seconds duration) const
    {
        bot().mute(sender.group.id, sender.id, duration);
    }

    ex::task<void> GroupMessageEvent::mute_sender_async(const std::chrono::seconds duration) const
    {
        return bot().mute_async(sender.group.id, sender.id, duration);
    }

    GroupMessageEvent GroupMessageEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            MessageEventBase::from_json(json),
            Member::from_json(json["sender"])
        };
    }

    MessageId TempMessageEvent::send_message(
        const Message& message, const clu::optional_param<MessageId> quote) const
    {
        return bot().send_message(TempId{ sender.id, sender.group.id }, message, quote);
    }

    ex::task<MessageId> TempMessageEvent::send_message_async(
        const Message& message, const clu::optional_param<MessageId> quote) const
    {
        return bot().send_message_async(TempId{ sender.id, sender.group.id }, message, quote);
    }

    TempMessageEvent TempMessageEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            MessageEventBase::from_json(json),
            Member::from_json(json["sender"])
        };
    }

    BotOnlineEvent BotOnlineEvent::from_json(const detail::JsonElem json)
    {
        const std::string_view type_sv = json["type"];
        return
        {
            .subtype = (type_sv == "BotOnlineEvent") ? SubType::login : SubType::relogin,
            .id = get_uid(json["qq"])
        };
    }

    BotOfflineEvent BotOfflineEvent::from_json(const detail::JsonElem json)
    {
        const SubType subtype = [&]
        {
            const std::string_view type_sv = json["type"];
            // @formatter:off
            switch (clu::fnv1a(type_sv))
            {
                case "BotOfflineEventActive"_fnv1a:  return SubType::active;
                case "BotOfflineEventForce"_fnv1a:   return SubType::forced;
                case "BotOfflineEventDropped"_fnv1a: return SubType::dropped;
                default: std::terminate(); // unreachable
            }
            // @formatter:on
        }();
        return
        {
            .subtype = subtype,
            .id = get_uid(json["qq"])
        };
    }

    BotGroupPermissionChangeEvent BotGroupPermissionChangeEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            GroupEventBase::from_json(json),
            permission_from_string(json["origin"]),
            permission_from_string(json["current"])
        };
    }

    BotMutedEvent BotMutedEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            ExecutorEventBase::from_json(json),
            std::chrono::seconds(json["durationSeconds"])
        };
    }

    BotUnmutedEvent BotUnmutedEvent::from_json(const detail::JsonElem json)
    {
        return { ExecutorEventBase::from_json(json) };
    }

    BotJoinGroupEvent BotJoinGroupEvent::from_json(const detail::JsonElem json)
    {
        return { GroupEventBase::from_json(json) };
    }

    BotQuitEvent BotQuitEvent::from_json(const detail::JsonElem json)
    {
        return { GroupEventBase::from_json(json) };
    }

    BotKickedEvent BotKickedEvent::from_json(const detail::JsonElem json)
    {
        return { GroupEventBase::from_json(json) };
    }

    GroupRecallEvent GroupRecallEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            GroupExecutorEventBase::from_json(json),
            get_uid(json["authorId"]),
            get_mid(json["messageId"]),
            detail::from_json<int32_t>(json["time"])
        };
    }

    MessageId FriendRecallEvent::quote_reply(const Message& message) const { return bot().send_message(sender_id, message, msg_id); }

    ex::task<MessageId> FriendRecallEvent::quote_reply_async(const Message& message) const
    {
        return bot().send_message_async(sender_id, message, msg_id);
    }

    FriendRecallEvent FriendRecallEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            .sender_id = get_uid(json["authorId"]),
            .msg_id = get_mid(json["messageId"]),
            .time = detail::from_json<int32_t>(json["time"])
        };
    }

    GroupNameChangeEvent GroupNameChangeEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            GroupExecutorEventBase::from_json(json),
            detail::from_json<std::string>(json["origin"]),
            detail::from_json<std::string>(json["current"])
        };
    }

    GroupEntranceAnnouncementChangeEvent GroupEntranceAnnouncementChangeEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            GroupExecutorEventBase::from_json(json),
            detail::from_json<std::string>(json["origin"]),
            detail::from_json<std::string>(json["current"])
        };
    }

    GroupConfigEvent GroupConfigEvent::from_json(const detail::JsonElem json)
    {
        const SubType subtype = [&]
        {
            const std::string_view type_sv = json["type"];
            // @formatter:off
            switch (clu::fnv1a(type_sv))
            {
                case "GroupMuteAllEvent"_fnv1a:            return SubType::mute_all;
                case "GroupAllowAnonymousChatEvent"_fnv1a: return SubType::anonymous_chat;
                case "GroupAllowConfessTalkEvent"_fnv1a:   return SubType::confess_talk;
                case "GroupAllowMemberInviteEvent"_fnv1a:  return SubType::member_invite;
                default: std::terminate(); // unreachable
            }
            // @formatter:on
        }();
        return
        {
            GroupExecutorEventBase::from_json(json),
            subtype,
            json["origin"].get_bool(),
            json["current"].get_bool()
        };
    }

    MemberJoinEvent MemberJoinEvent::from_json(const detail::JsonElem json)
    {
        return { MemberEventBase::from_json(json) };
    }

    MemberQuitEvent MemberQuitEvent::from_json(const detail::JsonElem json)
    {
        return { MemberEventBase::from_json(json) };
    }

    MemberKickedEvent MemberKickedEvent::from_json(const detail::JsonElem json)
    {
        return { MemberExecutorEventBase::from_json(json) };
    }

    MemberCardChangeEvent MemberCardChangeEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            MemberExecutorEventBase::from_json(json),
            detail::from_json<std::string>(json["origin"]),
            detail::from_json<std::string>(json["current"])
        };
    }

    MemberSpecialTitleChangeEvent MemberSpecialTitleChangeEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            MemberEventBase::from_json(json),
            detail::from_json<std::string>(json["origin"]),
            detail::from_json<std::string>(json["current"])
        };
    }

    MemberPermissionChangeEvent MemberPermissionChangeEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            MemberEventBase::from_json(json),
            permission_from_string(json["origin"]),
            permission_from_string(json["current"])
        };
    }

    MemberMutedEvent MemberMutedEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            MemberExecutorEventBase::from_json(json),
            std::chrono::seconds(json["durationSeconds"])
        };
    }

    MemberUnmutedEvent MemberUnmutedEvent::from_json(const detail::JsonElem json)
    {
        return { MemberExecutorEventBase::from_json(json) };
    }

    void NewFriendRequestEvent::respond(
        const ResponseType response, const std::string_view reason) const
    {
        bot().respond(*this, response, reason);
    }

    ex::task<void> NewFriendRequestEvent::respond_async(
        const ResponseType response, const std::string_view reason) const
    {
        return bot().respond_async(*this, response, reason);
    }

    NewFriendRequestEvent NewFriendRequestEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            .id = json["eventId"].get_int64(),
            .from_id = get_uid(json["fromId"]),
            .group_id = get_gid(json["groupId"]),
            .name = detail::from_json<std::string>(json["nick"]),
            .message = detail::from_json<std::string>(json["message"])
        };
    }

    void MemberJoinRequestEvent::respond(
        const ResponseType response, const std::string_view reason) const
    {
        bot().respond(*this, response, reason);
    }

    ex::task<void> MemberJoinRequestEvent::respond_async(
        const ResponseType response, const std::string_view reason) const
    {
        return bot().respond_async(*this, response, reason);
    }

    MemberJoinRequestEvent MemberJoinRequestEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            .id = json["eventId"].get_int64(),
            .from_id = get_uid(json["fromId"]),
            .group_id = get_gid(json["groupId"]),
            .group_name = detail::from_json<std::string>(json["groupName"]),
            .name = detail::from_json<std::string>(json["nick"]),
            .message = detail::from_json<std::string>(json["message"])
        };
    }

    void BotInvitedJoinGroupRequestEvent::respond(
        const ResponseType response, const std::string_view reason) const
    {
        bot().respond(*this, response, reason);
    }

    ex::task<void> BotInvitedJoinGroupRequestEvent::respond_async(
        const ResponseType response, const std::string_view reason) const
    {
        return bot().respond_async(*this, response, reason);
    }

    BotInvitedJoinGroupRequestEvent BotInvitedJoinGroupRequestEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            .id = json["eventId"].get_int64(),
            .from_id = get_uid(json["fromId"]),
            .group_id = get_gid(json["groupId"]),
            .group_name = detail::from_json<std::string>(json["groupName"]),
            .name = detail::from_json<std::string>(json["nick"]),
            .message = detail::from_json<std::string>(json["message"])
        };
    }
}
