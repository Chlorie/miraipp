#include <set>
#include <random>
#include <clu/coroutine/coroutine_scope.h>
#include <clu/coroutine/when_all.h>
#include <clu/coroutine/generator.h>
#include <mirai/core/bot.h>
#include <mirai/pattern/pattern_types.h>

using namespace std::literals;
using namespace mpp::literals;

std::mt19937 rng{ std::random_device{}() };

std::set<int64_t> playing_set;

clu::generator<std::pair<std::string, std::string>> generate_qa()
{
    const auto randint = [&](const int min, const int max)
    {
        std::uniform_int_distribution dist(min, max);
        return dist(rng);
    };

    for (size_t i = 1; i <= 5; i++)
    {
        const int a = randint(1, 9), b = randint(1, 9);
        if (randint(0, 1) == 0)
            co_yield { fmt::format("Q{}: {} + {} = ?", i, a, b), fmt::to_string(a + b) };
        else
            co_yield { fmt::format("Q{}: {} - {} = ?", i, a + b, a), fmt::to_string(b) };
    }
    for (size_t i = 6; i <= 10; i++)
    {
        if (randint(0, 1) == 0)
        {
            const int a = randint(1, 99), b = randint(1, 9);
            if (randint(0, 1) == 0)
                co_yield { fmt::format("Q{}: {} + {} = ?", i, a, b), fmt::to_string(a + b) };
            else
                co_yield { fmt::format("Q{}: {} - {} = ?", i, a + b, b), fmt::to_string(a) };
        }
        else
        {
            const int a = randint(1, 9), b = randint(1, 9);
            if (randint(0, 1) == 0)
                co_yield { fmt::format("Q{}: {} × {} = ?", i, a, b), fmt::to_string(a * b) };
            else
                co_yield { fmt::format("Q{}: {} / {} = ?", i, a * b, a), fmt::to_string(b) };
        }
    }
    for (size_t i = 11; i <= 20; i++)
    {
        if (randint(0, 1) == 0)
        {
            const int a = randint(100, 999), b = randint(100, 999);
            if (randint(0, 1) == 0)
                co_yield { fmt::format("Q{}: {} + {} = ?", i, a, b), fmt::to_string(a + b) };
            else
                co_yield { fmt::format("Q{}: {} - {} = ?", i, a + b, b), fmt::to_string(a) };
        }
        else
        {
            const int a = randint(10, 99), b = randint(1, 9);
            if (randint(0, 1) == 0)
                co_yield { fmt::format("Q{}: {} × {} = ?", i, a, b), fmt::to_string(a * b) };
            else
                co_yield { fmt::format("Q{}: {} / {} = ?", i, a * b, b), fmt::to_string(a) };
        }
    }
    for (size_t i = 21; i <= 25; i++)
    {
        switch (randint(0, 2))
        {
            case 0:
            {
                const int a = randint(100, 999), b = randint(100, 999), c = randint(100, 999);
                co_yield { fmt::format("Q{}: {} + {} + {} = ?", i, a, b, c), fmt::to_string(a + b + c) };
                break;
            }
            case 1:
            {
                const int a = randint(10000, 99999), b = randint(10000, 99999);
                if (randint(0, 1) == 0)
                    co_yield { fmt::format("Q{}: {} + {} = ?", i, a, b), fmt::to_string(a + b) };
                else
                    co_yield { fmt::format("Q{}: {} - {} = ?", i, a + b, b), fmt::to_string(a) };
                break;
            }
            default:
            {
                const int a = randint(10, 99), b = randint(10, 99);
                if (randint(0, 1) == 0)
                    co_yield { fmt::format("Q{}: {} × {} = ?", i, a, b), fmt::to_string(a * b) };
                else
                    co_yield { fmt::format("Q{}: {} / {} = ?", i, a * b, b), fmt::to_string(a) };
                break;
            }
        }
    }
}

clu::task<> mafs_test(mpp::Bot& bot, const mpp::GroupId gid, const mpp::UserId uid)
{
    clu::scope_exit _([&] { playing_set.erase(gid.id); });

    co_await bot.async_send_message(gid, "qUIcK mAFs");
    co_await bot.async_wait(3s);

    for (const auto& [question, answer] : generate_qa())
    {
        co_await bot.async_send_message(gid, question);
        const auto ev = co_await bot.async_match<mpp::GroupMessageEvent>(5s,
            from(gid), from(uid), mpp::with_content(answer));
        if (!ev)
        {
            co_await bot.async_send_message(gid, "不太行啊~");
            co_return;
        }
    }
    co_await bot.async_send_message(gid, "这是好的");
}

std::pair<std::string, std::string> random_starting_hand()
{
    static std::array<size_t, 136> arr = []()
    {
        std::array<size_t, 136> arr{};
        for (size_t i = 0; i < 36; i++) arr[i] = i / 4 + 1;
        for (size_t i = 36; i < 72; i++) arr[i] = i / 4 + 2;
        for (size_t i = 72; i < 108; i++) arr[i] = i / 4 + 3;
        for (size_t i = 108; i < 136; i++) arr[i] = i / 4 + 4;
        arr[0] = 0;
        arr[36] = 10;
        arr[72] = 20;
        return arr;
    }();
    static const auto comp = [](const size_t lhs, const size_t rhs)
    {
        if (lhs % 10 == 0 && rhs > lhs && rhs <= lhs + 5) return false;
        if (rhs % 10 == 0 && lhs > rhs && lhs <= rhs + 5) return true;
        return lhs < rhs;
    };
    static const auto to_str = [](const size_t i) -> std::string_view
    {
        thread_local std::string result(2, '\0');
        static const char* letters = "mpsz";
        result[0] = static_cast<char>(i % 10 + '0');
        result[1] = letters[i / 10];
        return result;
    };

    std::ranges::shuffle(arr, rng);
    std::sort(arr.begin(), arr.begin() + 13, comp);

    std::string hand;
    hand.reserve(32);
    for (size_t i = 0; i < 13; i++) hand += to_str(arr[i]);
    (hand += "2%7C") += to_str(arr[13]);

    return { hand, fmt::format("++{}++++", to_str(arr[14])) };
}

clu::task<bool> wait_stop(const mpp::Event& ev)
{
    if (const auto* ptr = ev.get_if<mpp::FriendMessageEvent>())
    {
        if (ptr->sender.id == 1357008522_uid && ptr->content() == "stop")
            co_return true;
        else if (ptr->content() == "随机起手")
        {
            const auto& [hand, dora] = random_starting_hand();
            co_await ptr->async_quote_reply({
                mpp::Image{ .url = "https://mj.black-desk.cn/" + hand },
                mpp::Image{ .url = "https://mj.black-desk.cn/" + dora }
            });
        }
    }

    if (const auto* ptr = ev.get_if<mpp::GroupMessageEvent>())
    {
        if (ptr->content() == "mafs")
        {
            const int64_t id = ptr->sender.group.id.id;
            if (!playing_set.contains(id))
            {
                playing_set.insert(id);
                co_await mafs_test(ptr->bot(), ptr->sender.group.id, ptr->sender.id);
            }
        }
    }

    co_return false;
}

clu::task<> test(mpp::Bot& bot)
{
    co_await bot.async_authorize("somerandomauthkeyformybot", 3378448768_uid);
    co_await bot.async_config({ .enable_websocket = true });
    co_await bot.async_monitor_events(wait_stop);
}

int main() // NOLINT
{
    std::system("chcp 65001 > nul");
    mpp::Bot bot;
    clu::coroutine_scope scope;
    scope.spawn(test(bot));
    bot.run();
}
