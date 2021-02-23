#pragma once

#include "../event/event_types.h"

namespace mpp
{
    namespace patterns
    {
        struct FromUser final
        {
            UserId id;

            bool match(const FriendMessageEvent& ev) const noexcept { return ev.sender.id == id; }
            bool match(const GroupMessageEvent& ev) const noexcept { return ev.sender.id == id; }

            bool match(const MemberEventBase& ev) const noexcept { return ev.member.id == id; }
            bool match(const ExecutorEventBase& ev) const noexcept { return ev.executor.id == id; }
        };

        struct FromGroup final
        {
            GroupId id;

            bool match(const GroupMessageEvent& ev) const noexcept { return ev.sender.group.id == id; }
        };

        struct FromMember final
        {
            UserId uid;
            GroupId gid;

            bool match(const GroupMessageEvent& ev) const noexcept
            {
                return ev.sender.id == uid
                    && ev.sender.group.id == gid;
            }
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
            bool match(const MessageEventBase& ev) const noexcept { return ev.msg.quote && ev.msg.quote->id == id; }
        };

        template <typename T>
        struct WithContent final
        {
            T message;
            bool match(const MessageEventBase& ev) const noexcept { return ev.content() == message; }
        };

        template <typename Pr>
        struct Satisfying final
        {
            Pr predicate;

            template <ConcreteEvent E> requires std::predicate<Pr, const E&>
            bool match(const E& ev) const noexcept { return std::invoke(predicate, ev); }
        };
    }

    inline patterns::FromUser from(const UserId uid) noexcept { return { uid }; }
    inline patterns::FromUser from(const Friend& friend_) noexcept { return { friend_.id }; }
    inline patterns::FromGroup from(const GroupId gid) noexcept { return { gid }; }
    inline patterns::FromGroup from(const Group& group) noexcept { return { group.id }; }
    inline patterns::FromMember from(const Member& member) noexcept { return { member.id, member.group.id }; }
    inline patterns::FromTemp from(const TempId tid) noexcept { return { tid }; }
    inline patterns::Replying replying(const MessageId mid) noexcept { return { mid }; }

    template <typename T = Message> requires std::equality_comparable_with<std::decay_t<T>, Message>
    patterns::WithContent<std::decay_t<T>> with_content(T&& content) { return { std::forward<T>(content) }; }

    template <typename Pr>
    patterns::Satisfying<std::remove_cvref_t<Pr>> satisfying(Pr&& predicate) { return { std::forward<Pr>(predicate) }; }
}
