#pragma once

#include <clu/coroutine/task.h>
#include <clu/optional_ref.h>

#include "event.h"
#include "../core/info_types.h"
#include "../message/sent_message.h"

namespace mpp
{
    /// 消息事件基类
    struct MessageEventBase : EventBase
    {
        SentMessage msg; ///< 收到的消息

        MessageId msgid() const noexcept { return msg.source.id; } ///< 消息的 id
        const Message& content() const noexcept { return msg.content; } ///< 消息内容

        static MessageEventBase from_json(detail::JsonElem json);
    };

    /// 群事件基类
    struct GroupEventBase : EventBase
    {
        Group group; ///< 事件来源的群

        /**
         * \brief 异步地向事件来源群发送消息
         * \param message 消息内容
         * \param quote （可选）要引用回复的消息 id
         * \return （异步）已发送消息的 id，用于撤回和引用回复
         */
        clu::task<MessageId> async_send_message(const Message& message, clu::optional_param<MessageId> quote = {}) const;

        static GroupEventBase from_json(detail::JsonElem json);
    };

    /// 包含操作者的事件基类
    struct ExecutorEventBase : EventBase
    {
        Member executor; ///< 事件的操作者

        At at_executor() const { return At{ executor.id }; } ///< 获取对象为事件操作者的 at 消息段

        static ExecutorEventBase from_json(detail::JsonElem json);
    };

    /// 包含操作者的群事件基类
    struct GroupExecutorEventBase : GroupEventBase
    {
        std::optional<Member> executor; ///< 事件的操作者

        At at_executor() const; ///< 获取对象为事件操作者的 at 消息段

        static GroupExecutorEventBase from_json(detail::JsonElem json);
    };

    /// 群成员事件基类
    struct MemberEventBase : EventBase
    {
        Member member; ///< 与当前事件关联的群成员

        At at_member() const { return At{ member.id }; } ///< 获取对象为与当前事件关联的群成员的 at 消息段

        /**
         * \brief 异步地向事件来源群发送消息
         * \param message 消息内容
         * \param quote （可选）要引用回复的消息 id
         * \return （异步）已发送消息的 id，用于撤回和引用回复
         */
        clu::task<MessageId> async_send_message(const Message& message, clu::optional_param<MessageId> quote = {}) const;

        /**
         * \brief 异步地禁言与当前事件关联的群成员
         * \param duration 禁言时长
         */
        clu::task<> async_mute_member(std::chrono::seconds duration) const;

        clu::task<> async_unmute_member() const; ///< 异步地解禁与当前事件关联的群成员

        static MemberEventBase from_json(detail::JsonElem json);
    };
    
    /// 包含操作者的群成员事件基类
    struct MemberExecutorEventBase : MemberEventBase
    {
        std::optional<Member> executor; ///< 事件的操作者

        At at_executor() const; ///< 获取对象为事件操作者的 at 消息段

        static MemberExecutorEventBase from_json(detail::JsonElem json);
    };
}
