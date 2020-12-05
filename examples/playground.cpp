#include <clu/coroutine/when_all.h>
#include <mirai/core/bot.h>
#include <mirai/event/event.h>
#include <mirai/pattern/pattern_types.h>

using namespace std::literals;
using namespace mpp::literals;

clu::task<> test_match(mpp::Bot& bot)
{
    const auto id = co_await bot.async_send_message(1357008522_uid, "等你个回复");
    auto&& ev = co_await bot.async_match<mpp::FriendMessageEvent>(
        from(1357008522_uid), replying(id));
    co_await ev.async_quote_reply("感谢回复！");
}

clu::task<bool> wait_stop(const mpp::Event& ev)
{
    if (const auto* ptr = ev.get_if<mpp::FriendMessageEvent>();
        ptr->msg.content == "stop")
        co_return true;
    co_return false;
}

clu::task<> test(mpp::Bot& bot)
{
    co_await bot.async_authorize("somerandomauthkeyformybot", 3378448768_uid);
    co_await bot.async_config({ .enable_websocket = true });
    co_await when_all(bot.async_monitor_events(wait_stop), test_match(bot));
}

int main() // NOLINT
{
    std::system("chcp 65001 > nul");
    mpp::Bot bot;
    clu::coroutine_scope scope;
    scope.spawn(test(bot));
    bot.run();
}
