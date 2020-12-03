#include "mirai/message/sent_message.h"

#include <clu/hash.h>

namespace mpp
{
    Source Source::from_json(const detail::JsonElem json)
    {
        return Source
        {
            .id = MessageId(detail::from_json<int32_t>(json["id"])),
            .time = detail::from_json<int32_t>(json["time"])
        };
    }

    Quote Quote::from_json(const detail::JsonElem json)
    {
        return Quote
        {
            MessageId(detail::from_json<int32_t>(json["id"])),
            UserId(json["senderId"].get_int64()),
            Message::from_json(json["origin"])
        };
    }

    SentMessage SentMessage::from_json(const detail::JsonElem json)
    {
        SentMessage result;
        for (const auto elem : json)
        {
            using namespace clu::literals;
            const std::string_view type = elem["type"];
            // @formatter:off
            switch (clu::fnv1a(type))
            {
                case "Source"_fnv1a: result.source = Source::from_json(elem); break;
                case "Quote"_fnv1a: result.quote = Quote::from_json(elem); break;
                default: result.content += Segment::from_json(elem);
            }
            // @formatter:on
        }
        return result;
    }
}
