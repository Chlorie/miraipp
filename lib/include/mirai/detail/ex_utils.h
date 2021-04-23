#pragma once

#include <unifex/inplace_stop_token.hpp>
#include <unifex/just.hpp>
#include <unifex/transform.hpp>
#include <unifex/just_done.hpp>
#include <unifex/stop_if_requested.hpp>

namespace mpp::detail
{
    namespace ex = unifex;

    inline constexpr auto true_predicate = [](const auto&) { return true; };

    template <typename T, ex::callable C>
    auto make_stop_callback(const T& token, C&& callback)
    {
        using callback_t = typename T::template callback_type<C>;
        return callback_t(token, std::forward<C>(callback));
    }

    template <ex::callable C>
    auto lazy(C&& callable) { return ex::just() | ex::transform(std::forward<C>(callable)); }

    inline auto get_coroutine_handle()
    {
        struct Impl final
        {
            std::coroutine_handle<> handle;
            bool await_ready() const noexcept { return false; }
            void await_suspend(const std::coroutine_handle<> hdl)
            {
                handle = hdl;
                handle.resume();
            }
            std::coroutine_handle<> await_resume() const noexcept { return handle; }
        };
        return Impl();
    }

    template <ex::callable<std::coroutine_handle<>> C>
    auto call_and_suspend(const C& callable)
    {
        struct Impl final
        {
            C callable;
            bool await_ready() const noexcept { return false; }
            void await_suspend(const std::coroutine_handle<> handle) { callable(handle); }
            void await_resume() const noexcept {}
        };
        return Impl(callable);
    }
}
