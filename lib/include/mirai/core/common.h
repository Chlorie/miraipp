#pragma once

#include <clu/hash.h>

namespace mpp
{
    /// QQ 号
    struct [[nodiscard]] UserId final
    {
        int64_t id = 0; ///< 整数 id
        constexpr UserId() = default; ///< 构造一个空 QQ 号
        constexpr explicit UserId(const int64_t uid): id(uid) {} ///< 由一个整数 id 构造一个 QQ 号
        constexpr friend bool operator==(UserId, UserId) = default; ///< 判断两个 QQ 号是否相等
        constexpr friend auto operator<=>(UserId, UserId) = default; ///< 比较两个 QQ 号的值大小
        constexpr explicit operator bool() const noexcept { return id != 0; } ///< 当前 QQ 号是否有效
        constexpr bool valid() const noexcept { return id != 0; } ///< 当前 QQ 号是否有效
    };

    /// 群号
    struct [[nodiscard]] GroupId final
    {
        int64_t id = 0; ///< 整数 id
        constexpr GroupId() = default; ///< 构造一个空群号
        constexpr explicit GroupId(const int64_t gid): id(gid) {} ///< 由一个整数 id 构造一个群号
        constexpr friend bool operator==(GroupId, GroupId) = default; ///< 判断两个群号是否相等
        constexpr friend auto operator<=>(GroupId, GroupId) = default; ///< 比较两个群号的值大小
        constexpr explicit operator bool() const noexcept { return id != 0; } ///< 当前群号是否有效
        constexpr bool valid() const noexcept { return id != 0; } ///< 当前群号是否有效
    };

    /// 从群聊中发起临时会话的目标 id，包含 QQ 号和群号
    struct [[nodiscard]] TempId final
    {
        UserId uid{}; ///< 临时会话的对象 QQ 号
        GroupId gid{}; ///< 发起临时会话的群号
        constexpr friend bool operator==(TempId, TempId) = default; ///< 判断两个临时会话目标是否相等
        constexpr friend auto operator<=>(TempId, TempId) = default; ///< 比较两个临时会话目标值的序关系
    };

    /// 消息号
    struct [[nodiscard]] MessageId final
    {
        int32_t id = 0; ///< 整数 id
        constexpr MessageId() = default; ///< 构造一个空消息号
        constexpr explicit MessageId(const int32_t mid): id(mid) {} ///< 由一个整数 id 构造一个消息号
        constexpr friend bool operator==(MessageId, MessageId) = default; ///< 判断两个消息号是否相等
        constexpr friend auto operator<=>(MessageId, MessageId) = default; ///< 比较两个消息号的值大小
    };

    /// 消息发送对象的类别
    enum class TargetType : uint8_t { friend_, group, temp };

    constexpr std::string_view to_string_view(const TargetType type)
    {
        // @formatter:off
        switch (type)
        {
            case TargetType::friend_: return "friend";
            case TargetType::group:   return "group";
            case TargetType::temp:    return "temp";
        }
        // @formatter:on
        return {};
    }

    /// 该命名空间中定义了一些用户自定义字面量 (UDL)
    namespace literals
    {
        /// 使用此 UDL 构造 QQ 号
        constexpr UserId operator""_uid(const unsigned long long value) noexcept { return UserId{ static_cast<int64_t>(value) }; }

        /// 使用此 UDL 构造群号
        constexpr GroupId operator""_gid(const unsigned long long value) noexcept { return GroupId{ static_cast<int64_t>(value) }; }
    }
}

namespace std
{
    template <>
    struct hash<mpp::UserId>
    {
        size_t operator()(const mpp::UserId id) const noexcept
        {
            return std::hash<int64_t>{}(id.id);
        }
    };

    template <>
    struct hash<mpp::GroupId>
    {
        size_t operator()(const mpp::GroupId id) const noexcept
        {
            return std::hash<int64_t>{}(id.id);
        }
    };

    template <>
    struct hash<mpp::TempId>
    {
        size_t operator()(const mpp::TempId id) const noexcept
        {
            size_t hash = 0;
            clu::hash_combine(hash, id.uid);
            clu::hash_combine(hash, id.gid);
            return hash;
        }
    };

    template <>
    struct hash<mpp::MessageId>
    {
        size_t operator()(const mpp::MessageId id) const noexcept
        {
            return std::hash<int32_t>{}(id.id);
        }
    };
}
