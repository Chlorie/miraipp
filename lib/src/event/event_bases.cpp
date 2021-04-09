#include "mirai/event/event_bases.h"
#include "mirai/core/bot.h"

namespace mpp
{
    MessageEventBase MessageEventBase::from_json(const detail::JsonElem json)
    {
        return { .msg = SentMessage::from_json(json["messageChain"]) };
    }

    ex::task<MessageId> GroupEventBase::send_message_async(
        const Message& message, const clu::optional_param<MessageId> quote) const
    {
        return bot().send_message_async(group.id, message, quote);
    }

    GroupEventBase GroupEventBase::from_json(const detail::JsonElem json)
    {
        return { .group = Group::from_json(json["group"]) };
    }

    ExecutorEventBase ExecutorEventBase::from_json(const detail::JsonElem json)
    {
        return { .executor = Member::from_json(json["operator"]) };
    }

    At GroupExecutorEventBase::at_executor() const { return At{ executor ? executor->id : bot().id() }; }

    GroupExecutorEventBase GroupExecutorEventBase::from_json(const detail::JsonElem json)
    {
        return
        {
            GroupEventBase::from_json(json),
            Member::from_json(json["operator"])
        };
    }

    ex::task<MessageId> MemberEventBase::send_message_async(
        const Message& message, const clu::optional_param<MessageId> quote) const
    {
        return bot().send_message_async(member.group.id, message, quote);
    }

    ex::task<void> MemberEventBase::mute_member_async(const std::chrono::seconds duration) const
    {
        return bot().mute_async(member.group.id, member.id, duration);
    }

    ex::task<void> MemberEventBase::unmute_member_async() const
    {
        return bot().unmute_async(member.group.id, member.id);
    }

    MemberEventBase MemberEventBase::from_json(const detail::JsonElem json)
    {
        return { .member = Member::from_json(json["member"]) };
    }

    At MemberExecutorEventBase::at_executor() const { return At{ executor ? executor->id : bot().id() }; }

    MemberExecutorEventBase MemberExecutorEventBase::from_json(const detail::JsonElem json)
    {
        return
        {
            MemberEventBase::from_json(json),
            Member::from_json(json["operator"])
        };
    }
}
