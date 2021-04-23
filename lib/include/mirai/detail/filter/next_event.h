#pragma once

#include <coroutine>

#include "filter_queue.h"

namespace mpp::detail
{
    template <typename E, typename F>
    class NextEventNode final : public FilterNodeBase
    {
    private:
        FilterQueue& queue_;
        [[no_unique_address]] F filter_;
        std::coroutine_handle<> handle_;
        std::optional<Event> res_;

    public:
        NextEventNode(FilterQueue& queue, const F& filter):
            queue_(queue), filter_(filter) {}

        bool match_filter(const Event& ev) override
        {
            if (const E* ptr = ev.get_if<E>())
                return filter_(*ptr);
            return false;
        }

        void resume_with(std::optional<Event> ev) override
        {
            res_ = std::move(ev);
            handle_.resume();
        }

        ex::task<E> wait()
        {
            co_await queue_.get_mutex().async_lock();
            co_await call_and_suspend([this](const std::coroutine_handle<> handle)
            {
                handle_ = handle;
                queue_.enqueue_with_lock(this);
                queue_.get_mutex().unlock();
            });
            if (!res_) co_await ex::stop();
            co_return std::move(*res_).get<E>();
        }
    };
}
