#pragma once

#include "message.h"
#include "../core/common.h"

namespace mpp
{
    MPP_SUPPRESS_EXPORT_WARNING
    struct MPP_API Source final
    {
        MessageId id;
        int32_t time = 0;

        bool operator==(const Source&) const noexcept = default;
        static Source from_json(detail::JsonElem json);
    };

    struct MPP_API Quote final
    {
        MessageId id;
        UserId sender;
        int32_t time = 0;
        Message msg;

        bool operator==(const Quote& other) const noexcept { return id == other.id; }
        static Quote from_json(detail::JsonElem json);
    };

    struct MPP_API SentMessage final
    {
        Source source;
        std::optional<Quote> quote;
        Message content;

        bool operator==(const SentMessage& other) const noexcept { return source == other.source; }
        static SentMessage from_json(detail::JsonElem json);
    };
    MPP_RESTORE_EXPORT_WARNING
}
