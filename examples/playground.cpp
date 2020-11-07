#include <iostream>
#include <mirai/core/bot.h>

using namespace std::literals;

clu::task<void> log_on_error(clu::task<void> task)
{
    try { co_await task; }
    catch (const std::exception& exc) { std::cerr << exc.what() << '\n'; }
}

clu::task<void> get_test(mpp::detail::NetClient& client)
{
    const auto response = co_await client.async_get("/about");
    std::cout << response;
    for (size_t i = 0; i < 5; i++)
    {
        co_await client.async_wait(std::chrono::steady_clock::now() + 500ms);
        std::cout << "\n500ms later\n";
    }
}

int main()
{
    clu::coroutine_scope scope;
    mpp::detail::NetClient client("127.0.0.1", "8080");
    scope.spawn(log_on_error(get_test(client)));
    client.run();
    sync_wait(scope.join());
}
