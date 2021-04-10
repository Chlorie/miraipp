#include "mirai/core/info_types.h"

#include <stdexcept>
#include <clu/hash.h>

#include "../detail/json.h"

namespace mpp
{
    Permission permission_from_string(const std::string_view str)
    {
        using namespace clu::literals;
        // @formatter:off
        switch (clu::fnv1a(str))
        {
            case "MEMBER"_fnv1a:        return Permission::member;
            case "ADMINISTRATOR"_fnv1a: return Permission::admin;
            case "OWNER"_fnv1a:         return Permission::owner;
            default: throw std::runtime_error("未知的权限类型");
        }
        // @formatter:on
    }
    
    Friend Friend::from_json(const detail::JsonElem json)
    {
        return
        {
            .id = UserId(json["id"].get_int64()),
            .name = detail::from_json<std::string>(json["nickname"]),
            .remark = detail::from_json<std::string>(json["remark"])
        };
    }

    Group Group::from_json(const detail::JsonElem json)
    {
        return
        {
            .id = GroupId(json["id"].get_int64()),
            .name = detail::from_json<std::string>(json["name"]),
            .permission = permission_from_string(json["permission"])
        };
    }

    Member Member::from_json(const detail::JsonElem json)
    {
        return
        {
            .group = Group::from_json(json["group"]),
            .id = UserId(json["id"].get_int64()),
            .name = detail::from_json<std::string>(json["memberName"]),
            .permission = permission_from_string(json["permission"])
        };
    }
}
