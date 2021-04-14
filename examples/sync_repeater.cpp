#include <mirai/mirai.h>

int main()
{
    mpp::Bot bot;
    bot.authorize("authkey", mpp::UserId(123456789));
    bot.config({ .enable_websocket = true });
    bot.monitor_events([](const mpp::Event& ev)
    {
        if (const auto* ptr = ev.get_if<mpp::GroupMessageEvent>())
        {
            if (ptr->content() == "stop") return false;
            ptr->send_message(ptr->content());
        }
        return true;
    });
}
