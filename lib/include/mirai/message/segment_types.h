#pragma once

#include <optional>
#include <string>

#include "segment_types_fwd.h"
#include "../core/common.h"
#include "../core/export.h"
#include "../core/format.h"
#include "../detail/json_fwd.h"

namespace mpp
{
    MPP_SUPPRESS_EXPORT_WARNING
    /// at 消息段
    struct MPP_API At final
    {
        static constexpr SegmentType type = SegmentType::at;

        UserId target{}; ///< 被 at 的用户 id
        std::string display{}; ///< 该 at 消息段的字符串表示

        bool operator==(const At& other) const noexcept { return target == other.target; }

        void format_to(fmt::format_context& ctx) const;
        void format_as_json(fmt::format_context& ctx) const;
        static At from_json(detail::JsonElem json);
    };

    /// at 全体成员消息段
    struct MPP_API AtAll final
    {
        static constexpr SegmentType type = SegmentType::at_all;

        friend bool operator==(AtAll, AtAll) noexcept { return true; }

        void format_to(fmt::format_context& ctx) const { fmt::format_to(ctx.out(), "@全体成员"); }
        void format_as_json(fmt::format_context& ctx) const { fmt::format_to(ctx.out(), R"({{"type":"AtAll"}})"); }
        static AtAll from_json(detail::JsonElem);
    };

    /// QQ 表情消息段
    struct MPP_API Face final
    {
        static constexpr SegmentType type = SegmentType::face;

        std::optional<int32_t> face_id = 0; ///< 表情编号
        std::optional<std::string> name{}; ///< 表情对应的拼音版本

        bool operator==(const Face& other) const noexcept;

        void format_to(fmt::format_context& ctx) const;
        void format_as_json(fmt::format_context& ctx) const;
        static Face from_json(detail::JsonElem json);
    };

    /// 纯文本消息段
    struct MPP_API Plain final
    {
        static constexpr SegmentType type = SegmentType::plain;

        std::string text; ///< 文本内容

        bool operator==(const Plain&) const noexcept = default;
        bool operator==(const std::string_view other) const noexcept { return text == other; }

        void format_to(fmt::format_context& ctx) const { fmt::format_to(ctx.out(), "{}", text); }
        void format_as_json(fmt::format_context& ctx) const;
        static Plain from_json(detail::JsonElem json);
    };

    /// 图片消息段
    struct MPP_API Image final
    {
        static constexpr SegmentType type = SegmentType::image;

        std::optional<std::string> image_id{}; ///< 图片 id
        std::optional<std::string> url{}; ///< 图片的链接，发送时为网络图片链接，接收时为腾讯图片服务器中图片的链接
        std::optional<std::string> path{}; ///< 图片的本地路径，相对 plugins/MiraiAPIHTTP/images

        bool operator==(const Image& other) const noexcept;

        void format_to(fmt::format_context& ctx) const;
        void format_as_json(fmt::format_context& ctx) const;
        static Image from_json(detail::JsonElem json);
    };

    /// 闪照消息段
    struct MPP_API FlashImage final
    {
        static constexpr SegmentType type = SegmentType::flash_image;

        std::optional<std::string> image_id{}; ///< 图片 id
        std::optional<std::string> url{}; ///< 图片的链接，发送时为网络图片链接，接收时为腾讯图片服务器中图片的链接
        std::optional<std::string> path{}; ///< 图片的本地路径，相对 plugins/MiraiAPIHTTP/images

        bool operator==(const FlashImage& other) const noexcept;

        void format_to(fmt::format_context& ctx) const;
        void format_as_json(fmt::format_context& ctx) const;
        static FlashImage from_json(detail::JsonElem json);
    };

    /// 语音消息段
    struct MPP_API Voice final
    {
        static constexpr SegmentType type = SegmentType::voice;

        std::optional<std::string> voice_id{}; ///< 语音 id
        std::optional<std::string> url{}; ///< 语音的链接，发送时为网络语音链接，接收时为腾讯语音服务器中语音的链接
        std::optional<std::string> path{}; ///< 语音的本地路径，相对 plugins/MiraiAPIHTTP/voices

        bool operator==(const Voice& other) const noexcept;

        void format_to(fmt::format_context& ctx) const;
        void format_as_json(fmt::format_context& ctx) const;
        static Voice from_json(detail::JsonElem json);
    };

    /// xml 消息段
    struct MPP_API Xml final
    {
        static constexpr SegmentType type = SegmentType::xml;

        std::string xml; ///< xml 文本

        bool operator==(const Xml&) const noexcept = default;

        void format_to(fmt::format_context& ctx) const { fmt::format_to(ctx.out(), "{}", xml); }
        void format_as_json(fmt::format_context& ctx) const;
        static Xml from_json(detail::JsonElem json);
    };

    /// json 消息段
    struct MPP_API Json final
    {
        static constexpr SegmentType type = SegmentType::json;

        std::string json; ///< json 文本

        bool operator==(const Json&) const noexcept = default;

        void format_to(fmt::format_context& ctx) const { fmt::format_to(ctx.out(), "{}", json); }
        void format_as_json(fmt::format_context& ctx) const;
        static Json from_json(detail::JsonElem json);
    };

    /// 小程序消息段
    struct MPP_API App final
    {
        static constexpr SegmentType type = SegmentType::app;

        std::string content; ///< 内容

        bool operator==(const App&) const noexcept = default;

        void format_to(fmt::format_context& ctx) const { fmt::format_to(ctx.out(), "{}", content); }
        void format_as_json(fmt::format_context& ctx) const;
        static App from_json(detail::JsonElem json);
    };

    /// 戳一戳消息段
    struct MPP_API Poke final
    {
        static constexpr SegmentType type = SegmentType::poke;

        std::string name; ///< 戳一戳消息的名称

        bool operator==(const Poke&) const noexcept = default;

        void format_to(fmt::format_context& ctx) const { fmt::format_to(ctx.out(), "[戳一戳 {}]", name); }
        void format_as_json(fmt::format_context& ctx) const;
        static Poke from_json(detail::JsonElem json);
    };

    struct ForwardedMessage;

    /// 合并转发消息段
    struct MPP_API Forward final
    {
        static constexpr SegmentType type = SegmentType::forward;

        std::string title; ///< 标题
        std::string brief; ///< 简介
        std::string source;
        std::string summary;
        std::vector<ForwardedMessage> messages;

        bool operator==(const Forward&) const noexcept = default;

        void format_to(fmt::format_context& ctx) const { fmt::format_to(ctx.out(), "{}", brief); }
        void format_as_json(fmt::format_context& ctx) const;
        static Forward from_json(detail::JsonElem json);
    };

    /// 文件消息段
    struct MPP_API File final
    {
        static constexpr SegmentType type = SegmentType::file;
    };
    MPP_RESTORE_EXPORT_WARNING
}
