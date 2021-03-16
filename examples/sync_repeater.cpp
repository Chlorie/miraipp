#include <mirai/core/bot.h>
#include <mirai/core/exceptions.h>
#include <mirai/event/event_types.h>

int main() try
{
    mpp::Bot bot;
    bot.authorize("authkey", mpp::UserId(123456789));
    bot.config({ .enable_websocket = true });
    bot.monitor_events([](const mpp::Event& ev)
    {
        if (const auto* ptr = ev.get_if<mpp::FriendMessageEvent>())
            ptr->bot().send_message(ptr->sender.id, ptr->content());
        return false;
    });
}
catch (...) { mpp::log_exception(); }
