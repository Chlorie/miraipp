#pragma once

#include <cstdint>

namespace mpp
{
    struct FriendMessageEvent;
    struct GroupMessageEvent;
    struct TempMessageEvent;
    struct BotOnlineEvent;
    struct BotOfflineEvent;
    struct BotGroupPermissionChangeEvent;
    struct BotMutedEvent;
    struct BotUnmutedEvent;
    struct BotJoinGroupEvent;
    struct BotQuitEvent;
    struct BotKickedEvent;
    struct GroupRecallEvent;
    struct FriendRecallEvent;
    struct GroupNameChangeEvent;
    struct GroupEntranceAnnouncementChangeEvent;
    struct GroupConfigEvent;
    struct MemberJoinEvent;
    struct MemberQuitEvent;
    struct MemberKickedEvent;
    struct MemberCardChangeEvent;
    struct MemberSpecialTitleChangeEvent;
    struct MemberPermissionChangeEvent;
    struct MemberMutedEvent;
    struct MemberUnmutedEvent;
    struct NewFriendRequestEvent;
    struct MemberJoinRequestEvent;
    struct BotInvitedJoinGroupRequestEvent;

    enum class NewFriendResponseType : uint8_t;
    enum class MemberJoinResponseType : uint8_t;
    enum class BotInvitedJoinGroupResponseType : uint8_t;
}
