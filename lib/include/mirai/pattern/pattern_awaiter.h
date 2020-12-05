#pragma once

#include <vector>
#include <clu/scope.h>
#include <clu/coroutine/async_mutex.h>
#include <clu/coroutine/task.h>

#include "pattern.h"

namespace mpp
{
    class PatternMatcherQueue final
    {
    private:
        class ItemModel // NOLINT(cppcoreguidelines-special-member-functions)
        {
        private:
            std::atomic_bool done_{ false };

        protected:
            virtual bool do_match(const Event& ev) = 0;

        public:
            virtual ~ItemModel() noexcept = default;
            bool done() const noexcept { return done_.load(std::memory_order_relaxed); }
            bool match(const Event& ev) { return do_match(ev); }
        };

        template <EventComponent E, PatternFor<E>... Ps>
        class ItemImpl final : public ItemModel
        {
        public:
            class Awaitable
            {
                friend class ItemImpl;

            private:
                std::coroutine_handle<> handle_{};
                PatternMatcherQueue* queue_;
                std::unique_ptr<ItemImpl> item_;
                ItemModel* enqueued_item_ = nullptr;

                struct Awaiter final
                {
                    Awaitable& parent;

                    bool await_ready() const noexcept { return false; }

                    void await_suspend(const std::coroutine_handle<> handle) const
                    {
                        parent.handle_ = handle;
                        parent.queue_->mutex_.unlock();
                    }

                    const E* await_resume() const noexcept { return static_cast<ItemImpl*>(parent.enqueued_item_)->ev_; }
                };

            public:
                template <typename... Qs>
                explicit Awaitable(PatternMatcherQueue* queue, Qs&&... ptns):
                    queue_(queue), item_(std::make_unique<ItemImpl>(std::forward<Qs>(ptns)...)) {}

                clu::task<const E*> operator co_await()
                {
                    co_await queue_->mutex_.async_lock();
                    auto _ = clu::scope_fail([&] { queue_->mutex_.unlock(); });
                    ItemImpl* item = item_.get();
                    queue_->queue_.push_back(std::move(item_));
                    enqueued_item_ = queue_->queue_.back().get();
                    item->awaitable_.store(this, std::memory_order_release);
                    co_return co_await Awaiter(*this);
                }

                // void cancel()
                // {
                //     
                // }
            };

        private:
            const E* ev_ = nullptr;
            std::atomic<Awaitable*> awaitable_;
            std::tuple<Ps...> patterns_;

        protected:
            bool do_match(const Event& ev) override
            {
                const E* ptr = ev.get_if<E>();
                if (ptr && std::apply([&](const Ps&... ps) { return match_with(*ptr, ps...); }, patterns_))
                {
                    ev_ = ptr;
                    awaitable_.load(std::memory_order_acquire)->handle_.resume();
                    return true;
                }
                return false;
            }

        public:
            template <typename... Qs>
            explicit ItemImpl(Qs&&... ptns): patterns_(std::forward<Qs>(ptns)...) {}
        };

        clu::async_mutex mutex_;
        std::vector<std::unique_ptr<ItemModel>> queue_;

    public:
        template <EventComponent E, PatternFor<E>... Ps>
        using Awaitable = typename ItemImpl<E, Ps...>::Awaitable;

        template <EventComponent E, typename... Ps> requires (PatternFor<std::remove_cv_t<Ps>, E> && ...)
        Awaitable<E, std::remove_cv_t<Ps>...> async_enqueue(Ps&&... patterns)
        {
            return Awaitable<E, std::remove_cv_t<Ps>...>(this, std::forward<Ps>(patterns)...);
        }

        clu::task<bool> async_match_event(const Event& ev);
    };
}
