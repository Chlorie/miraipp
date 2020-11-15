#pragma once

#include <string>

#include "common.h"
#include "../detail/json.h"

namespace mpp
{
    enum class Permission : uint8_t { member, admin, owner };

    Permission permission_from_string(std::string_view str);

    struct Friend final
    {
        UserId id;
        std::string name;
        std::string remark;

        static Friend from_json(detail::JsonElem json);
    };

    struct Group final
    {
        GroupId id;
        std::string name;
        Permission permission;

        static Group from_json(detail::JsonElem json);
    };

    struct Member final
    {
        Group group;
        UserId id;
        std::string name;
        Permission permission;

        static Member from_json(detail::JsonElem json);
    };
}
