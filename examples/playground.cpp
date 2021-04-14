#include <mirai/mirai.h>

using namespace std::literals;
using namespace mpp::literals;
using unifex::task;

task<bool> test(const mpp::Event& ev)
{
    if (const auto* ptr = ev.get_if<mpp::FriendMessageEvent>();
        ptr->content() == "stop")
    {
        co_await ptr->quote_reply_async("ok");
        co_return false;
    }
    co_return true;
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
