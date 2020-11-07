#pragma once

#include <stdexcept>

namespace mpp
{
    /// Mirai API HTTP 状态码
    enum class MiraiStatus
    {
        success = 0,
        incorrect_auth_key = 1,
        bot_not_exist = 2,
        session_invalid_or_not_exist = 3,
        session_not_authorized_or_not_verified = 4,
        target_not_exist = 5,
        file_not_exist = 6,
        no_permission = 10,
        bot_muted = 20,
        message_too_long = 30,
        erroneous_access = 400
    };

    const char* get_description(MiraiStatus code); ///< 获取状态码对应的解释字符串

    /// Mirai API 调用异常类
    class MiraiException : public std::runtime_error
    {
    private:
        MiraiStatus code_;

    public:
        /// 用特定的状态码构建异常
        explicit MiraiException(const MiraiStatus code):
            runtime_error(get_description(code)), code_(static_cast<MiraiStatus>(code)) {}

        MiraiStatus status_code() const noexcept { return code_; } ///< 获取状态码
    };

    void check_status_code(MiraiStatus code) noexcept(false); ///< 检查状态码，若不为成功状态则抛出异常
}
