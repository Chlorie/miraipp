#include <mirai/core/bot.h>
#include <mirai/event/event.h>

using namespace std::literals;
using namespace mpp::literals;

clu::task<> test(mpp::Bot& bot)
{
    co_await bot.async_auth("somerandomauthkeyformybot", 3378448768_uid);
    while (true)
    {
        const auto events = co_await bot.async_pop_events(20);
        if (events.empty())
        {
            co_await bot.async_wait(1s);
            continue;
        }
        for (auto&& ev : events)
            if (const auto* ptr = ev.get_if<mpp::FriendMessageEvent>())
            {
                co_await ptr->async_send_message(ptr->msg.content);
                co_return;
            }
    }
}

int main() // NOLINT
{
    mpp::Bot bot;
    bot.spawn_catching(test(bot));
    bot.run();
}
