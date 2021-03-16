#pragma once

#include <clu/type_traits.h>

#include "segment_types.h"

namespace mpp
{
    /// AnySegment 类存储任意一种消息段类型的对象（类型擦除类）
    class Segment final
    {
    private:
        struct Model // NOLINT(cppcoreguidelines-special-member-functions)
        {
            virtual ~Model() noexcept = default;
            virtual std::unique_ptr<Model> clone() const = 0;
            virtual SegmentType type() const noexcept = 0;
            virtual bool operator==(const Model&) const noexcept = 0;
            virtual void format_to(fmt::format_context& ctx) const = 0;
            virtual void format_as_json(fmt::format_context& ctx) const = 0;
        };

        template <typename T>
        class ModelImpl final : public Model
        {
        private:
            T data_;

        public:
            explicit ModelImpl(const T& data): data_(data) {}
            explicit ModelImpl(T&& data): data_(std::move(data)) {}

            T& get() noexcept { return data_; }

            std::unique_ptr<Model> clone() const override { return std::make_unique<ModelImpl>(data_); }
            SegmentType type() const noexcept override { return T::type; }
            void format_to(fmt::format_context& ctx) const override { data_.format_to(ctx); }
            void format_as_json(fmt::format_context& ctx) const override { data_.format_as_json(ctx); }
            bool operator==(const Model& base) const noexcept override
            {
                return base.type() == T::type
                    && static_cast<const ModelImpl<T>&>(base).data_ == data_;
            }
        };

        std::unique_ptr<Model> impl_;

        template <ConcreteSegment T, typename Self>
        static decltype(auto) get_impl(Self&& self)
        {
            if (self.type() != T::type)
                throw std::runtime_error("消息段类型不匹配");
            T& ref = static_cast<ModelImpl<T>&>(*self.impl_).get();
            return static_cast<clu::copy_cvref_t<Self&&, T>>(ref);
        }

        template <ConcreteSegment T>
        T* get_if_impl() const noexcept
        {
            if (type() == T::type)
                return &static_cast<ModelImpl<T>*>(impl_.get())->get();
            else
                return nullptr;
        }

    public:
        template <typename T> requires ConcreteSegment<std::remove_cvref_t<T>>
        explicit(false) Segment(T&& segment): // NOLINT(bugprone-forwarding-reference-overload)
            impl_(std::make_unique<ModelImpl<std::remove_cvref_t<T>>>(std::forward<T>(segment))) {}

        explicit(false) Segment(const std::string& text): Segment(Plain{ text }) {}
        explicit(false) Segment(std::string&& text): Segment(Plain{ std::move(text) }) {}
        explicit(false) Segment(const char* text): Segment(Plain{ text }) {}
        template <typename T> requires (std::convertible_to<T, std::string_view> && !std::convertible_to<T, const char*>)
        explicit(false) Segment(const T& text): Segment(Plain{ std::string(text) }) {}

        ~Segment() noexcept = default;
        Segment(Segment&&) noexcept = default;
        Segment& operator=(Segment&&) noexcept = default;

        Segment(const Segment& other): impl_(other.impl_->clone()) {} ///< 复制一个消息段

        Segment& operator=(const std::string& text) { return *this = Plain{ text }; }
        Segment& operator=(std::string&& text) { return *this = Plain{ std::move(text) }; }
        Segment& operator=(const char* text) { return *this = Plain{ text }; }
        template <typename T> requires (std::convertible_to<T, std::string_view> && !std::convertible_to<T, const char*>)
        Segment& operator=(const T& text) { return *this = Plain{ std::string(text) }; }

        template <typename T> requires ConcreteSegment<std::remove_cvref_t<T>>
        Segment& operator=(T&& segment)
        {
            impl_ = std::make_unique<ModelImpl<std::remove_cvref_t<T>>>(std::forward<T>(segment));
            return *this;
        }

        Segment& operator=(const Segment& other)
        {
            if (&other == this) return *this;
            impl_ = other.impl_->clone();
            return *this;
        }

        SegmentType type() const noexcept { return impl_->type(); }

        bool operator==(const Segment& other) const noexcept { return *impl_ == *other.impl_; }

        template <std::convertible_to<std::string_view> T>
        bool operator==(const T& other) const noexcept
        {
            if (const std::string_view sv = other;
                const auto* ptr = get_if<Plain>())
                return ptr->text == sv;
            return false;
        }

        template <ConcreteSegment T>
        bool operator==(const T& other) const noexcept
        {
            if (const T* ptr = get_if<T>())
                return *ptr == other;
            return false;
        }

        template <ConcreteSegment T> T& get() & { return get_impl<T>(*this); }
        template <ConcreteSegment T> const T& get() const & { return get_impl<T>(*this); }
        template <ConcreteSegment T> T&& get() && { return get_impl<T>(std::move(*this)); }
        template <ConcreteSegment T> const T&& get() const && { return get_impl<T>(std::move(*this)); }

        template <ConcreteSegment T> T* get_if() noexcept { return get_if_impl<T>(); }
        template <ConcreteSegment T> const T* get_if() const noexcept { return get_if_impl<T>(); }

        void format_to(fmt::format_context& ctx) const { impl_->format_to(ctx); }
        void format_as_json(fmt::format_context& ctx) const { impl_->format_as_json(ctx); }
        static Segment from_json(detail::JsonElem json);
    };
}
