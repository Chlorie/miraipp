#pragma once

#include <memory>
#include <clu/coroutine/coroutine_scope.h>
#include <clu/coroutine/task.h>

#include "common.h"
#include "../detail/net_client.h"

namespace mpp
{
    class Bot final
    {
    public:
        using Clock = std::chrono::steady_clock;

    private:
        detail::NetClient net_client_;
        clu::coroutine_scope coro_scope_;

    public:
        /**
         * \brief 创建新的 bot 对象并同步地连接到指定端点
         * \param host 要连接到端点的主机，默认为 127.0.0.1
         * \param port 要连接到端点的端口，默认为 80
         */
        explicit Bot(std::string_view host = "127.0.0.1", std::string_view port = "80");

        /**
         * \brief 异步地获取 mirai-api-http 版本号
         * \return （异步）包含 mirai-api-http 版本的字符串
         */
        clu::task<std::string> async_get_version();

        /**
         * \brief 异步地授权并校验一个 session
         * \param auth_key mirai-http-server 中指定的授权用 key
         * \param id 对应的已登录 bot 的 QQ 号
         * \return （异步）无返回值
         */
        clu::task<void> async_auth(std::string_view auth_key, UserId id);

        /**
         * \brief 异步等待直到某时间点
         * \param tp 等待的到期时间
         * \return （异步）无返回值
         */
        auto async_wait(const Clock::time_point tp) { return net_client_.async_wait(tp); }

        /**
         * \brief 异步等待给定时长
         * \param dur 等待的时长
         * \return （异步）无返回值
         */
        auto async_wait(const Clock::duration dur) { return net_client_.async_wait(Clock::now() + dur); }

        auto& coroutine_scope() { return coro_scope_; } ///< 获取该 bot 对应的协程作用域

        /// 在 bot 对应的协程作用域上等待一个可等待体
        template <clu::awaitable T>
        void spawn(T&& awaitable) { coro_scope_.spawn(std::forward<T>(awaitable)); }

        void run() { net_client_.run(); } ///< 阻塞当前线程执行 I/O，直到所有 I/O 处理完成，可从多个线程调用
    };
}
