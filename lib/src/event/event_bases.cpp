#include "mirai/event/event_bases.h"
#include "mirai/core/bot.h"

namespace mpp
{
    clu::task<MessageId> GroupEventBase::async_send_message(
        const Message& message, const clu::optional_param<MessageId> quote) const
    {
        return bot().async_send_message(group.id, message, quote);
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

    clu::task<MessageId> MemberEventBase::async_send_message(
        const Message& message, const clu::optional_param<MessageId> quote) const
    {
        return bot().async_send_message(member.group.id, message, quote);
    }

    clu::task<> MemberEventBase::async_mute_member(const std::chrono::seconds duration) const
    {
        return bot().async_mute(member.group.id, member.id, duration);
    }

    clu::task<> MemberEventBase::async_unmute_member() const
    {
        return bot().async_unmute(member.group.id, member.id);
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
