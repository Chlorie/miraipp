#pragma once

#include <optional>

#include "../detail/json.h"

namespace mpp
{
    struct SessionConfig final
    {
        std::optional<int32_t> cache_size;
        std::optional<bool> enable_websocket;
        
        static SessionConfig from_json(detail::JsonElem json);
    };
}
