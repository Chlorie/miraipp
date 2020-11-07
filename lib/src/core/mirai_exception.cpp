#include "mirai/core/mirai_exception.h"

namespace mpp
{
    const char* get_description(const MiraiStatus code)
    {
        using enum MiraiStatus;
        // @formatter:off
        switch (code)
        {
            case success:                                   return "正常";
            case incorrect_auth_key:                        return "错误的auth key";
            case bot_not_exist:                             return "指定的Bot不存在";
            case session_invalid_or_not_exist:              return "Session失效或不存在";
            case session_not_authorized_or_not_verified:    return "Session未认证(未激活)";
            case target_not_exist:                          return "发送消息目标不存在(指定对象不存在)";
            case file_not_exist:                            return "指定文件不存在，出现于发送本地图片";
            case no_permission:                             return "无操作权限，指Bot没有对应操作的限权";
            case bot_muted:                                 return "Bot被禁言，指Bot当前无法向指定群发送消息";
            case message_too_long:                          return "消息过长";
            case erroneous_access:                          return "错误的访问，如参数错误等";
            default:                                        return "未知的错误";
        }
        // @formatter:on
    }

    void check_status_code(const MiraiStatus code)
    {
        if (code != MiraiStatus::success)
            throw MiraiException(code);
    }
}
