#include "mirai/event/event.h"

#include <clu/hash.h>

#include "mirai/event/event_types.h"

namespace mpp
{
    Event Event::from_json(const detail::JsonElem json)
    {
        using namespace clu::literals;
        const std::string_view type = json["type"];
        // @formatter:off
        switch (clu::fnv1a(type))
        {
            case "GroupMessage"_fnv1a:                         return GroupMessageEvent::from_json(json);
            case "FriendMessage"_fnv1a:                        return FriendMessageEvent::from_json(json);
            case "TempMessage"_fnv1a:                          return TempMessageEvent::from_json(json);
            case "BotOnlineEvent"_fnv1a:
            case "BotReloginEvent"_fnv1a:                      return BotOnlineEvent::from_json(json);
            case "BotOfflineEventActive"_fnv1a:
            case "BotOfflineEventForce"_fnv1a:
            case "BotOfflineEventDropped"_fnv1a:               return BotOfflineEvent::from_json(json);
            case "BotGroupPermissionChangeEvent"_fnv1a:        return BotGroupPermissionChangeEvent::from_json(json);
            case "BotMuteEvent"_fnv1a:                         return BotMutedEvent::from_json(json);
            case "BotUnmuteEvent"_fnv1a:                       return BotUnmutedEvent::from_json(json);
            case "BotJoinGroupEvent"_fnv1a:                    return BotJoinGroupEvent::from_json(json);
            case "BotLeaveEventActive"_fnv1a:                  return BotQuitEvent::from_json(json);
            case "BotLeaveEventKick"_fnv1a:                    return BotKickedEvent::from_json(json);
            case "GroupRecallEvent"_fnv1a:                     return GroupRecallEvent::from_json(json);
            case "FriendRecallEvent"_fnv1a:                    return FriendRecallEvent::from_json(json);
            case "GroupNameChangeEvent"_fnv1a:                 return GroupNameChangeEvent::from_json(json);
            case "GroupEntranceAnnouncementChangeEvent"_fnv1a: return GroupEntranceAnnouncementChangeEvent::from_json(json);
            case "GroupMuteAllEvent"_fnv1a:
            case "GroupAllowAnonymousChatEvent"_fnv1a:
            case "GroupAllowConfessTalkEvent"_fnv1a:
            case "GroupAllowMemberInviteEvent"_fnv1a:          return GroupConfigEvent::from_json(json);
            case "MemberJoinEvent"_fnv1a:                      return MemberJoinEvent::from_json(json);
            case "MemberLeaveEventKick"_fnv1a:                 return MemberKickedEvent::from_json(json);
            case "MemberLeaveEventQuit"_fnv1a:                 return MemberQuitEvent::from_json(json);
            case "MemberCardChangeEvent"_fnv1a:                return MemberCardChangeEvent::from_json(json);
            case "MemberSpecialTitleChangeEvent"_fnv1a:        return MemberSpecialTitleChangeEvent::from_json(json);
            case "MemberPermissionChangeEvent"_fnv1a:          return MemberPermissionChangeEvent::from_json(json);
            case "MemberMuteEvent"_fnv1a:                      return MemberMutedEvent::from_json(json);
            case "MemberUnmuteEvent"_fnv1a:                    return MemberUnmutedEvent::from_json(json);
            case "NewFriendRequestEvent"_fnv1a:                return NewFriendRequestEvent::from_json(json);
            case "MemberJoinRequestEvent"_fnv1a:               return MemberJoinRequestEvent::from_json(json);
            case "BotInvitedJoinGroupRequestEvent"_fnv1a:      return BotInvitedJoinGroupRequestEvent::from_json(json);
            default: throw std::runtime_error("未知的事件类型");
        }
        // @formatter:on
    }
}
