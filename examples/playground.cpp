#include <mirai/core/bot.h>
#include <mirai/event/event.h>

using namespace std::literals;
using namespace mpp::literals;

clu::task<bool> repeat(const mpp::Event& ev)
{
    if (const auto* ptr = ev.get_if<mpp::FriendMessageEvent>())
    {
        const auto& msg = ptr->msg.content;
        if (msg == "停停！") co_return true;
        co_await ptr->async_send_message(msg);
    }
    co_return false;
}

clu::task<> test(mpp::Bot& bot)
{
    co_await bot.async_authorize("somerandomauthkeyformybot", 3378448768_uid);
    co_await bot.async_config({ .enable_websocket = true });
    co_await bot.async_monitor_events(repeat);
}

int main() // NOLINT
{
    mpp::Bot bot;
    clu::coroutine_scope scope;
    scope.spawn(test(bot));
    bot.run();
}
