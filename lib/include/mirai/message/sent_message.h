#pragma once

#include "message.h"

namespace mpp
{
    struct Source final
    {
        MessageId id;
        int32_t time = 0;

        bool operator==(const Source&) const noexcept = default;
        static Source from_json(detail::JsonElem json);
    };

    struct Quote final
    {
        MessageId id;
        UserId sender;
        Message msg;

        bool operator==(const Quote& other) const noexcept { return id == other.id; }
        static Quote from_json(detail::JsonElem json);
    };

    struct SentMessage final
    {
        Source source;
        std::optional<Quote> quote;
        Message content;

        bool operator==(const SentMessage& other) const noexcept { return source == other.source; }
        static SentMessage from_json(detail::JsonElem json);
    };
}
