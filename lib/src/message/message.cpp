#include "mirai/message/message.h"

#include "mirai/message/forwarded_message.h"
#include "../detail/json.h"

namespace mpp
{
    bool Message::starts_with_sv(const std::string_view sv) const
    {
        if (sv.empty()) return true;
        if (empty()) return false;
        if (const auto* ptr = front().get_if<Plain>())
        {
            const std::string_view prefix =
                std::string_view(ptr->text).substr(0, sv.length());
            return prefix == sv;
        }
        return false;
    }

    bool Message::ends_with_sv(const std::string_view sv) const
    {
        if (sv.empty()) return true;
        if (empty()) return false;
        if (const auto* ptr = front().get_if<Plain>())
        {
            const std::string_view full = ptr->text;
            const std::string_view suffix =
                full.substr(full.length() - sv.length(), sv.length());
            return suffix == sv;
        }
        return false;
    }

    bool Message::contains_sv(const std::string_view sv) const
    {
        if (sv.empty()) return true;
        return std::ranges::any_of(vec_, [sv](const Segment& seg)
        {
            if (const auto* ptr = seg.get_if<Plain>())
            {
                const std::string_view plain = ptr->text;
                return plain.find(sv) != std::string_view::npos;
            }
            return false;
        });
    }

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

    bool Message::starts_with(const Segment& prefix) const
    {
        if (const auto* ptr = prefix.get_if<Plain>())
            return starts_with_sv(ptr->text);
        else
            return !vec_.empty() && vec_.front() == prefix;
    }

    bool Message::starts_with(const Message& prefix) const
    {
        return false; // TODO: implement
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
