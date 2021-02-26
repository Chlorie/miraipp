#pragma once

#include <filesystem>
#include <span>
#include <clu/function_ref.h>
#include <clu/optional_ref.h>
#include <clu/coroutine/task.h>
#include <clu/coroutine/race.h>

#include "common.h"
#include "info_types.h"
#include "config_types.h"
#include "net_client.h"
#include "../detail/json.h"
#include "../message/segment_types_fwd.h"
#include "../event/event_types_fwd.h"
#include "../pattern/pattern_matcher_queue.h"

namespace mpp
{
    class Message;

    /// Mirai++ 中功能的核心类型，代表 mirai-api-http 中的一个会话。此类不可复制或移动。
    class Bot final
    {
    public:
        using Clock = std::chrono::steady_clock;

    private:
        net::Client net_client_;
        UserId bot_id_;
        std::string sess_key_;
        PatternMatcherQueue pm_queue_;

        std::string check_auth_gen_body(std::string_view auth_key) const;
        std::string release_body() const;
        std::string send_message_body(int64_t id, const Message& message, clu::optional_param<MessageId> quote) const;
        std::string send_message_body(TempId id, const Message& message, clu::optional_param<MessageId> quote) const;
        std::string group_target_body(GroupId group) const;
        Event parse_event(detail::JsonElem json);
        std::vector<Event> parse_events(detail::JsonElem json);

    public:
        /// \defgroup bot_specials
        /// \{
        /**
         * \brief 创建新的 bot 对象并同步地连接到指定端点
         * \param host 要连接到端点的主机，默认为 127.0.0.1
         * \param port 要连接到端点的端口，默认为 8080
         */
        explicit Bot(const std::string_view host = "127.0.0.1", const std::string_view port = "8080"):
            net_client_(host, port) {}
        ~Bot() noexcept; ///< 销毁当前 bot 对象，释放未结束的会话并关闭所有连接
        /// \}

        Bot(const Bot&) = delete;
        Bot(Bot&&) = delete;
        Bot& operator=(const Bot&) = delete;
        Bot& operator=(Bot&&) = delete;

        /// \defgroup bot_properties
        /// \{
        UserId id() const noexcept { return bot_id_; } ///< 获取当前 bot 的 QQ 号
        bool authorized() const noexcept { return bot_id_.valid(); } ///< 当前 bot 是否已授权
        std::string_view session_key() const noexcept { return sess_key_; } ///< 获取当前已授权 bot 的会话密钥
        /// \}

        /// \defgroup bot_get_version
        /// \{
        std::string get_version();
        /**
         * \brief 获取 mirai-api-http 版本号
         * \return 包含 mirai-api-http 版本的字符串
         */
        clu::task<std::string> async_get_version();
        /// \}

        /// \defgroup bot_session
        /// \{
        void authorize(std::string_view auth_key, UserId id);
        /**
         * \brief 授权并校验一个 session
         * \param auth_key mirai-http-server 中指定的授权用 key
         * \param id 对应的已登录 bot 的 QQ 号
         */
        clu::task<void> async_authorize(std::string_view auth_key, UserId id);
        void release(); ///< 同步地释放当前会话，若当前对象析构时会话仍在开启状态则会调用此函数
        clu::task<void> async_release(); ///< 异步地释放当前会话
        /// \}

        /// \defgroup bot_send_message
        /// \{
        MessageId send_message(UserId id, const Message& message, clu::optional_param<MessageId> quote = {});
        MessageId send_message(GroupId id, const Message& message, clu::optional_param<MessageId> quote = {});
        MessageId send_message(TempId id, const Message& message, clu::optional_param<MessageId> quote = {});
        clu::task<MessageId> async_send_message(UserId id, const Message& message, clu::optional_param<MessageId> quote = {});
        clu::task<MessageId> async_send_message(GroupId id, const Message& message, clu::optional_param<MessageId> quote = {});
        /**
         * \brief 向指定对象（好友，群，临时会话）发送消息
         * \param id 消息发送的对象
         * \param message 消息内容
         * \param quote （可选）要引用回复的消息 id
         * \return 已发送消息的 id，用于撤回和引用回复
         */
        clu::task<MessageId> async_send_message(TempId id, const Message& message, clu::optional_param<MessageId> quote = {});
        /// \}

        /**
         * \brief 异步地撤回一条消息
         * \param id 要撤回的消息 id
         */
        clu::task<void> async_recall(MessageId id);

        /**
         * \brief 异步地向给定好友发送图片消息，除非需要获得图片的 ImageId 否则不建议使用此函数
         * \param id 好友的 QQ 号
         * \param urls 要发送的图片的 URL
         * \return （异步）发送出去图片的 ImageId
         */
        clu::task<std::vector<std::string>> async_send_image_message(UserId id, std::span<const std::string> urls);

        /**
         * \brief 异步地向给定群发送图片消息，除非需要获得图片的 ImageId 否则不建议使用此函数
         * \param id 群号
         * \param urls 要发送的图片的 URL
         * \return （异步）发送出去图片的 ImageId
         */
        clu::task<std::vector<std::string>> async_send_image_message(GroupId id, std::span<const std::string> urls);

        /**
         * \brief 异步地发送图片临时消息，除非需要获得图片的 ImageId 否则不建议使用此函数
         * \param id 临时会话对象
         * \param urls 要发送的图片的 URL
         * \return （异步）发送出去图片的 ImageId
         */
        clu::task<std::vector<std::string>> async_send_image_message(TempId id, std::span<const std::string> urls);

        /**
         * \brief 上传图片以获得可用于发送的图片消息段
         * \param type 图片发送目标的类型（好友图片与群图片不通用）
         * \param path 图片文件的路径
         * \return （异步）上传成功的图片消息段
         */
        clu::task<Image> async_upload_image(TargetType type, const std::filesystem::path& path);

        /**
         * \brief 上传语音以获得可用于发送的语音消息段
         * \param type 语音发送目标的类型（好友语音与群语音不通用）
         * \param path 语音文件的路径
         * \return （异步）上传成功的语音消息段
         */
        clu::task<Voice> async_upload_voice(TargetType type, const std::filesystem::path& path);

        /**
         * \brief 接收 bot 收到的最老的消息和事件，并将这些事件从事件队列中删除
         * \param count 获取事件个数的上限
         * \return （异步）获取到的事件
         */
        clu::task<std::vector<Event>> async_pop_events(size_t count);

        /**
         * \brief 接收 bot 收到的最新的消息和事件，并将这些事件从事件队列中删除
         * \param count 获取事件个数的上限
         * \return （异步）获取到的事件
         */
        clu::task<std::vector<Event>> async_pop_latest_events(size_t count);

        /**
         * \brief 接收 bot 收到的最老的消息和事件
         * \param count 获取事件个数的上限
         * \return （异步）获取到的事件
         */
        clu::task<std::vector<Event>> async_peek_events(size_t count);

        /**
         * \brief 接收 bot 收到的最新的消息和事件
         * \param count 获取事件个数的上限
         * \return （异步）获取到的事件
         */
        clu::task<std::vector<Event>> async_peek_latest_events(size_t count);

        clu::task<Event> async_retrieve_message(MessageId id);
        clu::task<size_t> async_count_message();

        clu::task<std::vector<Friend>> async_list_friends();
        clu::task<std::vector<Group>> async_list_groups();
        clu::task<std::vector<Member>> async_list_members(GroupId id);

        clu::task<void> async_mute(GroupId group, UserId user, std::chrono::seconds duration);
        clu::task<void> async_unmute(GroupId group, UserId user);
        clu::task<void> async_mute_all(GroupId group);
        clu::task<void> async_unmute_all(GroupId group);

        clu::task<void> async_kick(GroupId group, UserId user, std::string_view reason);
        clu::task<void> async_quit(GroupId group);

        clu::task<void> async_respond(
            const NewFriendRequestEvent& ev, NewFriendResponseType type, std::string_view reason);
        clu::task<void> async_respond(
            const MemberJoinRequestEvent& ev, MemberJoinResponseType type, std::string_view reason);
        clu::task<void> async_respond(
            const BotInvitedJoinGroupRequestEvent& ev, BotInvitedJoinGroupResponseType type, std::string_view reason);

        clu::task<GroupConfig> async_get_group_config(GroupId group);
        clu::task<void> async_config_group(GroupId group, const GroupConfig& config);

        clu::task<MemberInfo> async_get_member_info(GroupId group, UserId user);
        clu::task<void> async_set_member_info(GroupId group, UserId user, const MemberInfo& info);

        /**
         * \brief 异步地启动 Websocket 会话，监听所有种类的事件
         * \param callback 接收到消息时需要调用的函数
         * \return （异步）无返回值
         * \remark \rst
         * 回调函数的函数原型须为 ``clu::task<bool> callback(const Event&)``，
         * 当某次调用回调函数返回 ``true`` 时，在所有正在进行的回调函数全部执行完成后将会关闭 Websocket 会话。
         * \endrst
         */
        clu::task<void> async_monitor_events(clu::function_ref<clu::task<bool>(const Event&)> callback);

        clu::task<SessionConfig> async_get_config();

        clu::task<void> async_config(clu::optional_param<int32_t> cache_size = {}, clu::optional_param<bool> enable_websocket = {});

        clu::task<void> async_config(SessionConfig config);

        template <ConcreteEvent E, PatternFor<E>... Ps>
        clu::task<E> async_match(Ps&&... patterns)
        {
            co_return *co_await pm_queue_.async_enqueue<E>(std::forward<Ps>(patterns)...);
        }

        template <ConcreteEvent E, PatternFor<E>... Ps>
        clu::task<std::optional<E>> async_match(const Clock::time_point deadline, Ps&&... patterns)
        {
            const auto res = co_await clu::race(
                pm_queue_.async_enqueue<E>(std::forward<Ps>(patterns)...),
                async_wait(deadline));
            if (res.index() == 0) co_return *clu::get<0>(res);
            co_return std::nullopt;
        }

        template <ConcreteEvent E, PatternFor<E>... Ps>
        clu::task<std::optional<E>> async_match(const Clock::duration timeout, Ps&&... patterns)
        {
            return async_match<E>(Clock::now() + timeout, std::forward<Ps>(patterns)...);
        }

        /**
         * \brief 异步等待直到某时间点
         * \param tp 等待的到期时间
         * \return （异步）无返回值
         */
        clu::cancellable_task<bool> async_wait(const Clock::time_point tp) { return net_client_.async_wait(tp); }

        /**
         * \brief 异步等待给定时长
         * \param dur 等待的时长
         * \return （异步）无返回值
         */
        clu::cancellable_task<bool> async_wait(const Clock::duration dur) { return net_client_.async_wait(Clock::now() + dur); }

        void run() { net_client_.run(); } ///< 阻塞当前线程执行 I/O，直到所有 I/O 处理完成，可从多个线程调用

        boost::asio::io_context& io_context() { return net_client_.io_context(); } ///< 返回 bot 内使用的 asio::io_context
    };
}
