#pragma once

#include "event_bases.h"

namespace mpp
{
    /// 事件类型枚举
    enum class EventType : uint8_t
    {
        friend_message, group_message, temp_message,
        bot_online, bot_offline,
        bot_group_permission_change, bot_muted, bot_unmuted, bot_join_group, bot_quit, bot_kicked,
        group_recall, friend_recall,
        group_name_change, group_entrance_announcement_change, group_config,
        member_join, member_quit, member_kicked, member_card_change, member_special_title_change,
        member_permission_change, member_muted, member_unmuted,
        new_friend_request, member_join_request, bot_invited_join_group_request
    };

    // @formatter:off
    /// 事件组成类型概念
    template <typename T>
    concept EventComponent = std::derived_from<T, EventBase>
        && requires { {T::type} -> std::convertible_to<EventType>; };
    // @formatter:on

    /// 私聊消息事件
    struct FriendMessageEvent final : EventBase
    {
        static constexpr EventType type = EventType::friend_message;

        SentMessage msg; ///< 收到的消息
        Friend sender; ///< 消息的发送者

        /**
         * \brief 异步地向消息发送者发送消息
         * \param message 消息内容
         * \param quote （可选）要引用回复的消息 id
         * \return （异步）已发送消息的 id，用于撤回和引用回复
         */
        clu::task<MessageId> async_send_message(const Message& message, clu::optional_param<MessageId> quote = {}) const;

        /**
         * \brief 异步地引用回复本消息
         * \param message 消息内容
         * \return （异步）已发送消息的 id，用于撤回和引用回复
         */
        clu::task<MessageId> async_quote_reply(const Message& message) const { return async_send_message(message, msg.source.id); }

        static FriendMessageEvent from_json(detail::JsonElem json);
    };

    /// 群聊消息事件
    struct GroupMessageEvent final : EventBase
    {
        static constexpr EventType type = EventType::group_message;

        SentMessage msg; ///< 收到的消息
        Member sender; ///< 消息的发送者

        At at_sender() const { return At{ sender.id }; } ///< 获取对象为消息发送者的 at 消息段

        /**
         * \brief 异步地向消息发送者发送消息
         * \param message 消息内容
         * \param quote （可选）要引用回复的消息 id
         * \return （异步）已发送消息的 id，用于撤回和引用回复
         */
        clu::task<MessageId> async_send_message(const Message& message, clu::optional_param<MessageId> quote = {}) const;

        /**
         * \brief 异步地引用回复本消息
         * \param message 消息内容
         * \return （异步）已发送消息的 id，用于撤回和引用回复
         */
        clu::task<MessageId> async_quote_reply(const Message& message) const { return async_send_message(message, msg.source.id); }

        clu::task<> async_recall() const; ///< 异步地撤回该消息

        /**
         * \brief 异步地禁言该消息发送者
         * \param duration 禁言时长
         */
        clu::task<> async_mute_sender(std::chrono::seconds duration) const;

        static GroupMessageEvent from_json(detail::JsonElem json);
    };

    /// 临时会话消息事件
    struct TempMessageEvent final : EventBase
    {
        static constexpr EventType type = EventType::temp_message;

        SentMessage msg; ///< 收到的消息
        Member sender; ///< 消息的发送者

        /**
         * \brief 异步地向消息发送者发送消息
         * \param message 消息内容
         * \param quote （可选）要引用回复的消息 id
         * \return （异步）已发送消息的 id，用于撤回和引用回复
         */
        clu::task<MessageId> async_send_message(const Message& message, clu::optional_param<MessageId> quote = {}) const;

        /**
         * \brief 异步地引用回复本消息
         * \param message 消息内容
         * \return （异步）已发送消息的 id，用于撤回和引用回复
         */
        clu::task<MessageId> async_quote_reply(const Message& message) const { return async_send_message(message, msg.source.id); }

        static TempMessageEvent from_json(detail::JsonElem json);
    };

    /// Bot 上线事件
    struct BotOnlineEvent final : EventBase
    {
        static constexpr EventType type = EventType::bot_online;

        enum class SubType : uint8_t { login, relogin }; ///< 事件的子类型枚举
        SubType subtype; ///< 事件的子类型
        UserId id; ///< 上线 bot 的 QQ 号

        static BotOnlineEvent from_json(detail::JsonElem json);
    };

    /// Bot 下线事件
    struct BotOfflineEvent final : EventBase
    {
        static constexpr EventType type = EventType::bot_offline;

        enum class SubType : uint8_t { active, forced, dropped }; ///< 事件的子类型枚举
        SubType subtype{}; ///< 事件的子类型
        UserId id; ///< 下线 bot 的 QQ 号

        static BotOfflineEvent from_json(detail::JsonElem json);
    };

    /// Bot 群权限变动事件
    struct BotGroupPermissionChangeEvent final : GroupEventBase
    {
        static constexpr EventType type = EventType::bot_group_permission_change;

        Permission original{}; ///< 原权限
        Permission current{}; ///< 现权限

        static BotGroupPermissionChangeEvent from_json(detail::JsonElem json);
    };

    /// Bot 被禁言事件
    struct BotMutedEvent final : ExecutorEventBase
    {
        static constexpr EventType type = EventType::bot_muted;

        std::chrono::seconds duration{}; ///< 被禁言的时长

        static BotMutedEvent from_json(detail::JsonElem json);
    };

    /// Bot 被解禁事件
    struct BotUnmutedEvent final : ExecutorEventBase
    {
        static constexpr EventType type = EventType::bot_unmuted;

        static BotUnmutedEvent from_json(detail::JsonElem json);
    };

    /// Bot 加入新群事件
    struct BotJoinGroupEvent final : GroupEventBase
    {
        static constexpr EventType type = EventType::bot_join_group;

        static BotJoinGroupEvent from_json(detail::JsonElem json);
    };

    /// Bot 退群事件
    struct BotQuitEvent final : GroupEventBase
    {
        static constexpr EventType type = EventType::bot_quit;

        static BotQuitEvent from_json(detail::JsonElem json);
    };

    /// Bot 被踢出群事件
    struct BotKickedEvent final : GroupEventBase
    {
        static constexpr EventType type = EventType::bot_kicked;

        static BotKickedEvent from_json(detail::JsonElem json);
    };

    /// 群消息被撤回事件
    struct GroupRecallEvent final : GroupExecutorEventBase
    {
        static constexpr EventType type = EventType::group_recall;

        UserId sender_id; ///< 被撤回消息的发送者 QQ 号
        MessageId msg_id; ///< 被撤回消息的 id
        int32_t time{}; ///< 被撤回消息发送的时间

        At at_sender() const { return At{ sender_id }; } ///< 获取对象为被撤回消息的发送者的 at 消息段

        /**
         * \brief 异步地引用回复被撤回的消息
         * \param message 消息内容
         * \return （异步）已发送消息的 id，用于撤回和引用回复
         */
        clu::task<MessageId> async_quote_reply(const Message& message) const { return async_send_message(message, msg_id); }

        static GroupRecallEvent from_json(detail::JsonElem json);
    };

    /// 私聊消息被撤回事件
    struct FriendRecallEvent final : EventBase
    {
        static constexpr EventType type = EventType::friend_recall;

        UserId sender_id; ///< 被撤回消息的发送者 QQ 号
        MessageId msg_id; ///< 被撤回消息的 id
        int32_t time{}; ///< 被撤回消息发送的时间

        /**
         * \brief 异步地引用回复被撤回的消息
         * \param message 消息内容
         * \return （异步）已发送消息的 id，用于撤回和引用回复
         */
        clu::task<MessageId> async_quote_reply(const Message& message) const;

        static FriendRecallEvent from_json(detail::JsonElem json);
    };

    /// 群名更改事件
    struct GroupNameChangeEvent final : GroupExecutorEventBase
    {
        static constexpr EventType type = EventType::group_name_change;

        std::string original; ///< 原群名
        std::string current; ///< 现群名

        static GroupNameChangeEvent from_json(detail::JsonElem json);
    };

    /// 进群公告变动事件
    struct GroupEntranceAnnouncementChangeEvent final : GroupExecutorEventBase
    {
        static constexpr EventType type = EventType::group_entrance_announcement_change;

        std::string original; ///< 原公告
        std::string current; ///< 现公告

        static GroupEntranceAnnouncementChangeEvent from_json(detail::JsonElem json);
    };

    /// 群功能开启状态变动事件
    struct GroupConfigEvent final : GroupExecutorEventBase
    {
        static constexpr EventType type = EventType::group_config;

        enum class SubType : uint8_t { mute_all, anonymous_chat, confess_talk, member_invite }; ///< 事件子类型枚举
        SubType subtype{}; ///< 事件子类型
        bool original{}; ///< 原状态
        bool current{}; ///< 现状态

        static GroupConfigEvent from_json(detail::JsonElem json);
    };

    /// 成员加群事件
    struct MemberJoinEvent final : MemberEventBase
    {
        static constexpr EventType type = EventType::member_join;

        static MemberJoinEvent from_json(detail::JsonElem json);
    };

    /// 成员退群事件
    struct MemberQuitEvent final : MemberEventBase
    {
        static constexpr EventType type = EventType::member_quit;

        static MemberQuitEvent from_json(detail::JsonElem json);
    };

    /// 成员被踢出群事件
    struct MemberKickedEvent final : MemberExecutorEventBase
    {
        static constexpr EventType type = EventType::member_kicked;

        static MemberKickedEvent from_json(detail::JsonElem json);
    };

    /// 成员群名片变动事件
    struct MemberCardChangeEvent final : MemberExecutorEventBase
    {
        static constexpr EventType type = EventType::member_card_change;

        std::string original; ///< 原群名片
        std::string current; ///< 现群名片

        static MemberCardChangeEvent from_json(detail::JsonElem json);
    };

    /// 成员专属头衔变动事件
    struct MemberSpecialTitleChangeEvent final : MemberEventBase
    {
        static constexpr EventType type = EventType::member_special_title_change;

        std::string original; ///< 原头衔
        std::string current; ///< 现头衔

        static MemberSpecialTitleChangeEvent from_json(detail::JsonElem json);
    };

    /// 成员群权限变动事件
    struct MemberPermissionChangeEvent final : MemberEventBase
    {
        static constexpr EventType type = EventType::member_permission_change;

        Permission original{}; ///< 原权限
        Permission current{}; ///< 现权限

        static MemberPermissionChangeEvent from_json(detail::JsonElem json);
    };

    /// 成员被禁言事件
    struct MemberMutedEvent final : MemberExecutorEventBase
    {
        static constexpr EventType type = EventType::member_muted;

        std::chrono::seconds duration{}; ///< 被禁言的时长

        static MemberMutedEvent from_json(detail::JsonElem json);
    };

    /// 成员被解禁事件
    struct MemberUnmutedEvent final : MemberExecutorEventBase
    {
        static constexpr EventType type = EventType::member_unmuted;

        static MemberUnmutedEvent from_json(detail::JsonElem json);
    };

    /// 加好友申请事件
    struct NewFriendRequestEvent final : EventBase
    {
        static constexpr EventType type = EventType::new_friend_request;

        enum class ResponseType : uint8_t { approve = 0, reject = 1, reject_and_blacklist = 2 };

        int64_t id{}; ///< 事件 id
        UserId from_id; ///< 申请人的 QQ 号
        GroupId group_id; ///< 申请人通过何群加好友
        std::string name; ///< 申请人昵称
        std::string message; ///< 申请附言

        clu::task<> async_respond(ResponseType response, std::string_view reason) const;

        static NewFriendRequestEvent from_json(detail::JsonElem json);
    };

    using NewFriendResponseType = NewFriendRequestEvent::ResponseType;

    /// 加群申请事件
    struct MemberJoinRequestEvent final : EventBase
    {
        static constexpr EventType type = EventType::member_join_request;

        enum class ResponseType : uint8_t
        {
            approve = 0, reject = 1, ignore = 2,
            reject_and_blacklist = 3, ignore_and_blacklist = 4
        };

        int64_t id{}; ///< 事件 id
        UserId from_id; ///< 申请人的 QQ 号
        GroupId group_id; ///< 申请加入的群
        std::string group_name; ///< 群名称
        std::string name; ///< 申请人昵称
        std::string message; ///< 申请附言

        clu::task<> async_respond(ResponseType response, std::string_view reason) const;

        static MemberJoinRequestEvent from_json(detail::JsonElem json);
    };

    using MemberJoinResponseType = MemberJoinRequestEvent::ResponseType;

    /// Bot 被邀请入群事件
    struct BotInvitedJoinGroupRequestEvent final : EventBase
    {
        static constexpr EventType type = EventType::bot_invited_join_group_request;

        enum class ResponseType : uint8_t { approve = 0, reject = 1 };

        int64_t id{}; ///< 事件 id
        UserId from_id; ///< 申请人的 QQ 号
        GroupId group_id; ///< 申请加入的群
        std::string group_name; ///< 群名称
        std::string name; ///< 申请人昵称
        std::string message; ///< 申请附言

        clu::task<> async_respond(ResponseType response, std::string_view reason) const;

        static BotInvitedJoinGroupRequestEvent from_json(detail::JsonElem json);
    };

    using BotInvitedJoinGroupResponseType = BotInvitedJoinGroupRequestEvent::ResponseType;
}
