#pragma once

#include <optional>

#include "../detail/json_fwd.h"

namespace mpp
{
    struct SessionConfig final
    {
        std::optional<int32_t> cache_size;
        std::optional<bool> enable_websocket;
        
        static SessionConfig from_json(detail::JsonElem json);
    };

    struct GroupConfig final
    {
        std::optional<std::string> name;
        std::optional<std::string> announcement;
        std::optional<bool> allow_confess_talk;
        std::optional<bool> allow_member_invitation;
        std::optional<bool> auto_approve;
        std::optional<bool> allow_anonymous_chat;

        static GroupConfig from_json(detail::JsonElem json);
        void format_as_json(fmt::format_context& ctx) const;
    };

    struct MemberInfo final
    {
        std::optional<std::string> name;
        std::optional<std::string> special_title;

        static MemberInfo from_json(detail::JsonElem json);
        void format_as_json(fmt::format_context& ctx) const;
    };
}
