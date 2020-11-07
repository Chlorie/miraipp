#pragma once

#include <cstdint>

namespace mpp
{
    struct [[nodiscard]] UserId final
    {
        int64_t id = 0;
    };

    struct [[nodiscard]] GroupId final
    {
        int64_t id = 0;
    };

    namespace literals
    {
        constexpr UserId operator""_uid(const unsigned long long value) noexcept { return UserId{ static_cast<int64_t>(value) }; }
        constexpr GroupId operator""_gid(const unsigned long long value) noexcept { return GroupId{ static_cast<int64_t>(value) }; }
    }
}
