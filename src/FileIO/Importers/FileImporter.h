#pragma once

#include <QString>
#include <string>
#include <vector>

#include "Scene/Document.h"

namespace FileIO::Importers {

struct ImportedObjectSummary {
    Scene::Document::ObjectId objectId = 0;
    std::vector<std::string> materialSlots;
};

struct ImportResult {
    bool success = false;
    QString errorMessage;
    Scene::Document::FileFormat resolvedFormat = Scene::Document::FileFormat::Auto;
    std::vector<ImportedObjectSummary> objects;
};

class FileImporter {
public:
    static ImportResult import(const QString& filePath, Scene::Document& document,
                               Scene::Document::FileFormat hint);
};

} // namespace FileIO::Importers
