#pragma once

#include <cstdint>
#include <concepts>

#include "../core/export.h"

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

    class Bot;

    /// 事件基类
    class MPP_API EventBase
    {
        friend class Bot;
    private:
        Bot* bot_ = nullptr;

    public:
        constexpr EventBase() noexcept = default;
        Bot& bot() const noexcept { return *bot_; } ///< 获取指向收到该事件的 bot 的引用
    };

    // @formatter:off
    /// 事件组成类型概念
    template <typename T>
    concept ConcreteEvent = std::derived_from<T, EventBase>
        && requires { { T::type } -> std::convertible_to<EventType>; };
    // @formatter:on
}
