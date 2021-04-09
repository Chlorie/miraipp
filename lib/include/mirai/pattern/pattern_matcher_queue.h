#pragma once

#include <vector>
#include <clu/scope.h>
#include <clu/coroutine/async_mutex.h>
#include <clu/coroutine/task.h>
#include <unifex/task.hpp>
#include <unifex/async_mutex.hpp>

#include "pattern.h"
#include "../event/event.h"

namespace mpp
{
    class PatternMatcherQueue final
    {
    private:
        class ItemModel
        {
        private:
            std::atomic_bool done_{ false };

        public:
            virtual ~ItemModel() noexcept = default;
            bool done() const noexcept { return done_.load(std::memory_order_relaxed); }
            bool set_done() noexcept { return done_.exchange(true, std::memory_order_release); }
            virtual bool match(const Event& ev) = 0;
        };

        template <ConcreteEvent E, PatternFor<E>... Ps>
        class ItemImpl final : public ItemModel
        {
        public:
            class Awaitable
            {
                friend class ItemImpl;

            private:
                PatternMatcherQueue* queue_;
                std::unique_ptr<ItemImpl> item_;
                ItemImpl* enqueued_item_ = nullptr;

                struct Awaiter final
                {
                    Awaitable& parent;

                    bool await_ready() const noexcept { return false; }

                    void await_suspend(const std::coroutine_handle<> handle) const
                    {
                        parent.enqueued_item_->handle_ = handle;
                        parent.queue_->mutex_.unlock();
                    }

                    std::optional<E> await_resume() const noexcept { return std::move(parent.enqueued_item_->event_); }
                };

            public:
                template <typename... Qs>
                explicit Awaitable(PatternMatcherQueue* queue, Qs&&... ptns):
                    queue_(queue),
                    item_(std::make_unique<ItemImpl>(std::forward<Qs>(ptns)...)),
                    enqueued_item_(item_.get()) {}

                clu::task<std::optional<E>> operator co_await()
                {
                    co_await queue_->mutex_.async_lock();
                    {
                        auto _ = clu::scope_fail([&] { queue_->mutex_.unlock(); });
                        queue_->queue_.push_back(std::move(item_));
                    }
                    enqueued_item_->awaitable_.store(this, std::memory_order_release);
                    co_return co_await Awaiter(*this);
                }

                void cancel()
                {
                    if (!enqueued_item_->set_done())
                        enqueued_item_->handle_.resume();
                }
            };

        private:
            std::optional<E> event_;
            std::coroutine_handle<> handle_{};
            std::atomic<Awaitable*> awaitable_;
            std::tuple<Ps...> patterns_;

        public:
            template <typename... Qs>
            explicit ItemImpl(Qs&&... ptns): patterns_(std::forward<Qs>(ptns)...) {}

            bool match(const Event& ev) override
            {
                if (const E* ptr = ev.get_if<E>();
                    ptr && std::apply([&](const Ps&... ps) { return match_with(*ptr, ps...); }, patterns_))
                {
                    event_ = *ptr;
                    if (!set_done())
                    {
                        handle_.resume();
                        return true;
                    }
                }
                return false;
            }
        };

        clu::async_mutex mutex_;
        std::vector<std::unique_ptr<ItemModel>> queue_;

    public:
        template <ConcreteEvent E, PatternFor<E>... Ps>
        using Awaitable = typename ItemImpl<E, Ps...>::Awaitable;

        template <ConcreteEvent E, typename... Ps> requires (PatternFor<std::remove_cv_t<Ps>, E> && ...)
        Awaitable<E, std::remove_cv_t<Ps>...> enqueue_async(Ps&&... patterns)
        {
            return Awaitable<E, std::remove_cv_t<Ps>...>(this, std::forward<Ps>(patterns)...);
        }

        clu::task<bool> match_event_async(const Event& ev);
    };
}
