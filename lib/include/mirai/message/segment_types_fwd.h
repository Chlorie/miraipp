#pragma once

namespace mpp
{
    struct At;
    struct AtAll;
    struct Face;
    struct Plain;
    struct Image;
    struct FlashImage;
    struct Voice;
    struct Xml;
    struct Json;
    struct App;
    struct Poke;
    struct Forward;
    struct File;
    
    /// 消息段类型枚举
    enum class SegmentType : uint8_t
    {
        at, at_all, face, plain, image,
        flash_image, voice, xml, json, app,
        poke, forward, file
    };

    /// 消息段组成类型概念
    template <typename T> concept ConcreteSegment = requires { { T::type } -> std::convertible_to<SegmentType>; };
}
