#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>

namespace Scene {

class Document;

class SceneSerializer {
public:
    struct Result {
        enum class Status {
            Success,
            FormatMismatch,
            Error
        } status = Status::Error;
        std::string message;

        static Result success();
        static Result formatMismatch();
        static Result failure(const std::string& message);
    };

    static Result save(const Document& document, const std::string& path);
    static Result load(Document& document, const std::string& path);

    static constexpr std::uint16_t kSupportedMajorVersion = 1;
    static constexpr std::uint16_t kSupportedMinorVersion = 0;

private:
    struct Header {
        char magic[4];
        std::uint16_t majorVersion;
        std::uint16_t minorVersion;
        std::uint32_t jsonByteLength;
        std::uint32_t geometryByteLength;
    };

    static Result saveToStream(const Document& document, std::ostream& stream);
    static Result loadFromStream(Document& document, std::istream& stream);
};

} // namespace Scene

