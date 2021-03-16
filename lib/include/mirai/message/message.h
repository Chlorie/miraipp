#pragma once

#include <vector>
#include <ranges>
#include <clu/concepts.h>

#include "segment.h"

namespace mpp
{
    /// 表示一条可能由多个消息段 \c Segment 组成的完整消息
    class Message final
    {
    public:
        using iterator = std::vector<Segment>::iterator;
        using const_iterator = std::vector<Segment>::const_iterator;
        using reverse_iterator = std::vector<Segment>::reverse_iterator;
        using const_reverse_iterator = std::vector<Segment>::const_reverse_iterator;

    private:
        std::vector<Segment> vec_;

        bool starts_with_sv(std::string_view sv) const;
        bool ends_with_sv(std::string_view sv) const;
        bool contains_sv(std::string_view sv) const;

    public:
        template <typename... Ts> requires (std::convertible_to<Ts&&, Segment> && ...)
        explicit(false) Message(Ts&&... segments)
        {
            vec_.reserve(sizeof...(segments));
            (vec_.emplace_back(std::forward<Ts>(segments)), ...);
        }

        /// \defgroup MsgIdx
        /// \{
        Segment& at(const size_t index) { return vec_.at(index); }
        const Segment& at(const size_t index) const { return vec_.at(index); } ///< 访问指定的消息段，同时进行越界检查
        Segment& operator[](const size_t index) { return vec_[index]; }
        const Segment& operator[](const size_t index) const { return vec_[index]; } ///< 访问指定的消息段
        Segment& front() { return vec_.front(); }
        const Segment& front() const { return vec_.front(); } ///< 获取第一个消息段
        Segment& back() { return vec_.back(); }
        const Segment& back() const { return vec_.back(); } ///< 获取最后一个消息段
        Segment* data() { return vec_.data(); }
        const Segment* data() const { return vec_.data(); } ///< 获取指向内存中数组第一个元素的指针
        /// \}

        std::vector<Segment>& get_vector() & { return vec_; }
        const std::vector<Segment>& get_vector() const & { return vec_; }
        std::vector<Segment>&& get_vector() && { return std::move(vec_); }
        const std::vector<Segment>&& get_vector() const && { return std::move(vec_); } // NOLINT(performance-move-const-arg)

        auto begin() { return vec_.begin(); }
        auto begin() const { return vec_.begin(); }
        auto cbegin() const { return vec_.cbegin(); }
        auto end() { return vec_.end(); }
        auto end() const { return vec_.end(); }
        auto cend() const { return vec_.cend(); }
        auto rbegin() { return vec_.rbegin(); }
        auto rbegin() const { return vec_.rbegin(); }
        auto crbegin() const { return vec_.crbegin(); }
        auto rend() { return vec_.rend(); }
        auto rend() const { return vec_.rend(); }
        auto crend() const { return vec_.crend(); }

        bool empty() const noexcept { return vec_.empty(); } ///< 判断该消息是否为空
        size_t size() const noexcept { return vec_.size(); } ///< 获取该消息中消息段的个数
        void reserve(const size_t new_capacity) { vec_.reserve(new_capacity); } ///< 预分配内存空间
        size_t capacity() const noexcept { return vec_.capacity(); } ///< 获取该消息在不进行额外内存分配的情况下能容纳多少消息段
        void shrink_to_fit() { return vec_.shrink_to_fit(); } ///< 缩减消息占用内存以恰好容纳当前所有消息段

        template <typename T> requires std::convertible_to<T&&, Segment>
        Message& operator+=(T&& segment)
        {
            vec_.emplace_back(std::forward<T>(segment));
            return *this;
        }

        /// \defgroup MsgCat
        /// \{

        /// 在本消息末尾连接另一条消息
        Message& operator+=(const Message& other)
        {
            vec_.insert(vec_.end(), other.begin(), other.end());
            return *this;
        }

        /// 在本消息末尾连接另一条消息
        Message& operator+=(Message&& other)
        {
            vec_.insert(vec_.end(),
                std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()));
            return *this;
        }

        /// 连接一条消息与一个消息段
        template <typename T> requires std::convertible_to<T&&, Segment>
        friend Message operator+(Message msg, T&& segment) { return msg += std::forward<T>(segment); }

        /// 连接两条消息
        template <clu::forwarding<Message> T>
        friend Message operator+(Message lhs, T&& rhs) { return lhs += std::forward<T>(rhs); }
        /// \}

        /// \defgroup MsgCmp
        /// \{
        bool operator==(const Message& other) const noexcept = default;
        bool operator==(const Segment& other) const noexcept { return size() == 1 && front() == other; }

        template <std::convertible_to<std::string_view> T>
        bool operator==(const T& other) const noexcept
        {
            if (vec_.size() != 1) return false;
            if (const auto* ptr = vec_.front().get_if<Plain>())
                return ptr->text == other;
            return false;
        }

        template <ConcreteSegment T>
        bool operator==(const T& other) const noexcept
        {
            if constexpr (std::is_same_v<T, Plain>)
                return *this == other.text;
            else
                return size() == 1 && front() == other;
        }
        /// \}

        void clear() { vec_.clear(); } ///< 清空当前消息
        void swap(Message& other) noexcept { vec_.swap(other.vec_); } ///< 将当前消息与另一个消息互换
        friend void swap(Message& lhs, Message& rhs) noexcept { lhs.swap(rhs); } ///< 将两条消息互换

        void collapse_adjacent_text(); ///< 将相邻的纯文本消息段合并成一个消息段

        template <std::convertible_to<std::string_view> T>
        bool starts_with(const T& prefix) const { return starts_with_sv(prefix); }

        template <ConcreteSegment T>
        bool starts_with(const T& prefix) const
        {
            if constexpr (std::is_same_v<T, Plain>)
                return starts_with_sv(prefix.text);
            else
                return !vec_.empty() && vec_.front() == prefix;
        }

        bool starts_with(const Segment& prefix) const;
        bool starts_with(const Message& prefix) const;

        /// 获取遍历该消息中所有特定类型消息段的范围
        template <ConcreteSegment T>
        auto collect() const
        {
            return vec_
                | std::views::filter([](const Segment& seg) { return seg.type() == T::type; })
                | std::views::transform([](const Segment& seg) -> auto&& { return seg.get<T>(); });
        }

        std::string collect_text() const; ///< 获取消息中所有纯文本消息段连接而成的字符串

        void format_to(fmt::format_context& ctx) const;
        void format_as_json(fmt::format_context& ctx) const;
        static Message from_json(detail::JsonElem json);
    };
}
