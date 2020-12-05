#pragma once

#include "../event/event.h"

namespace mpp
{
    namespace patterns
    {
        struct FromUser final
        {
            UserId id;

            bool match(const FriendMessageEvent& ev) const noexcept { return ev.sender.id == id; }

            bool match(const MemberEventBase& ev) const noexcept { return ev.member.id == id; }
            bool match(const ExecutorEventBase& ev) const noexcept { return ev.executor.id == id; }
        };

        struct FromGroup final
        {
            GroupId id;
        };

        struct FromTemp final
        {
            TempId id;

            bool match(const TempMessageEvent& ev) const noexcept
            {
                return ev.sender.id == id.uid
                    && ev.sender.group.id == id.gid;
            }
        };

        struct Replying final
        {
            MessageId id;

            bool match(const FriendMessageEvent& ev) const noexcept { return ev.msg.quote && ev.msg.quote->id == id; }
            bool match(const GroupMessageEvent& ev) const noexcept { return ev.msg.quote && ev.msg.quote->id == id; }
            bool match(const TempMessageEvent& ev) const noexcept { return ev.msg.quote && ev.msg.quote->id == id; }
        };
    }

    inline patterns::FromUser from(const UserId uid) noexcept { return { uid }; }
    inline patterns::FromGroup from(const GroupId gid) noexcept { return { gid }; }
    inline patterns::FromTemp from(const TempId tid) noexcept { return { tid }; }
    inline patterns::Replying replying(const MessageId mid) noexcept { return { mid }; }
}
