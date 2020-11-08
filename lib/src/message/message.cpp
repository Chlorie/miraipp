#include "mirai/message/message.h"
#include "mirai/detail/json.h"

namespace mpp
{
    void Message::format_to(fmt::format_context& ctx) const
    {
        for (const Segment& seg : vec_)
            fmt::format_to(ctx.out(), "{}", seg);
    }

    void Message::format_as_json(fmt::format_context& ctx) const
    {
        detail::JsonArrScope scope(ctx);
        for (const Segment& seg : vec_)
            scope.add_entry(seg);
    }

    Message Message::from_json(const detail::JsonElem json)
    {
        Message result;
        for (const auto elem : json)
            result.vec_.emplace_back(Segment::from_json(elem));
        return result;
    }
}
