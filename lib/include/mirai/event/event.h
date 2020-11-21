#pragma once

#include <memory>
#include <clu/type_traits.h>

#include "event_types.h"

namespace mpp
{
    class Event final
    {
    private:
        struct EventModel // NOLINT(cppcoreguidelines-special-member-functions)
        {
            virtual ~EventModel() noexcept = default;
            virtual std::unique_ptr<EventModel> clone() const = 0;
            virtual EventType type() const noexcept = 0;
            virtual Bot& bot() const noexcept = 0;
            virtual EventBase& event_base() noexcept = 0; 
        };

        template <typename T>
        class EventModelImpl final : public EventModel
        {
        private:
            T data_;

        public:
            explicit EventModelImpl(const T& data): data_(data) {}
            explicit EventModelImpl(T&& data): data_(std::move(data)) {}

            T& get() { return data_; }

            std::unique_ptr<EventModel> clone() const override { return std::make_unique<EventModelImpl>(data_); }
            EventType type() const noexcept override { return T::type; }
            Bot& bot() const noexcept override { return data_.bot(); }
            EventBase& event_base() noexcept override { return static_cast<EventBase&>(data_); }
        };

        std::unique_ptr<EventModel> impl_;

        template <EventComponent T, typename Self>
        static decltype(auto) get_impl(Self&& self)
        {
            if (self.type() != T::type)
                throw std::runtime_error("事件类型不匹配");
            T& ref = static_cast<EventModelImpl<T>&>(*self.impl_).get();
            return static_cast<clu::copy_cvref_t<Self&&, T>>(ref);
        }

        template <EventComponent T> T* get_if_impl() const
        {
            if (type() == T::type)
                return &static_cast<EventModelImpl<T>*>(impl_.get())->get();
            else
                return nullptr;
        }

        friend class Bot;
        EventBase& event_base() const noexcept { return impl_->event_base(); }

    public:
        template <typename T> requires EventComponent<std::remove_cvref_t<T>>
        explicit(false) Event(T&& ev): // NOLINT(bugprone-forwarding-reference-overload)
            impl_(std::make_unique<EventModelImpl<std::remove_cvref_t<T>>>(std::forward<T>(ev))) {}

        ~Event() noexcept = default;
        Event(Event&&) noexcept = default;
        Event& operator=(Event&&) noexcept = default;

        Event(const Event& other): impl_(other.impl_->clone()) {}

        Event& operator=(const Event& other)
        {
            if (&other == this) return *this;
            impl_ = other.impl_->clone();
            return *this;
        }

        EventType type() const noexcept { return impl_->type(); }

        template <EventComponent T> T& get() & { return get_impl<T>(*this); }
        template <EventComponent T> const T& get() const & { return get_impl<T>(*this); }
        template <EventComponent T> T&& get() && { return get_impl<T>(std::move(*this)); }
        template <EventComponent T> const T&& get() const && { return get_impl<T>(std::move(*this)); }
        template <EventComponent T> explicit(false) operator T&() & { return get_impl<T>(*this); }
        template <EventComponent T> explicit(false) operator const T&() const & { return get_impl<T>(*this); }
        template <EventComponent T> explicit(false) operator T&&() && { return get_impl<T>(std::move(*this)); }
        template <EventComponent T> explicit(false) operator const T&&() const && { return get_impl<T>(std::move(*this)); }

        template <EventComponent T> T* get_if() { return get_if_impl<T>(); }
        template <EventComponent T> const T* get_if() const { return get_if_impl<T>(); }

        Bot& bot() const { return impl_->bot(); }

        static Event from_json(detail::JsonElem json);
    };
}
