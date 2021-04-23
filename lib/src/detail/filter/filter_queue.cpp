#include "mirai/detail/filter/filter_queue.h"

#include <clu/scope.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

#include "mirai/core/bot.h"

namespace mpp::detail
{
    void FilterQueue::dequeue_with_lock(FilterNodeBase* node) noexcept
    {
        if (node == head_) head_ = node->next_;
        if (node == tail_) tail_ = node->prev_;
        if (node->prev_) node->prev_->next_ = node->next_;
        if (node->next_) node->next_->prev_ = node->prev_;
    }

    void FilterQueue::enqueue_with_lock(FilterNodeBase* node) noexcept
    {
        node->queue_ = this;
        if (!head_) head_ = node;
        if (tail_)
        {
            node->prev_ = tail_;
            tail_->next_ = node;
        }
        tail_ = node;
    }

    ex::task<bool> FilterQueue::filter_event(Event& ev)
    {
        co_await mutex_.async_lock();
        clu::scope_exit guard([this] { mutex_.unlock(); });
        auto* ptr = head_;
        while (ptr)
        {
            if (ptr->match_filter(ev))
            {
                dequeue_with_lock(ptr);
                ptr->active_ = false;
                ptr->resume_on_scheduler_with(std::move(ev));
                co_return true;
            }
            ptr = ptr->next_;
        }
        co_return false;
    }

    ex::task<void> FilterQueue::cancel_all()
    {
        co_await mutex_.async_lock();
        clu::scope_exit guard([this] { mutex_.unlock(); });
        while (head_)
        {
            auto* next = head_->next_;
            head_->active_ = false;
            head_->resume_on_scheduler_with(std::nullopt);
            head_ = next;
        }
        tail_ = nullptr;
    }

    ex::task<void> FilterNodeBase::cancel_task()
    {
        co_await queue_->get_mutex().async_lock();

        // Still active, the event loop haven't matched this filter
        // successfully while we are trying to acquire the lock
        if (active_)
        {
            queue_->dequeue_with_lock(this); // noexcept
            queue_->get_mutex().unlock();
            resume_with(std::nullopt); // We are already on queue's scheduler
            co_return;
        }

        queue_->get_mutex().unlock();
    }

    void FilterNodeBase::request_cancel()
    {
        queue_->get_async_scope().spawn(
            cancel_task(), queue_->get_scheduler());
    }

    void FilterNodeBase::resume_on_scheduler_with(std::optional<Event> ev)
    {
        // Normal function not coroutine,
        // post it directly instead of through async_scope to avoid one allocation
        post(queue_->get_scheduler().io_context(),
            [this, ev = std::move(ev)]() mutable { resume_with(std::move(ev)); });
    }
}
