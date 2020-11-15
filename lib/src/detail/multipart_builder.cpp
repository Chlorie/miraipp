#include "mirai/detail/multipart_builder.h"

#include <fmt/core.h>
#include <fstream>
#include <clu/take.h>

namespace mpp::detail
{
    void MultipartBuilder::add_key_value(const std::string_view key, const std::string_view value)
    {
        str_ += fmt::format("\r\n"
            "--{}\r\n"
            "Content-Disposition: form-data; name=\"{}\"\r\n"
            "\r\n"
            "{}",
            boundary, key, value);
    }

    void MultipartBuilder::add_file(const std::string_view key, const std::filesystem::path& path)
    {
        std::ifstream fs(path, std::ios::in | std::ios::binary);
        if (fs.fail()) throw std::runtime_error("failed to open binary file");

        str_ += fmt::format("\r\n"
            "--{}\r\n"
            "Content-Disposition: form-data; name=\"{}\"; filename=\"{}\"\r\n"
            "Content-Type: application/octet-stream\r\n"
            "\r\n",
            boundary, key, path.filename().string());

        fs.ignore(std::numeric_limits<std::streamsize>::max());
        const std::streamsize size = fs.gcount();
        str_.reserve(str_.size() + size + closing_boundary.size());
        char* data = str_.data() + str_.size();
        str_.resize(str_.size() + size);
        fs.seekg(0, std::ios::beg);
        fs.read(data, size);
        if (fs.fail() && !fs.eof()) throw std::runtime_error("failed to read binary file");
    }

    std::string MultipartBuilder::take_string()
    {
        str_ += closing_boundary;
        return clu::take(str_);
    }
}
