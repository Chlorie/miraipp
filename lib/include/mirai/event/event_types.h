#pragma once

#include "event.h"
#include "event_bases.h"

namespace mpp
{
    MPP_SUPPRESS_EXPORT_WARNING

    /// 私聊消息事件
    struct MPP_API FriendMessageEvent final : MessageEventBase
    {
        static constexpr EventType type = EventType::friend_message;

        Friend sender; ///< 消息的发送者

        /**
         * \brief 向消息发送者发送消息
         * \param message 消息内容
         * \param quote （可选）要引用回复的消息 id
         * \return 已发送消息的 id，用于撤回和引用回复
         */
        ex::task<MessageId> send_message_async(const Message& message, clu::optional_param<MessageId> quote = {}) const;

        /**
         * \brief 引用回复本消息
         * \param message 消息内容
         * \return 已发送消息的 id，用于撤回和引用回复
         */
        ex::task<MessageId> quote_reply_async(const Message& message) const { return send_message_async(message, msg.source.id); }

        static FriendMessageEvent from_json(detail::JsonElem json);
    };

    /// 群聊消息事件
    struct MPP_API GroupMessageEvent final : MessageEventBase
    {
        static constexpr EventType type = EventType::group_message;

        Member sender; ///< 消息的发送者

        At at_sender() const { return At{ sender.id }; } ///< 获取对象为消息发送者的 at 消息段

        /**
         * \brief 异步地向消息发送者发送消息
         * \param message 消息内容
         * \param quote （可选）要引用回复的消息 id
         * \return （异步）已发送消息的 id，用于撤回和引用回复
         */
        ex::task<MessageId> send_message_async(const Message& message, clu::optional_param<MessageId> quote = {}) const;

        /**
         * \brief 异步地引用回复本消息
         * \param message 消息内容
         * \return （异步）已发送消息的 id，用于撤回和引用回复
         */
        ex::task<MessageId> quote_reply_async(const Message& message) const { return send_message_async(message, msg.source.id); }

        ex::task<void> recall_async() const; ///< 异步地撤回该消息

        /**
         * \brief 异步地禁言该消息发送者
         * \param duration 禁言时长
         */
        ex::task<void> mute_sender_async(std::chrono::seconds duration) const;

        static GroupMessageEvent from_json(detail::JsonElem json);
    };

    /// 临时会话消息事件
    struct MPP_API TempMessageEvent final : MessageEventBase
    {
        static constexpr EventType type = EventType::temp_message;

        Member sender; ///< 消息的发送者

        /**
         * \brief 异步地向消息发送者发送消息
         * \param message 消息内容
         * \param quote （可选）要引用回复的消息 id
         * \return （异步）已发送消息的 id，用于撤回和引用回复
         */
        ex::task<MessageId> send_message_async(const Message& message, clu::optional_param<MessageId> quote = {}) const;

        /**
         * \brief 异步地引用回复本消息
         * \param message 消息内容
         * \return （异步）已发送消息的 id，用于撤回和引用回复
         */
        ex::task<MessageId> quote_reply_async(const Message& message) const { return send_message_async(message, msg.source.id); }

        static TempMessageEvent from_json(detail::JsonElem json);
    };

    /// Bot 上线事件
    struct MPP_API BotOnlineEvent final : EventBase
    {
        static constexpr EventType type = EventType::bot_online;

        enum class SubType : uint8_t { login, relogin }; ///< 事件的子类型枚举
        SubType subtype{}; ///< 事件的子类型
        UserId id; ///< 上线 bot 的 QQ 号

        static BotOnlineEvent from_json(detail::JsonElem json);
    };

    /// Bot 下线事件
    struct MPP_API BotOfflineEvent final : EventBase
    {
        static constexpr EventType type = EventType::bot_offline;

        enum class SubType : uint8_t { active, forced, dropped }; ///< 事件的子类型枚举
        SubType subtype{}; ///< 事件的子类型
        UserId id; ///< 下线 bot 的 QQ 号

        static BotOfflineEvent from_json(detail::JsonElem json);
    };

    /// Bot 群权限变动事件
    struct MPP_API BotGroupPermissionChangeEvent final : GroupEventBase
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
    struct MPP_API BotUnmutedEvent final : ExecutorEventBase
    {
        static constexpr EventType type = EventType::bot_unmuted;

        static BotUnmutedEvent from_json(detail::JsonElem json);
    };

    /// Bot 加入新群事件
    struct MPP_API BotJoinGroupEvent final : GroupEventBase
    {
        static constexpr EventType type = EventType::bot_join_group;

        static BotJoinGroupEvent from_json(detail::JsonElem json);
    };

    /// Bot 退群事件
    struct MPP_API BotQuitEvent final : GroupEventBase
    {
        static constexpr EventType type = EventType::bot_quit;

        static BotQuitEvent from_json(detail::JsonElem json);
    };

    /// Bot 被踢出群事件
    struct MPP_API BotKickedEvent final : GroupEventBase
    {
        static constexpr EventType type = EventType::bot_kicked;

        static BotKickedEvent from_json(detail::JsonElem json);
    };

    /// 群消息被撤回事件
    struct MPP_API GroupRecallEvent final : GroupExecutorEventBase
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
        ex::task<MessageId> quote_reply_async(const Message& message) const { return send_message_async(message, msg_id); }

        static GroupRecallEvent from_json(detail::JsonElem json);
    };

    /// 私聊消息被撤回事件
    struct MPP_API FriendRecallEvent final : EventBase
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
        ex::task<MessageId> quote_reply_async(const Message& message) const;

        static FriendRecallEvent from_json(detail::JsonElem json);
    };

    /// 群名更改事件
    struct MPP_API GroupNameChangeEvent final : GroupExecutorEventBase
    {
        static constexpr EventType type = EventType::group_name_change;

        std::string original; ///< 原群名
        std::string current; ///< 现群名

        static GroupNameChangeEvent from_json(detail::JsonElem json);
    };

    /// 进群公告变动事件
    struct MPP_API GroupEntranceAnnouncementChangeEvent final : GroupExecutorEventBase
    {
        static constexpr EventType type = EventType::group_entrance_announcement_change;

        std::string original; ///< 原公告
        std::string current; ///< 现公告

        static GroupEntranceAnnouncementChangeEvent from_json(detail::JsonElem json);
    };

    /// 群功能开启状态变动事件
    struct MPP_API GroupConfigEvent final : GroupExecutorEventBase
    {
        static constexpr EventType type = EventType::group_config;

        enum class SubType : uint8_t { mute_all, anonymous_chat, confess_talk, member_invite }; ///< 事件子类型枚举
        SubType subtype{}; ///< 事件子类型
        bool original{}; ///< 原状态
        bool current{}; ///< 现状态

        static GroupConfigEvent from_json(detail::JsonElem json);
    };

    /// 成员加群事件
    struct MPP_API MemberJoinEvent final : MemberEventBase
    {
        static constexpr EventType type = EventType::member_join;

        static MemberJoinEvent from_json(detail::JsonElem json);
    };

    /// 成员退群事件
    struct MPP_API MemberQuitEvent final : MemberEventBase
    {
        static constexpr EventType type = EventType::member_quit;

        static MemberQuitEvent from_json(detail::JsonElem json);
    };

    /// 成员被踢出群事件
    struct MPP_API MemberKickedEvent final : MemberExecutorEventBase
    {
        static constexpr EventType type = EventType::member_kicked;

        static MemberKickedEvent from_json(detail::JsonElem json);
    };

    /// 成员群名片变动事件
    struct MPP_API MemberCardChangeEvent final : MemberExecutorEventBase
    {
        static constexpr EventType type = EventType::member_card_change;

        std::string original; ///< 原群名片
        std::string current; ///< 现群名片

        static MemberCardChangeEvent from_json(detail::JsonElem json);
    };

    /// 成员专属头衔变动事件
    struct MPP_API MemberSpecialTitleChangeEvent final : MemberEventBase
    {
        static constexpr EventType type = EventType::member_special_title_change;

        std::string original; ///< 原头衔
        std::string current; ///< 现头衔

        static MemberSpecialTitleChangeEvent from_json(detail::JsonElem json);
    };

    /// 成员群权限变动事件
    struct MPP_API MemberPermissionChangeEvent final : MemberEventBase
    {
        static constexpr EventType type = EventType::member_permission_change;

        Permission original{}; ///< 原权限
        Permission current{}; ///< 现权限

        static MemberPermissionChangeEvent from_json(detail::JsonElem json);
    };

    /// 成员被禁言事件
    struct MPP_API MemberMutedEvent final : MemberExecutorEventBase
    {
        static constexpr EventType type = EventType::member_muted;

        std::chrono::seconds duration{}; ///< 被禁言的时长

        static MemberMutedEvent from_json(detail::JsonElem json);
    };

    /// 成员被解禁事件
    struct MPP_API MemberUnmutedEvent final : MemberExecutorEventBase
    {
        static constexpr EventType type = EventType::member_unmuted;

        static MemberUnmutedEvent from_json(detail::JsonElem json);
    };

    enum class NewFriendResponseType : uint8_t { approve = 0, reject = 1, reject_and_blacklist = 2 };

    /// 加好友申请事件
    struct MPP_API NewFriendRequestEvent final : EventBase
    {
        static constexpr EventType type = EventType::new_friend_request;

        using ResponseType = NewFriendResponseType;

        int64_t id{}; ///< 事件 id
        UserId from_id; ///< 申请人的 QQ 号
        GroupId group_id; ///< 申请人通过何群加好友
        std::string name; ///< 申请人昵称
        std::string message; ///< 申请附言

        ex::task<void> respond_async(ResponseType response, std::string_view reason) const;

        static NewFriendRequestEvent from_json(detail::JsonElem json);
    };

    enum class MemberJoinResponseType : uint8_t
    {
        approve = 0, reject = 1, ignore = 2,
        reject_and_blacklist = 3, ignore_and_blacklist = 4
    };

    /// 加群申请事件
    struct MPP_API MemberJoinRequestEvent final : EventBase
    {
        static constexpr EventType type = EventType::member_join_request;

        using ResponseType = MemberJoinResponseType;

        int64_t id{}; ///< 事件 id
        UserId from_id; ///< 申请人的 QQ 号
        GroupId group_id; ///< 申请加入的群
        std::string group_name; ///< 群名称
        std::string name; ///< 申请人昵称
        std::string message; ///< 申请附言

        ex::task<void> respond_async(ResponseType response, std::string_view reason) const;

        static MemberJoinRequestEvent from_json(detail::JsonElem json);
    };

    enum class BotInvitedJoinGroupResponseType : uint8_t { approve = 0, reject = 1 };

    /// Bot 被邀请入群事件
    struct MPP_API BotInvitedJoinGroupRequestEvent final : EventBase
    {
        static constexpr EventType type = EventType::bot_invited_join_group_request;

        using ResponseType = BotInvitedJoinGroupResponseType;

        int64_t id{}; ///< 事件 id
        UserId from_id; ///< 申请人的 QQ 号
        GroupId group_id; ///< 申请加入的群
        std::string group_name; ///< 群名称
        std::string name; ///< 申请人昵称
        std::string message; ///< 申请附言

        ex::task<void> respond_async(ResponseType response, std::string_view reason) const;

        static BotInvitedJoinGroupRequestEvent from_json(detail::JsonElem json);
    };

    MPP_RESTORE_EXPORT_WARNING
}
