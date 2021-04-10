#include "mirai/message/forwarded_message.h"

#include "../detail/json.h"

namespace mpp
{
    void ForwardedMessage::format_as_json(fmt::format_context& ctx) const
    {
        detail::JsonObjScope obj(ctx);
        obj.add_entry("senderId", sender.id);
        obj.add_entry("time", time);
        obj.add_entry("senderName", sender_name);
        obj.add_entry("messageChain", content);
    }

    ForwardedMessage ForwardedMessage::from_json(const detail::JsonElem json)
    {
        return
        {
            .sender = UserId(json["senderId"]),
            .time = detail::from_json<int32_t>(json["time"]),
            .sender_name = std::string(json["senderName"]),
            .content = detail::from_json<Message>(json["messageChain"])
        };
    }
}
