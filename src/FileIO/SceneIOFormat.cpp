#include "SceneIOFormat.h"

#include <algorithm>
#include <cctype>
#include <iterator>

namespace FileIO {
namespace {
struct FormatMetadata {
    SceneFormat format;
    const char* displayName;
    const char* extension;
    bool requiresAssimp;
    bool requiresSkp;
};

constexpr FormatMetadata kFormats[] = {
    { SceneFormat::OBJ, "Wavefront OBJ", ".obj", false, false },
    { SceneFormat::STL, "Stereolithography", ".stl", false, false },
    { SceneFormat::FBX, "Autodesk FBX", ".fbx", true, false },
    { SceneFormat::DAE, "COLLADA DAE", ".dae", true, false },
    { SceneFormat::GLTF, "glTF 2.0", ".gltf", false, false },
    { SceneFormat::SKP, "SketchUp (*.skp)", ".skp", false, true }
};

const FormatMetadata* lookup(SceneFormat format)
{
    for (const auto& meta : kFormats) {
        if (meta.format == format) {
            return &meta;
        }
    }
    return nullptr;
}
}

std::string formatExtension(SceneFormat format)
{
    const auto* meta = lookup(format);
    return meta ? std::string(meta->extension) : std::string();
}

std::string formatDisplayName(SceneFormat format)
{
    const auto* meta = lookup(format);
    return meta ? std::string(meta->displayName) : std::string();
}

std::string formatFilterString(SceneFormat format)
{
    const auto* meta = lookup(format);
    if (!meta) {
        return {};
    }
    std::string filter = meta->displayName;
    filter += " (*";
    filter += meta->extension;
    filter += ")";
    return filter;
}

bool formatRequiresAssimp(SceneFormat format)
{
    const auto* meta = lookup(format);
    return meta ? meta->requiresAssimp : false;
}

bool formatRequiresSkp(SceneFormat format)
{
    const auto* meta = lookup(format);
    return meta ? meta->requiresSkp : false;
}

std::vector<SceneFormat> allSceneFormats()
{
    std::vector<SceneFormat> formats;
    formats.reserve(std::size(kFormats));
    for (const auto& meta : kFormats) {
        formats.push_back(meta.format);
    }
    return formats;
}

std::optional<SceneFormat> sceneFormatFromExtension(const std::string& extension)
{
    if (extension.empty()) {
        return std::nullopt;
    }
    std::string normalized = extension;
    if (normalized.front() != '.') {
        normalized.insert(normalized.begin(), '.');
    }
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    for (const auto& meta : kFormats) {
        if (normalized == meta.extension) {
            return meta.format;
        }
    }
    return std::nullopt;
}

bool hasAssimpRuntimeSupport()
{
#ifdef FREECRAFTER_HAS_ASSIMP
    return true;
#else
    return false;
#endif
}

bool hasSkpRuntimeSupport()
{
#ifdef FREECRAFTER_HAS_SKP_SDK
    return true;
#else
    return false;
#endif
}

bool isSceneFormatAvailable(SceneFormat format)
{
    if (formatRequiresAssimp(format) && !hasAssimpRuntimeSupport()) {
        return false;
    }
    if (formatRequiresSkp(format) && !hasSkpRuntimeSupport()) {
        return false;
    }
    return true;
}

std::vector<SceneFormat> availableSceneFormats()
{
    std::vector<SceneFormat> formats;
    for (auto format : allSceneFormats()) {
        if (isSceneFormatAvailable(format)) {
            formats.push_back(format);
        }
    }
    return formats;
}

}
