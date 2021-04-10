#pragma once

#include <string>

#include "common.h"
#include "export.h"
#include "../detail/json_fwd.h"

namespace mpp
{
    enum class Permission : uint8_t { member, admin, owner };

    MPP_API Permission permission_from_string(std::string_view str);

    MPP_SUPPRESS_EXPORT_WARNING
    struct MPP_API Friend final
    {
        UserId id;
        std::string name;
        std::string remark;

        static Friend from_json(detail::JsonElem json);
    };

    struct MPP_API Group final
    {
        GroupId id;
        std::string name;
        Permission permission{};

        static Group from_json(detail::JsonElem json);
    };

    struct MPP_API Member final
    {
        Group group;
        UserId id;
        std::string name;
        Permission permission{};

        static Member from_json(detail::JsonElem json);
    };
    MPP_RESTORE_EXPORT_WARNING
}
