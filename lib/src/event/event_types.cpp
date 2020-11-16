#include "mirai/event/event_types.h"

#include <clu/hash.h>

#include "mirai/core/bot.h"

namespace mpp
{
    using namespace clu::literals;

    namespace
    {
        UserId get_uid(const detail::JsonRes json) { return UserId(static_cast<int64_t>(json)); }
        GroupId get_gid(const detail::JsonRes json) { return GroupId(static_cast<int64_t>(json)); }
        MessageId get_mid(const detail::JsonRes json) { return MessageId(detail::from_json(json)); }
    }

    clu::task<MessageId> FriendMessageEvent::async_send_message(
        const Message& message, const clu::optional_param<MessageId> quote) const
    {
        return bot().async_send_message(sender.id, message, quote);
    }

    FriendMessageEvent FriendMessageEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            .msg = detail::from_json(json["messageChain"]),
            .sender = detail::from_json(json["sender"])
        };
    }

    clu::task<MessageId> GroupMessageEvent::async_send_message(
        const Message& message, const clu::optional_param<MessageId> quote) const
    {
        return bot().async_send_message(sender.group.id, message, quote);
    }

    clu::task<> GroupMessageEvent::async_recall() const
    {
        return bot().async_recall(msg.source.id);
    }

    clu::task<> GroupMessageEvent::async_mute_sender(const std::chrono::seconds duration) const
    {
        return bot().async_mute(sender.group.id, sender.id, duration);
    }

    GroupMessageEvent GroupMessageEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            .msg = detail::from_json(json["messageChain"]),
            .sender = detail::from_json(json["sender"])
        };
    }

    clu::task<MessageId> TempMessageEvent::async_send_message(
        const Message& message, const clu::optional_param<MessageId> quote) const
    {
        return bot().async_send_message(TempId{ sender.id, sender.group.id }, message, quote);
    }

    TempMessageEvent TempMessageEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            .msg = detail::from_json(json["messageChain"]),
            .sender = detail::from_json(json["sender"])
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
            detail::from_json(json["time"])
        };
    }

    clu::task<MessageId> FriendRecallEvent::async_quote_reply(const Message& message) const
    {
        return bot().async_send_message(sender_id, message, msg_id);
    }

    FriendRecallEvent FriendRecallEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            .sender_id = get_uid(json["authorId"]),
            .msg_id = get_mid(json["messageId"]),
            .time = detail::from_json(json["time"])
        };
    }

    GroupNameChangeEvent GroupNameChangeEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            GroupExecutorEventBase::from_json(json),
            detail::from_json(json["origin"]),
            detail::from_json(json["current"])
        };
    }

    GroupEntranceAnnouncementChangeEvent GroupEntranceAnnouncementChangeEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            GroupExecutorEventBase::from_json(json),
            detail::from_json(json["origin"]),
            detail::from_json(json["current"])
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
            detail::from_json(json["origin"]),
            detail::from_json(json["current"])
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
            detail::from_json(json["origin"]),
            detail::from_json(json["current"])
        };
    }

    MemberSpecialTitleChangeEvent MemberSpecialTitleChangeEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            MemberEventBase::from_json(json),
            detail::from_json(json["origin"]),
            detail::from_json(json["current"])
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

    NewFriendRequestEvent NewFriendRequestEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            .id = detail::from_json(json["eventId"]),
            .from_id = get_uid(json["fromId"]),
            .group_id = get_gid(json["groupId"]),
            .name = detail::from_json(json["nick"]),
            .message = detail::from_json(json["message"])
        };
    }

    MemberJoinRequestEvent MemberJoinRequestEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            .id = detail::from_json(json["eventId"]),
            .from_id = get_uid(json["fromId"]),
            .group_id = get_gid(json["groupId"]),
            .group_name = detail::from_json(json["groupName"]),
            .name = detail::from_json(json["nick"]),
            .message = detail::from_json(json["message"])
        };
    }

    BotInvitedJoinGroupRequestEvent BotInvitedJoinGroupRequestEvent::from_json(const detail::JsonElem json)
    {
        return
        {
            .id = detail::from_json(json["eventId"]),
            .from_id = get_uid(json["fromId"]),
            .group_id = get_gid(json["groupId"]),
            .group_name = detail::from_json(json["groupName"]),
            .name = detail::from_json(json["nick"]),
            .message = detail::from_json(json["message"])
        };
    }
}
