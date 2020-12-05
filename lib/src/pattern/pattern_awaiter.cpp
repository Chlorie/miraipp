#include "mirai/pattern/pattern_awaiter.h"

namespace mpp
{
    clu::task<bool> PatternMatcherQueue::async_match_event(const Event& ev)
    {
        auto lock = co_await mutex_.async_lock_scoped();
        bool matched = false;
        std::erase_if(queue_, [&](const std::unique_ptr<ItemModel>& ptr)
        {
            if (ptr->done()) return true;
            if (ptr->match(ev))
            {
                matched = true;
                return true;
            }
            return false;
        });
        co_return matched;
    }
}
