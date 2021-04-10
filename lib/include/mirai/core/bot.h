#pragma once

#include <filesystem>
#include <span>
#include <clu/function_ref.h>
#include <clu/optional_ref.h>

#include "common.h"
#include "info_types.h"
#include "config_types.h"
#include "net_client.h"
#include "../message/segment_types_fwd.h"
#include "../event/event_types_fwd.h"
// #include "../pattern/pattern_matcher_queue.h"

namespace mpp
{
    namespace ex = unifex;
    class Message;
    class Event;

    MPP_SUPPRESS_EXPORT_WARNING
    /// Mirai++ 中功能的核心类型，代表 mirai-api-http 中的一个会话。此类不可复制或移动。
    class MPP_API Bot final
    {
    public:
        using Clock = std::chrono::steady_clock;

    private:
        net::Client net_client_;
        UserId bot_id_;
        std::string sess_key_;
        // PatternMatcherQueue pm_queue_;

        std::string check_auth_gen_body(std::string_view auth_key) const;
        Event parse_event(detail::JsonElem json);
        std::vector<Event> parse_events(detail::JsonElem json);

    public:
        /// \defgroup BotSpecial
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

        /// \defgroup BotProps
        /// \{
        UserId id() const noexcept { return bot_id_; } ///< 获取当前 bot 的 QQ 号
        bool authorized() const noexcept { return bot_id_.valid(); } ///< 当前 bot 是否已授权
        std::string_view session_key() const noexcept { return sess_key_; } ///< 获取当前已授权 bot 的会话密钥
        /// \}

        /// \defgroup BotGetVer
        /// \{
        std::string get_version();
        /**
         * \brief 获取 mirai-api-http 版本号
         * \return 包含 mirai-api-http 版本的字符串
         */
        ex::task<std::string> get_version_async();
        /// \}

        /// \defgroup BotSess
        /// \{
        void authorize(std::string_view auth_key, UserId id);
        /**
         * \brief 授权并校验一个 session
         * \param auth_key mirai-http-server 中指定的授权用 key
         * \param id 对应的已登录 bot 的 QQ 号
         */
        ex::task<void> authorize_async(std::string_view auth_key, UserId id);
        void release(); ///< 同步地释放当前会话，若当前对象析构时会话仍在开启状态则会调用此函数
        ex::task<void> release_async(); ///< 异步地释放当前会话
        /// \}

        /// \defgroup BotSendMsg
        /// \{
        MessageId send_message(UserId id, const Message& message, clu::optional_param<MessageId> quote = {});
        MessageId send_message(GroupId id, const Message& message, clu::optional_param<MessageId> quote = {});
        MessageId send_message(TempId id, const Message& message, clu::optional_param<MessageId> quote = {});
        ex::task<MessageId> send_message_async(UserId id, const Message& message, clu::optional_param<MessageId> quote = {});
        ex::task<MessageId> send_message_async(GroupId id, const Message& message, clu::optional_param<MessageId> quote = {});
        /**
         * \brief 向指定对象（好友，群，临时会话）发送消息
         * \param id 消息发送的对象
         * \param message 消息内容
         * \param quote （可选）要引用回复的消息 id
         * \return 已发送消息的 id，用于撤回和引用回复
         */
        ex::task<MessageId> send_message_async(TempId id, const Message& message, clu::optional_param<MessageId> quote = {});

        void recall(MessageId id);
        /**
         * \brief 撤回一条消息
         * \param id 要撤回的消息 id
         */
        ex::task<void> recall_async(MessageId id);

        std::vector<std::string> send_image_message(UserId id, std::span<const std::string> urls);
        std::vector<std::string> send_image_message(GroupId id, std::span<const std::string> urls);
        std::vector<std::string> send_image_message(TempId id, std::span<const std::string> urls);
        ex::task<std::vector<std::string>> send_image_message_async(UserId id, std::span<const std::string> urls);
        ex::task<std::vector<std::string>> send_image_message_async(GroupId id, std::span<const std::string> urls);
        /**
         * \brief 向指定对象（好友，群，临时会话）发送图片消息，除非需要获得图片的 ImageId 否则不建议使用此函数
         * \param id 消息发送的对象
         * \param urls 要发送的图片的 URL
         * \return 发送出去图片的 ImageId
         */
        ex::task<std::vector<std::string>> send_image_message_async(TempId id, std::span<const std::string> urls);

        Image upload_image(TargetType type, const std::filesystem::path& path);
        /**
         * \brief 上传图片以获得可用于发送的图片消息段
         * \param type 图片发送目标的类型（好友图片与群图片不通用）
         * \param path 图片文件的路径
         * \return 上传成功的图片消息段
         */
        ex::task<Image> upload_image_async(TargetType type, const std::filesystem::path& path);

        Voice upload_voice(TargetType type, const std::filesystem::path& path);
        /**
         * \brief 上传语音以获得可用于发送的语音消息段
         * \param type 语音发送目标的类型（好友语音与群语音不通用）
         * \param path 语音文件的路径
         * \return 上传成功的语音消息段
         */
        ex::task<Voice> upload_voice_async(TargetType type, const std::filesystem::path& path);
        /// \}

        /// \defgroup BotEvent
        /// \{
        std::vector<Event> pop_events(size_t count);
        std::vector<Event> pop_latest_events(size_t count);
        std::vector<Event> peek_events(size_t count);
        std::vector<Event> peek_latest_events(size_t count);
        ex::task<std::vector<Event>> pop_events_async(size_t count);
        ex::task<std::vector<Event>> pop_latest_events_async(size_t count);
        ex::task<std::vector<Event>> peek_events_async(size_t count);
        /**
         * \brief 接收 bot 收到的消息和事件
         * \details \rst
         * 使用 ``pop`` 时获取到的事件会从事件队列中删除，而 ``peek`` 不会。
         * ``latest_events`` 会从事件队列的队尾开始（获取最新的事件），无 ``latest`` 的版本获取事件队列中最早的事件。
         * \endrst
         * \param count 获取事件个数的上限
         * \return 获取到的事件
         */
        ex::task<std::vector<Event>> peek_latest_events_async(size_t count);

        ex::task<Event> retrieve_message_async(MessageId id);
        ex::task<size_t> count_message_async();
        /// \}

        ex::task<std::vector<Friend>> list_friends_async();
        ex::task<std::vector<Group>> list_groups_async();
        ex::task<std::vector<Member>> list_members_async(GroupId id);

        ex::task<void> mute_async(GroupId group, UserId user, std::chrono::seconds duration);
        ex::task<void> unmute_async(GroupId group, UserId user);
        ex::task<void> mute_all_async(GroupId group);
        ex::task<void> unmute_all_async(GroupId group);

        ex::task<void> kick_async(GroupId group, UserId user, std::string_view reason);
        ex::task<void> quit_async(GroupId group);

        ex::task<void> respond_async(
            const NewFriendRequestEvent& ev, NewFriendResponseType type, std::string_view reason);
        ex::task<void> respond_async(
            const MemberJoinRequestEvent& ev, MemberJoinResponseType type, std::string_view reason);
        ex::task<void> respond_async(
            const BotInvitedJoinGroupRequestEvent& ev, BotInvitedJoinGroupResponseType type, std::string_view reason);

        ex::task<GroupConfig> get_group_config_async(GroupId group);
        ex::task<void> config_group_async(GroupId group, const GroupConfig& config);

        ex::task<MemberInfo> get_member_info_async(GroupId group, UserId user);
        ex::task<void> set_member_info_async(GroupId group, UserId user, const MemberInfo& info);

        /// \defgroup BotMonitor
        /// \{
        /**
         * \brief 启动 Websocket 会话，监听所有种类的事件
         * \param callback 接收到消息时需要调用的函数
         * \remark \rst
         * 回调函数的函数原型须为 ``bool callback(const Event&)``，
         * 当某次调用回调函数返回 ``true`` 时关闭 Websocket 会话。
         * \endrst
         */
        void monitor_events(clu::function_ref<bool(const Event&)> callback);

        /**
         * \brief 异步地启动 Websocket 会话，监听所有种类的事件
         * \param callback 接收到消息时需要调用的函数
         * \remark \rst
         * 回调函数的函数原型须为 ``ex::task<bool> callback(const Event&)``，
         * 当某次调用回调函数返回 ``true`` 时，在所有正在进行的回调函数全部执行完成后将会关闭 Websocket 会话。
         * \endrst
         */
        ex::task<void> monitor_events_async(clu::function_ref<ex::task<bool>(const Event&)> callback);
        /// \}

        SessionConfig get_config();
        ex::task<SessionConfig> get_config_async();

        void config(SessionConfig config);
        ex::task<void> config_async(SessionConfig config);

        // Pattern matcher queue needs a major revamp

        // template <ConcreteEvent E, PatternFor<E>... Ps>
        // ex::task<E> match_async(Ps&&... patterns)
        // {
        //     co_return *co_await pm_queue_.enqueue_async<E>(std::forward<Ps>(patterns)...);
        // }
        // 
        // template <ConcreteEvent E, PatternFor<E>... Ps>
        // ex::task<std::optional<E>> match_async(const Clock::time_point deadline, Ps&&... patterns)
        // {
        //     if (const auto res = co_await clu::race(
        //             pm_queue_.enqueue_async<E>(std::forward<Ps>(patterns)...),
        //             wait_async(deadline));
        //         res.index() == 0)
        //         co_return *clu::get<0>(res);
        //     co_return std::nullopt;
        // }
        // 
        // template <ConcreteEvent E, PatternFor<E>... Ps>
        // ex::task<std::optional<E>> match_async(const Clock::duration timeout, Ps&&... patterns)
        // {
        //     return match_async<E>(Clock::now() + timeout, std::forward<Ps>(patterns)...);
        // }

        /// \defgroup BotAsyncWait
        /// \{
        /**
         * \brief 异步等待直到某时间点
         * \param tp 等待的到期时间
         */
        ex::task<void> wait_async(const Clock::time_point tp) { return net_client_.wait_async(tp); }

        /**
         * \brief 异步等待给定时长
         * \param dur 等待的时长
         */
        ex::task<void> wait_async(const Clock::duration dur) { return net_client_.wait_async(Clock::now() + dur); }
        /// \}

        void run() { net_client_.run(); } ///< 阻塞当前线程执行 I/O，直到所有 I/O 处理完成，可从多个线程调用

        boost::asio::io_context& io_context() { return net_client_.io_context(); } ///< 返回 bot 内使用的 asio::io_context
    };
    MPP_RESTORE_EXPORT_WARNING

    MPP_API void launch_async_bot(
        clu::function_ref<ex::task<void>(Bot&)> task, size_t thread_count = 1,
        std::string_view host = "127.0.0.1", std::string_view port = "8080");
}
