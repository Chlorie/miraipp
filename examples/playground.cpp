#include <fmt/core.h>
#include <mirai/core/bot.h>
#include <mirai/message/message.h>

using namespace mpp::literals;
using namespace std::literals;

clu::task<> test(mpp::Bot& bot)
{
    const auto ver = co_await bot.async_get_version();
    fmt::print("mirai-api-http version: {}\n", ver);
    co_await bot.async_auth("somerandomauthkeyformybot", 3378448768_uid);
    fmt::print("会话认证上了.jpg\n");
    const auto start_time = std::chrono::steady_clock::now();
    const auto msgid = co_await bot.async_send_message(1357008522_uid, "引用这一条");
    co_await bot.async_send_message(1357008522_uid, "引用上", msgid);
    const auto end_time = std::chrono::steady_clock::now();
    fmt::print("发送成功，用了 {}ms\n", (end_time - start_time) / 1.0ms);
}

int main()
{
    mpp::Bot bot;
    bot.spawn_catching(test(bot));
    bot.run();
}
