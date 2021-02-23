#include "mirai/message/message.h"

namespace mpp
{
    void Message::collapse_adjacent_text()
    {
        std::vector<Segment> result;
        for (Segment& segment : vec_)
        {
            if (const auto* ptr = segment.get_if<Plain>();
                ptr && !result.empty())
                if (auto* back = result.back().get_if<Plain>())
                {
                    back->text += ptr->text;
                    continue;
                }
            result.push_back(std::move(segment));
        }
        vec_ = std::move(result);
    }

    std::string Message::collect_text() const
    {
        std::string result;
        for (auto&& str : vec_
             | std::views::filter([](const Segment& seg) { return seg.type() == Plain::type; })
             | std::views::transform([](const Segment& seg) -> auto&& { return seg.get<Plain>().text; }))
            result += str;
        return result;
    }

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
        {
            Segment segment = Segment::from_json(elem);
            if (const auto* ptr = segment.get_if<Plain>();
                ptr && !result.empty())
                if (auto* back = result.back().get_if<Plain>())
                {
                    back->text += ptr->text;
                    continue;
                }
            result.vec_.emplace_back(std::move(segment));
        }
        return result;
    }
}
