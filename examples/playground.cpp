#include <mirai/mirai.h>
#include <clu/vector_utils.h>

using namespace std::literals;
using namespace mpp::literals;
using unifex::task;

task<bool> test(const mpp::Event& ev)
{
    if (const auto* gme = ev.get_if<mpp::GroupMessageEvent>())
    {
        const auto& msg = gme->content();
        if (msg.size() == 1 && msg[0].type() == mpp::SegmentType::forward)
        {
            const auto& [title, brief, source, summary, messages] = msg[0].get<mpp::Forward>();
            std::string sent;
            for (const auto& [sender, time, sender_name, content] : messages)
                sent += fmt::format("time: {}, sender: {} ({}), content: {}\n", time, sender_name, sender.id, content);
            co_await gme->quote_reply_async(std::move(sent));
            co_return true;
        }
    }
    co_return false;
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
