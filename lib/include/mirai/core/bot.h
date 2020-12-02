#pragma once

#include <filesystem>
#include <span>
#include <clu/function_ref.h>
#include <clu/optional_ref.h>
#include <clu/coroutine/task.h>
#include <clu/coroutine/coroutine_scope.h>

#include "common.h"
#include "config_types.h"
#include "../detail/net_client.h"
#include "../detail/json.h"

namespace mpp
{
    class Message;
    class Event;
    struct Image;
    struct Voice;
    struct Friend;
    struct Group;
    struct Member;

    /// Mirai++ 中功能的核心类型，代表 mirai-api-http 中的一个会话。此类不可复制或移动。
    class Bot final
    {
    public:
        using Clock = std::chrono::steady_clock;

    private:
        detail::NetClient net_client_;
        UserId bot_id_;
        std::string sess_key_;

        std::string release_body() const;
        std::string send_message_body(int64_t id, const Message& message, clu::optional_param<MessageId> quote) const;
        std::string group_target_body(GroupId group) const;
        Event parse_event(detail::JsonElem json);
        std::vector<Event> parse_events(detail::JsonElem json);

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

        UserId id() const noexcept { return bot_id_; } ///< 获取当前 bot 的 QQ 号
        bool authorized() const noexcept { return bot_id_.valid(); } ///< 当前 bot 是否已授权

        /**
         * \brief 异步地获取 mirai-api-http 版本号
         * \return （异步）包含 mirai-api-http 版本的字符串
         */
        clu::task<std::string> async_get_version();

        /**
         * \brief 异步地授权并校验一个 session
         * \param auth_key mirai-http-server 中指定的授权用 key
         * \param id 对应的已登录 bot 的 QQ 号
         */
        clu::task<> async_authorize(std::string_view auth_key, UserId id);

        void release(); ///< 同步地释放当前会话，若当前对象析构时会话仍在开启状态则会调用此函数

        clu::task<> async_release(); ///< 异步地释放当前会话

        /**
         * \brief 异步地向给定好友发送消息
         * \param id 好友的 QQ 号
         * \param message 消息内容
         * \param quote （可选）要引用回复的消息 id
         * \return （异步）已发送消息的 id，用于撤回和引用回复
         */
        clu::task<MessageId> async_send_message(UserId id, const Message& message, clu::optional_param<MessageId> quote = {});

        /**
         * \brief 异步地向给定群发送消息
         * \param id 群号
         * \param message 消息内容
         * \param quote （可选）要引用回复的消息 id
         * \return （异步）已发送消息的 id，用于撤回和引用回复
         */
        clu::task<MessageId> async_send_message(GroupId id, const Message& message, clu::optional_param<MessageId> quote = {});

        /**
         * \brief 异步地发送临时会话消息
         * \param id 临时会话对象
         * \param message 消息内容
         * \param quote （可选）要引用回复的消息 id
         * \return （异步）已发送消息的 id，用于撤回和引用回复
         */
        clu::task<MessageId> async_send_message(TempId id, const Message& message, clu::optional_param<MessageId> quote = {});

        /**
         * \brief 异步地撤回一条消息
         * \param id 要撤回的消息 id
         */
        clu::task<> async_recall(MessageId id);

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

        clu::task<> async_mute(GroupId group, UserId user, std::chrono::seconds duration);
        clu::task<> async_unmute(GroupId group, UserId user);
        clu::task<> async_mute_all(GroupId group);
        clu::task<> async_unmute_all(GroupId group);

        clu::task<> async_kick(GroupId group, UserId user, std::string_view reason);
        clu::task<> async_quit(GroupId group);

        // clu::task<> async_respond(const NewFriendRequestEvent& event, NewFriendResponseType type, std::string_view reason);
        // clu::task<> async_respond(const MemberJoinRequestEvent& event, MemberJoinResponseType type, std::string_view reason);

        // clu::task<GroupConfig> async_get_group_config(GroupId group);
        // clu::task<> async_config_group(GroupId group, const GroupConfig& config);

        // clu::task<MemberInfo> async_get_member_info(GroupId group, UserId user);
        // clu::task<> async_set_member_info(GroupId group, UserId user, const MemberInfo& info);

        /**
         * \brief 异步地启动 Websocket 会话，监听所有种类的事件
         * \param callback 接收到消息时需要调用的函数
         * \return （异步）无返回值
         * \remark 回调函数的函数原型须为 clu::task<bool> callback(const Event&)，
         * 当某次调用回调函数返回 true 时，在所有正在进行的回调函数全部执行完成后将会关闭 Websocket 会话。
         */
        clu::task<> async_monitor_events(clu::function_ref<clu::task<bool>(const Event&)> callback);

        clu::task<> async_config(clu::optional_param<int32_t> cache_size = {}, clu::optional_param<bool> enable_websocket = {});

        clu::task<> async_config(SessionConfig config);

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

        void run() { net_client_.run(); } ///< 阻塞当前线程执行 I/O，直到所有 I/O 处理完成，可从多个线程调用
    };
}
