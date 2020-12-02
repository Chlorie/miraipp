#include "mirai/core/config_types.h"

namespace mpp
{
    SessionConfig SessionConfig::from_json(const detail::JsonElem json)
    {
        return SessionConfig
        {
            .cache_size = detail::from_json<std::optional<int32_t>>(json["cacheSize"]),
            .enable_websocket = detail::from_json<std::optional<bool>>(json["enableWebsocket"])
        };
    }
}
