#pragma once

#include "message.h"

namespace mpp
{
    MPP_SUPPRESS_EXPORT_WARNING
    struct MPP_API ForwardedMessage final
    {
        UserId sender;
        int32_t time = 0;
        std::string sender_name;
        Message content;

        bool operator==(const ForwardedMessage&) const noexcept = default;

        void format_as_json(fmt::format_context& ctx) const;
        static ForwardedMessage from_json(detail::JsonElem json);
    };
    MPP_RESTORE_EXPORT_WARNING
}
