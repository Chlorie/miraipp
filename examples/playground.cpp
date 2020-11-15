#include <mirai/core/bot.h>

using namespace std::literals;
using namespace mpp::literals;

clu::task<> test(mpp::Bot& bot)
{
    co_await bot.async_auth("somerandomauthkeyformybot", 3378448768_uid);
    
}

int main() // NOLINT
{
    mpp::Bot bot;
    bot.spawn_catching(test(bot));
    bot.run();
}
