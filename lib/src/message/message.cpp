#include "mirai/message/message.h"

namespace mpp
{
    namespace
    {
        bool compare_remove_prefix(std::string_view& whole, const std::string_view prefix)
        {
            if (!whole.starts_with(prefix)) return false;
            whole.remove_prefix(prefix.size());
            return true;
        }

        enum class rcp_res
        {
            left_empty, right_empty, both_empty,
            not_exhausted
        };

        rcp_res remove_common_prefix(std::string_view& lhs, std::string_view& rhs)
        {
            if (lhs.size() == rhs.size())
            {
                lhs = rhs = {};
                return lhs == rhs ? rcp_res::both_empty : rcp_res::not_exhausted;
            }
            if (lhs.size() > rhs.size())
            {
                const bool res = compare_remove_prefix(lhs, rhs);
                rhs = {};
                return res ? rcp_res::right_empty : rcp_res::not_exhausted;
            }
            const bool res = compare_remove_prefix(rhs, lhs);
            lhs = {};
            return res ? rcp_res::left_empty : rcp_res::not_exhausted;
        }

        bool is_empty_plain(const Segment& seg)
        {
            if (const auto* ptr = seg.get_if<Plain>())
                return ptr->text.empty();
            return false;
        }

        bool is_plain(const Segment& seg) { return seg.type() == Plain::type; }

        std::string_view to_sv(const Segment& seg) { return seg.get<Plain>().text; }

        bool are_plain_spans_equal(std::span<const Segment> first, const std::span<const Segment> second)
        {
            const auto first_range = first | std::views::transform(to_sv);
            const auto second_range = second | std::views::transform(to_sv);
            auto first_iter = first_range.begin(), second_iter = second_range.begin();

            using iter_t = decltype(first_iter);

            static constexpr auto move_next_non_empty = [](iter_t& iter, const iter_t end)
            {
                iter = std::ranges::find_if_not(iter, end, &std::string_view::empty);
                return iter != end ? *iter++ : std::string_view{};
            };
            const auto move_first = [&] { return move_next_non_empty(first_iter, first_range.end()); };
            const auto move_second = [&] { return move_next_non_empty(second_iter, second_range.end()); };

            std::string_view first_sv = move_first(), second_sv = move_second();

            while (true)
            {
                if (first_sv.empty() || second_sv.empty())
                    return first_sv.empty() == second_sv.empty();
                // @formatter:off
                switch (remove_common_prefix(first_sv, second_sv))
                {
                    case rcp_res::both_empty: second_sv = move_second(); [[fallthrough]];
                    case rcp_res::left_empty: first_sv = move_first(); break;
                    case rcp_res::right_empty: second_sv = move_second(); break;
                    case rcp_res::not_exhausted: return false;
                }
                // @formatter:on
            }
        }
    }

    bool Message::sv_compare_impl(std::string_view other) const noexcept
    {
        for (auto&& segment : vec_)
            if (const auto* ptr = segment.get_if<Plain>())
            {
                if (!compare_remove_prefix(other, ptr->text))
                    return false;
            }
            else
                return false;
        return other.empty();
    }

    bool Message::operator==(const Message& other) const noexcept
    {
        auto first = begin(), second = other.begin();
        while (true)
        {
            std::tie(first, second) = std::mismatch(first, end(), second, other.end());
            if (first == end()) return std::all_of(second, other.end(), is_empty_plain);
            if (second == other.end()) return std::all_of(first, end(), is_empty_plain);
            const auto first_end = std::find_if_not(first, end(), is_plain);
            const auto second_end = std::find_if_not(second, other.end(), is_plain);
            if (first_end == first && second_end == second) return false;
            if (!are_plain_spans_equal({ first, first_end }, { second, second_end })) return false;
            std::tie(first, second) = std::pair{ first_end, second_end };
        }
    }

    bool Message::operator==(const Segment& other) const noexcept
    {
        if (const auto* ptr = other.get_if<Plain>())
            return *this == ptr->text;
        return size() == 1 && front() == other;
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
            result.vec_.emplace_back(Segment::from_json(elem));
        return result;
    }
}
