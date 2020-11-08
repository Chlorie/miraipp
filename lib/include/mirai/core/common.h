#pragma once

#include <cstdint>

namespace mpp
{
    /// QQ 号
    struct [[nodiscard]] UserId final
    {
        int64_t id = 0; ///< 整数 id
        constexpr UserId() = default; ///< 构造一个空 QQ 号
        constexpr explicit UserId(const int64_t uid): id(uid) {} ///< 由一个整数 id 构造一个 QQ 号
        constexpr bool operator==(const UserId&) const = default; ///< 判断两个 QQ 号是否相等
        constexpr explicit operator bool() const noexcept { return id != 0; } ///< 当前 QQ 号是否有效
        constexpr bool valid() const noexcept { return id != 0; } ///< 当前 QQ 号是否有效
    };

    /// 群号
    struct [[nodiscard]] GroupId final
    {
        int64_t id = 0; ///< 整数 id
        constexpr GroupId() = default; ///< 构造一个空群号
        constexpr explicit GroupId(const int64_t gid): id(gid) {} ///< 由一个整数 id 构造一个群号
        constexpr bool operator==(const GroupId&) const = default; ///< 判断两个群号是否相等
        constexpr explicit operator bool() const noexcept { return id != 0; } ///< 当前群号是否有效
        constexpr bool valid() const noexcept { return id != 0; } ///< 当前群号是否有效
    };

    /// 消息号
    struct [[nodiscard]] MessageId final
    {
        int32_t id = 0; ///< 整数 id
        constexpr MessageId() = default; ///< 构造一个空消息号
        constexpr explicit MessageId(const int32_t mid): id(mid) {} ///< 由一个整数 id 构造一个消息号
        constexpr bool operator==(const MessageId&) const = default; ///< 判断两个消息号是否相等
    };

    /// 该命名空间中定义了一些用户自定义字面量 (UDL)
    namespace literals
    {
        /// 使用此 UDL 构造 QQ 号
        constexpr UserId operator""_uid(const unsigned long long value) noexcept { return UserId{ static_cast<int64_t>(value) }; }

        /// 使用此 UDL 构造群号
        constexpr GroupId operator""_gid(const unsigned long long value) noexcept { return GroupId{ static_cast<int64_t>(value) }; }
    }
}
