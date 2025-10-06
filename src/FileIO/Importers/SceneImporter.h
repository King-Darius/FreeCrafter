#pragma once

#include <string>

#include "FileIO/SceneIOFormat.h"

namespace Scene {
class Document;
}

namespace FileIO::Importers {

bool isFormatAvailable(SceneFormat format);
bool importScene(Scene::Document& document, const std::string& filePath, SceneFormat format, std::string* errorMessage = nullptr);

}
