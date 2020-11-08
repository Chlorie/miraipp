#pragma once

#include <fmt/format.h>

namespace mpp
{
    /// 类型可以使用 fmt 格式化到字符串
    template <typename T>
    concept Formattable = requires(const T& value, fmt::format_context& ctx) { value.format_to(ctx); };
}

namespace fmt
{
    template <mpp::Formattable T>
    struct formatter<T>
    {
        constexpr auto parse(format_parse_context& ctx) const { return ctx.begin(); }

        auto format(const T& value, format_context& ctx) const
        {
            value.format_to(ctx);
            return ctx.out();
        }
    };
}
