#include "mirai/core/config_types.h"

#include "../detail/json.h"

namespace mpp
{
    SessionConfig SessionConfig::from_json(const detail::JsonElem json)
    {
        return SessionConfig
        {
            .cache_size = detail::from_json<std::optional<int32_t>>(json["cacheSize"]),
            .enable_websocket = detail::from_json<std::optional<bool>>(json["enableWebsocket"])
        };
    }

    GroupConfig GroupConfig::from_json(const detail::JsonElem json)
    {
        return GroupConfig
        {
            .name = detail::from_json<std::optional<std::string>>(json["name"]),
            .announcement = detail::from_json<std::optional<std::string>>(json["announcement"]),
            .allow_confess_talk = detail::from_json<std::optional<bool>>(json["confessTalk"]),
            .allow_member_invitation = detail::from_json<std::optional<bool>>(json["allowMemberInvite"]),
            .auto_approve = detail::from_json<std::optional<bool>>(json["autoApprove"]),
            .allow_anonymous_chat = detail::from_json<std::optional<bool>>(json["anonymousChat"])
        };
    }

    void GroupConfig::format_as_json(fmt::format_context& ctx) const
    {
        detail::JsonObjScope obj(ctx);
        obj.add_entry("name", name);
        obj.add_entry("announcement", announcement);
        obj.add_entry("confessTalk", allow_confess_talk);
        obj.add_entry("allowMemberInvite", allow_member_invitation);
        obj.add_entry("autoApprove", auto_approve);
        obj.add_entry("anonymousChat", allow_anonymous_chat);
    }

    MemberInfo MemberInfo::from_json(const detail::JsonElem json)
    {
        return MemberInfo
        {
            .name = detail::from_json<std::optional<std::string>>(json["name"]),
            .special_title = detail::from_json<std::optional<std::string>>(json["specialTitle"])
        };
    }

    void MemberInfo::format_as_json(fmt::format_context& ctx) const
    {
        detail::JsonObjScope obj(ctx);
        obj.add_entry("name", name);
        obj.add_entry("specialTitle", special_title);
    }
}
