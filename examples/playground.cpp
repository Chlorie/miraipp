#include <mirai/mirai.h>

using namespace std::literals;
using namespace mpp::literals;
using unifex::task;
using unifex::stop;

task<void> test(const mpp::Event& ev)
{
    if (const auto* ptr = ev.get_if<mpp::FriendMessageEvent>();
        ptr && ptr->content() == "start")
    {
        co_await ptr->quote_reply_async("ok, please send stop");
        const auto res = co_await ev.bot().next_event_async<mpp::FriendMessageEvent>(5s,
            [&](const mpp::FriendMessageEvent& e)
            {
                return e.sender == ptr->sender && e.content() == "stop";
            });
        if (res) co_await res->quote_reply_async("ok");
        else co_await ptr->send_message_async("y u so slow?");
        co_await stop();
    }
}

int main() // NOLINT
{
    std::system("chcp 65001 > nul");
    mpp::launch_async_bot([](mpp::Bot& bot) -> task<void>
    {
        try
        {
            co_await bot.authorize_async("somerandomauthkeyformybot", 3378448768_uid);
            co_await bot.config_async({ .enable_websocket = true });
            co_await bot.monitor_events_async(test);
        }
        catch (...) { mpp::log_exception(); }
    });
}

// task<void> target(mpp::Bot& bot, const mpp::GroupId subject)
// {
//     co_await bot.send_message_async(subject, "开启复读模式");
//     co_await bot.while_select_messages(
//         content_is("stop") | [&]() -> task<bool>
//         {
//             co_await bot.send_message_async(subject, "已关闭复读");
//             co_return false;
//         },
//         default_message | [&](const mpp::Message& msg) -> task<bool>
//         {
//             co_await bot.send_message_async(subject, msg);
//             co_return true;
//         },
//         timeout(3s) | ex::just(true)
//     );
//     co_await bot.send_message_async(subject, "复读模式结束");
// }
