#pragma once

#include <iostream>
#include <clu/coroutine/coroutine_scope.h>
#include <clu/coroutine/task.h>
#include <clu/optional_ref.h>

#include "common.h"
#include "../message/message.h"
#include "../detail/net_client.h"

namespace mpp
{
    /// Mirai++ 中功能的核心类型，代表 mirai-api-http 中的一个会话。此类不可复制或移动。
    class Bot final
    {
    public:
        using Clock = std::chrono::steady_clock;

    private:
        detail::NetClient net_client_;
        clu::coroutine_scope coro_scope_;
        UserId bot_id_;
        std::string sess_key_;

        template <clu::awaitable T>
        static clu::task<> run_catching(T awaitable)
        {
            try { co_await awaitable; }
            catch (const std::exception& exc) { std::cerr << "Caught exception: " << exc.what() << '\n'; }
            catch (...) { std::cerr << "Unknown exception caught\n"; }
        }

        std::string release_body() const;

    public:
        /**
         * \brief 创建新的 bot 对象并同步地连接到指定端点
         * \param host 要连接到端点的主机，默认为 127.0.0.1
         * \param port 要连接到端点的端口，默认为 8080
         */
        explicit Bot(const std::string_view host = "127.0.0.1", const std::string_view port = "8080"):
            net_client_(host, port) {}

        ~Bot() noexcept; ///< 销毁当前 bot 对象，释放未结束的对话并关闭所有连接

        Bot(const Bot&) = delete;
        Bot(Bot&&) = delete;
        Bot& operator=(const Bot&) = delete;
        Bot& operator=(Bot&&) = delete;

        bool authorized() const noexcept { return bot_id_.valid(); } ///< 当前 Bot 是否已授权

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
        clu::task<> async_auth(std::string_view auth_key, UserId id);

        void release(); ///< 同步地释放当前会话，若当前对象析构时会话仍在开启状态则会调用此函数

        /**
         * \brief 异步地释放当前会话
         * \return （异步）无返回值
         */
        clu::task<> async_release();

        /**
         * \brief 异步地向给定好友发送消息
         * \param id 好友的 QQ 号
         * \param message 消息内容
         * \param quote （可选）要引用回复的消息 id
         * \return （异步）已发送消息的 id，用于撤回和引用回复
         */
        clu::task<MessageId> async_send_message(UserId id, const Message& message, clu::optional_param<MessageId> quote = {});

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

        /// 在 bot 对应的协程作用域上等待一个可等待体，若有未处理的异常则调用 std::terminate
        template <clu::awaitable T>
        void spawn(T&& awaitable) { coro_scope_.spawn(std::forward<T>(awaitable)); }

        /// 在 bot 对应的协程作用域上等待一个可等待体，将未处理异常的信息输出到 stderr
        template <clu::awaitable T>
        void spawn_catching(T&& awaitable) { coro_scope_.spawn(run_catching(std::forward<T>(awaitable))); }

        void run() { net_client_.run(); } ///< 阻塞当前线程执行 I/O，直到所有 I/O 处理完成，可从多个线程调用
    };
}
