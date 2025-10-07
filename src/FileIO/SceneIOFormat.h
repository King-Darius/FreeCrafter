#pragma once

#include <optional>
#include <string>
#include <vector>

namespace FileIO {

enum class SceneFormat {
    OBJ,
    STL,
    FBX,
    DAE,
    GLTF,
    SKP
};

std::string formatExtension(SceneFormat format);
std::string formatDisplayName(SceneFormat format);
std::string formatFilterString(SceneFormat format);
bool formatRequiresAssimp(SceneFormat format);
bool formatRequiresSkp(SceneFormat format);
std::vector<SceneFormat> allSceneFormats();
std::optional<SceneFormat> sceneFormatFromExtension(const std::string& extension);
bool hasAssimpRuntimeSupport();
bool hasSkpRuntimeSupport();
bool isSceneFormatAvailable(SceneFormat format);
std::vector<SceneFormat> availableSceneFormats();

}
