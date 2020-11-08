#pragma once

#include <optional>
#include <fmt/core.h>
#include <simdjson.h>
#include <clu/concepts.h>

namespace mpp::detail
{
    using JsonElem = simdjson::dom::element;
    using JsonRes = simdjson::simdjson_result<JsonElem>;

    template <typename T>
    concept JsonSerializable = requires(const T& value, fmt::format_context& ctx) { value.format_as_json(ctx); };

    template <typename T>
    concept JsonDeserializable = requires(JsonElem elem) { { T::from_json(elem) } -> std::convertible_to<T>; };
}

// Serialization

namespace mpp::detail
{
    // @formatter:off
    struct JsonQuoted { std::string_view content; };
    template <typename T> struct JsonFormatWrapper { const T& ref; };
    // @formatter:on

    template <typename T> auto as_json(const T& value) { return JsonFormatWrapper<T>{ value }; }
}

namespace fmt
{
    template <>
    struct formatter<mpp::detail::JsonQuoted>
    {
        constexpr auto parse(format_parse_context& ctx) const { return ctx.begin(); }

        template <typename Ctx>
        auto format(const mpp::detail::JsonQuoted& value, Ctx& ctx)
        {
            // @formatter:off
            fmt::format_to(ctx.out(), "\"");
            for (const char c : value.content)
                switch (c)
                {
                    case '\b': fmt::format_to(ctx.out(), "\\b"); continue;
                    case '\f': fmt::format_to(ctx.out(), "\\f"); continue;
                    case '\n': fmt::format_to(ctx.out(), "\\n"); continue;
                    case '\r': fmt::format_to(ctx.out(), "\\r"); continue;
                    case '\t': fmt::format_to(ctx.out(), "\\t"); continue;
                    case '"' : fmt::format_to(ctx.out(), "\"" ); continue;
                    case '\\': fmt::format_to(ctx.out(), "\\" ); continue;
                    default:   fmt::format_to(ctx.out(), "{}", c);
                }
            // @formatter:on
            return fmt::format_to(ctx.out(), "\"");
        }
    };
}

namespace mpp::detail
{
    template <JsonSerializable T>
    void format_to_json(fmt::format_context& ctx, const T& value) { value.format_as_json(ctx); }

    template <typename T> requires std::integral<T> || std::floating_point<T>
    void format_to_json(fmt::format_context& ctx, const T value) { fmt::format_to(ctx.out(), "{}", value); }

    template <std::convertible_to<std::string_view> T>
    void format_to_json(fmt::format_context& ctx, const T& value)
    {
        fmt::format_to(ctx.out(), "{}", JsonQuoted{ value });
    }

    template <typename T>
    void format_to_json(fmt::format_context& ctx, const std::optional<T>& value)
    {
        if (value)
            format_to_json(ctx, *value);
        else
            fmt::format_to(ctx.out(), "null");
    }

    class JsonObjScope final // NOLINT(cppcoreguidelines-special-member-functions)
    {
    private:
        fmt::format_context& ctx_;
        bool first_ = true;

    public:
        explicit JsonObjScope(fmt::format_context& ctx): ctx_(ctx) { fmt::format_to(ctx_.out(), "{{"); }
        ~JsonObjScope() { fmt::format_to(ctx_.out(), "}}"); }

        template <typename T>
        void add_entry(const std::string_view key, const T& value)
        {
            fmt::format_to(ctx_.out(), (first_ ? "\"{}\":" : ",\"{}\":"), key);
            format_to_json(ctx_, value);
            first_ = false;
        }
    };

    class JsonArrScope final // NOLINT(cppcoreguidelines-special-member-functions)
    {
    private:
        fmt::format_context& ctx_;
        bool first_ = true;

    public:
        explicit JsonArrScope(fmt::format_context& ctx): ctx_(ctx) { fmt::format_to(ctx_.out(), "["); }
        ~JsonArrScope() { fmt::format_to(ctx_.out(), "]"); }

        template <typename T>
        void add_entry(const T& value)
        {
            fmt::format_to(ctx_.out(), first_ ? "" : ",");
            format_to_json(ctx_, value);
            first_ = false;
        }
    };

    template <typename Inv>
    struct FormatWrapper
    {
        const Inv& invocable;
    };
}

namespace fmt
{
    template <typename T>
    struct formatter<mpp::detail::JsonFormatWrapper<T>>
    {
        constexpr auto parse(format_parse_context& ctx) const { return ctx.begin(); }

        template <typename Ctx>
        auto format(const mpp::detail::JsonFormatWrapper<T>& value, Ctx& ctx)
        {
            mpp::detail::format_to_json(ctx, value.ref);
        }
    };

    template <typename Inv>
    struct formatter<mpp::detail::FormatWrapper<Inv>>
    {
        constexpr auto parse(format_parse_context& ctx) const { return ctx.begin(); }
        auto format(const mpp::detail::FormatWrapper<Inv>& value, format_context& ctx)
        {
            value.invocable(ctx);
            return ctx.out();
        }
    };
}

namespace mpp::detail
{
    template <typename Inv> requires std::invocable<const Inv&, fmt::format_context&>
    std::string perform_format(const Inv& invocable) { return fmt::format("{}", FormatWrapper<Inv>{ invocable }); }
}

// Deserialization
namespace mpp::detail
{
    struct FromJsonImpl
    {
        JsonRes json;

        template <JsonDeserializable T> explicit(false) operator T() const { return T::from_json(json); }

        template <std::integral T>
        explicit(false) operator T() const
        {
            if constexpr (std::same_as<T, bool>)
                return static_cast<bool>(json);
            else if constexpr (std::unsigned_integral<T>)
                return static_cast<T>(json.get_uint64());
            else // std::signed_integral<T>
                return static_cast<T>(json.get_int64());
        }

        template <std::floating_point T> explicit(false) operator T() const { return static_cast<T>(json.get_double()); }

        explicit(false) operator std::string() const { return std::string(json); }

        explicit(false) operator std::string_view() const { return std::string_view(json); }

        template <typename T>
        explicit(false) operator std::optional<T>() const
        {
            if (json.error() == simdjson::NO_SUCH_FIELD || json.is_null())
                return std::nullopt;
            else
                return operator T();
        }
    };

    inline auto from_json(const JsonRes json) { return FromJsonImpl{ json }; }
}
