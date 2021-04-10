#pragma once

#include <string>
#include <filesystem>

namespace mpp::detail
{
    class MultipartBuilder // NOLINT(cppcoreguidelines-special-member-functions)
    {
    private:
        static constexpr std::string_view closing_boundary = "\r\n--miraippI7a60ebtQBvVicm91sqr2g--\r\n";
        static constexpr std::string_view boundary = "miraippI7a60ebtQBvVicm91sqr2g";

    public:
        static constexpr std::string_view content_type = "multipart/form-data; boundary=miraippI7a60ebtQBvVicm91sqr2g";

    private:
        std::string str_;

    public:
        void add_key_value(std::string_view key, std::string_view value);
        void add_file(std::string_view key, const std::filesystem::path& path);
        std::string take_string();
    };
}
