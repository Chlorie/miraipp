#pragma once

#include "../event/event.h"

namespace mpp
{
    // @formatter:off
    /// 适用于某事件类型的模式概念
    template <typename P, typename E>
    concept PatternFor = ConcreteEvent<E> &&
        requires(const P& pattern, const E& ev) { { pattern.match(ev) } -> std::convertible_to<bool>; };
    // @formatter:on

    template <ConcreteEvent E, PatternFor<E>... Ps>
    bool match_with(const E& ev, const Ps&... patterns) { return (patterns.match(ev) && ...); }
}
