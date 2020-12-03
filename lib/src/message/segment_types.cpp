#include "mirai/message/segment_types.h"

namespace mpp
{
    void At::format_to(fmt::format_context& ctx) const
    {
        if (!display.empty()) (void)fmt::format_to(ctx.out(), "{}", display);
        fmt::format_to(ctx.out(), "@{}", target.id);
    }

    void At::format_as_json(fmt::format_context& ctx) const
    {
        detail::JsonObjScope obj(ctx);
        obj.add_entry("type", "At");
        obj.add_entry("target", target.id);
    }

    At At::from_json(const detail::JsonElem json)
    {
        return
        {
            .target = UserId(json["target"]),
            .display = std::string(json["display"])
        };
    }

    bool Face::operator==(const Face& other) const noexcept
    {
        if (face_id.has_value() && face_id == other.face_id) return true;
        return name == other.name;
    }

    void Face::format_to(fmt::format_context& ctx) const
    {
        if (name) return (void)fmt::format_to(ctx.out(), "[表情 {}]", *name);
        if (face_id) return (void)fmt::format_to(ctx.out(), "[表情 {}]", *face_id);
        fmt::format_to(ctx.out(), "[表情]");
    }

    void Face::format_as_json(fmt::format_context& ctx) const
    {
        detail::JsonObjScope obj(ctx);
        obj.add_entry("type", "Face");
        obj.add_entry("faceId", face_id);
        obj.add_entry("name", name);
    }

    Face Face::from_json(const detail::JsonElem json)
    {
        return
        {
            .face_id = detail::from_json<std::optional<uint32_t>>(json["faceId"]),
            .name = detail::from_json<std::string>(json["name"])
        };
    }

    void Plain::format_as_json(fmt::format_context& ctx) const
    {
        detail::JsonObjScope obj(ctx);
        obj.add_entry("type", "Plain");
        obj.add_entry("text", text);
    }

    bool Image::operator==(const Image& other) const noexcept
    {
        if (image_id.has_value() && image_id == other.image_id) return true;
        if (url.has_value() && url == other.url) return true;
        return path == other.path;
    }

    void Image::format_to(fmt::format_context& ctx) const
    {
        if (image_id) return (void)fmt::format_to(ctx.out(), "{}", *image_id);
        if (url) return (void)fmt::format_to(ctx.out(), "{}", *url);
        if (path) return (void)fmt::format_to(ctx.out(), "{}", *path);
        fmt::format_to(ctx.out(), "null");
    }

    void Image::format_as_json(fmt::format_context& ctx) const
    {
        detail::JsonObjScope obj(ctx);
        obj.add_entry("type", "Image");
        if (image_id) return obj.add_entry("imageId", image_id);
        if (url) return obj.add_entry("url", url);
        if (path) return obj.add_entry("path", path);
    }

    Image Image::from_json(const detail::JsonElem json)
    {
        return
        {
            .image_id = detail::from_json<std::optional<std::string>>(json["imageId"]),
            .url = detail::from_json<std::optional<std::string>>(json["url"]),
            .path = detail::from_json<std::optional<std::string>>(json["path"])
        };
    }

    bool FlashImage::operator==(const FlashImage& other) const noexcept
    {
        if (image_id.has_value() && image_id == other.image_id) return true;
        if (url.has_value() && url == other.url) return true;
        return path == other.path;
    }

    void FlashImage::format_to(fmt::format_context& ctx) const
    {
        if (image_id) return (void)fmt::format_to(ctx.out(), "[图片 {}]", *image_id);
        if (url) return (void)fmt::format_to(ctx.out(), "[图片 {}]", *url);
        if (path) return (void)fmt::format_to(ctx.out(), "[图片 {}]", *path);
        fmt::format_to(ctx.out(), "[图片]");
    }

    void FlashImage::format_as_json(fmt::format_context& ctx) const
    {
        detail::JsonObjScope obj(ctx);
        obj.add_entry("type", "FlashImage");
        if (image_id) return obj.add_entry("imageId", image_id);
        if (url) return obj.add_entry("url", url);
        if (path) return obj.add_entry("path", path);
    }

    FlashImage FlashImage::from_json(const detail::JsonElem json)
    {
        return
        {
            .image_id = detail::from_json<std::optional<std::string>>(json["imageId"]),
            .url = detail::from_json<std::optional<std::string>>(json["url"]),
            .path = detail::from_json<std::optional<std::string>>(json["path"])
        };
    }

    bool Voice::operator==(const Voice& other) const noexcept
    {
        if (voice_id.has_value() && voice_id == other.voice_id) return true;
        if (url.has_value() && url == other.url) return true;
        return path == other.path;
    }

    void Voice::format_to(fmt::format_context& ctx) const
    {
        if (voice_id) return (void)fmt::format_to(ctx.out(), "[语音 {}]", *voice_id);
        if (url) return (void)fmt::format_to(ctx.out(), "[语音 {}]", *url);
        if (path) return (void)fmt::format_to(ctx.out(), "[语音 {}]", *path);
        fmt::format_to(ctx.out(), "[语音]");
    }

    void Voice::format_as_json(fmt::format_context& ctx) const
    {
        detail::JsonObjScope obj(ctx);
        obj.add_entry("type", "Voice");
        if (voice_id) return obj.add_entry("voiceId", voice_id);
        if (url) return obj.add_entry("url", url);
        if (path) return obj.add_entry("path", path);
    }

    Voice Voice::from_json(const detail::JsonElem json)
    {
        return
        {
            .voice_id = detail::from_json<std::optional<std::string>>(json["voiceId"]),
            .url = detail::from_json<std::optional<std::string>>(json["url"]),
            .path = detail::from_json<std::optional<std::string>>(json["path"])
        };
    }

    void Xml::format_as_json(fmt::format_context& ctx) const
    {
        detail::JsonObjScope obj(ctx);
        obj.add_entry("type", "Xml");
        obj.add_entry("xml", xml);
    }

    void Json::format_as_json(fmt::format_context& ctx) const
    {
        detail::JsonObjScope obj(ctx);
        obj.add_entry("type", "Json");
        obj.add_entry("json", json);
    }

    void App::format_as_json(fmt::format_context& ctx) const
    {
        detail::JsonObjScope obj(ctx);
        obj.add_entry("type", "App");
        obj.add_entry("content", content);
    }

    void Poke::format_as_json(fmt::format_context& ctx) const
    {
        detail::JsonObjScope obj(ctx);
        obj.add_entry("type", "Poke");
        obj.add_entry("name", name);
    }
}
