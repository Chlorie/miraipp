#pragma once

#include "../core/format.h"

namespace simdjson
{
    template <typename T> struct simdjson_result;
    namespace dom
    {
        class element;
    }
}

namespace mpp::detail
{
    using JsonElem = simdjson::dom::element;
    using JsonRes = simdjson::simdjson_result<JsonElem>;
}
