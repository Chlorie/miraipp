#include <mirai/core/bot.h>
#include <mirai/message/message.h>

using namespace std::literals;
using namespace mpp::literals;
using unifex::task;

int main() // NOLINT
{
    std::system("chcp 65001 > nul");
    mpp::launch_async_bot([](mpp::Bot& bot) -> task<void>
    {
        co_await bot.authorize_async("somerandomauthkeyformybot", 3378448768_uid);
        co_await bot.config_async({ .enable_websocket = true });
        co_await bot.send_message_async(1357008522_uid, "online");
    });
}
