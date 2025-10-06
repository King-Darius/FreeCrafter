#pragma once

#include <optional>
#include <string>
#include <vector>

#include "FileIO/SceneIOFormat.h"

namespace Scene {
class Document;
}

namespace FileIO::Exporters {

bool isFormatAvailable(SceneFormat format);
std::vector<SceneFormat> supportedFormats();
std::vector<std::string> supportedFormatFilters();
std::optional<SceneFormat> guessFormatFromFilename(const std::string& filename);
bool exportScene(const Scene::Document& document, const std::string& filePath, SceneFormat format, std::string* errorMessage = nullptr);

}
