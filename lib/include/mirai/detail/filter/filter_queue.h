#pragma once

#include <optional>

#include <unifex/async_mutex.hpp>
#include <unifex/async_scope.hpp>

#include "../../core/net_client.h"
#include "../../event/event.h"

namespace mpp
{
    class Bot;
}

namespace mpp::detail
{
    namespace ex = unifex;
    using Scheduler = net::Client::Scheduler;
    class FilterNodeBase;

    // Intrinsic queue for event filters
    MPP_SUPPRESS_EXPORT_WARNING
    class MPP_API FilterQueue final
    {
    private:
        FilterNodeBase* head_ = nullptr;
        FilterNodeBase* tail_ = nullptr;
        Scheduler sch_;
        ex::async_mutex mutex_;
        ex::async_scope scope_;

    public:
        explicit FilterQueue(const Scheduler sch): sch_(sch) {}

        Scheduler get_scheduler() const noexcept { return sch_; }
        ex::async_mutex& get_mutex() noexcept { return mutex_; }
        ex::async_scope& get_async_scope() noexcept { return scope_; }

        void enqueue_with_lock(FilterNodeBase* node) noexcept;
        void dequeue_with_lock(FilterNodeBase* node) noexcept;
        ex::task<bool> filter_event(Event& ev); // may take ownership
        ex::task<void> cancel_all();

        auto cleanup() noexcept { return scope_.cleanup(); } // TODO: cancel_all before cleanup
    };

    class MPP_API FilterNodeBase
    {
        friend FilterQueue;
    private:
        FilterQueue* queue_ = nullptr;
        FilterNodeBase* prev_ = nullptr;
        FilterNodeBase* next_ = nullptr;
        bool active_ = true;

        ex::task<void> cancel_task();
        
    protected:
        ~FilterNodeBase() noexcept = default;

    public:
        FilterNodeBase() = default;
        void request_cancel();
        void resume_on_scheduler_with(std::optional<Event> ev);

        virtual bool match_filter(const Event& ev) = 0;
        virtual void resume_with(std::optional<Event> ev) = 0;
    };
    MPP_RESTORE_EXPORT_WARNING
}
