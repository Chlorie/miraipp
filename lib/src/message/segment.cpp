#include "mirai/message/segment.h"

#include <clu/hash.h>

#include "mirai/message/forwarded_message.h"
#include "../detail/json.h"

namespace mpp
{
    bool Segment::compare_with_sv(const std::string_view sv) const noexcept
    {
        if (const auto* ptr = get_if<Plain>())
            return ptr->text == sv;
        return false;
    }

    Segment Segment::from_json(const detail::JsonElem json)
    {
        using namespace clu::literals;
        // @formatter:off
        switch (const std::string_view type = json["type"]; clu::fnv1a(type))
        {
            case "At"_fnv1a:         return At::from_json(json);
            case "AtAll"_fnv1a:      return AtAll::from_json(json);
            case "Face"_fnv1a:       return Face::from_json(json);
            case "Plain"_fnv1a:      return Plain::from_json(json);
            case "Image"_fnv1a:      return Image::from_json(json);
            case "FlashImage"_fnv1a: return FlashImage::from_json(json);
            case "Voice"_fnv1a:      return Voice::from_json(json);
            case "Xml"_fnv1a:        return Xml::from_json(json);
            case "Json"_fnv1a:       return Json::from_json(json);
            case "App"_fnv1a:        return App::from_json(json);
            case "Poke"_fnv1a:       return Poke::from_json(json);
            case "Forward"_fnv1a:    return Forward::from_json(json);
            default: throw std::runtime_error("未知的消息段类型");
        }
        // @formatter:on
    }
}
