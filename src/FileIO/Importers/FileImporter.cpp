#include "FileImporter.h"

#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QObject>
#include <QtGlobal>

#include <cctype>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "GeometryKernel/Solid.h"
#include "GeometryKernel/GeometryKernel.h"
#include "GeometryKernel/HalfEdgeMesh.h"
#include "GeometryKernel/Vector3.h"
#include "GeometryKernel/Vector2.h"
#include "Scene/Document.h"
#include "FileIO/Importers/SceneImporter.h"

namespace FileIO::Importers {
namespace {

using Scene::Document;

Scene::Document::FileFormat detectFormat(const QString& path, Scene::Document::FileFormat hint)
{
    if (hint != Scene::Document::FileFormat::Auto)
        return hint;

    QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix == QLatin1String("obj"))
        return Scene::Document::FileFormat::Obj;
    if (suffix == QLatin1String("stl"))
        return Scene::Document::FileFormat::Stl;
    if (suffix == QLatin1String("gltf"))
        return Scene::Document::FileFormat::Gltf;
    if (suffix == QLatin1String("fbx"))
        return Scene::Document::FileFormat::Fbx;
    if (suffix == QLatin1String("dxf"))
        return Scene::Document::FileFormat::Dxf;
    if (suffix == QLatin1String("dwg"))
        return Scene::Document::FileFormat::Dwg;
    return Scene::Document::FileFormat::Auto;
}

struct VertexKey {
    int position = -1;
    int texCoord = -1;
    int normal = -1;

    bool operator==(const VertexKey& other) const
    {
        return position == other.position && texCoord == other.texCoord && normal == other.normal;
    }
};

struct VertexKeyHasher {
    std::size_t operator()(const VertexKey& key) const noexcept
    {
        std::size_t seed = static_cast<std::size_t>(key.position + 1);
        seed ^= static_cast<std::size_t>(key.texCoord + 1) << 8;
        seed ^= static_cast<std::size_t>(key.normal + 1) << 16;
        return seed;
    }
};

bool parseFaceVertex(const std::string& token, int vertexCount, int texCount, int normalCount,
                     VertexKey& key)
{
    if (token.empty())
        return false;

    int indices[3] = { -1, -1, -1 };
    int index = 0;
    std::string current;
    for (char ch : token) {
        if (ch == '/') {
            if (index < 3) {
                if (!current.empty()) {
                    indices[index] = std::stoi(current);
                }
                current.clear();
                ++index;
            }
        } else {
            current.push_back(ch);
        }
    }
    if (!current.empty() && index < 3) {
        indices[index] = std::stoi(current);
    }

    if (indices[0] == 0)
        return false;

    auto resolveIndex = [](int idx, int count) {
        if (idx > 0) {
            return idx - 1;
        }
        if (idx < 0) {
            return count + idx;
        }
        return -1;
    };

    key.position = resolveIndex(indices[0], vertexCount);
    key.texCoord = resolveIndex(indices[1], texCount);
    key.normal = resolveIndex(indices[2], normalCount);
    if (key.position < 0 || key.position >= vertexCount)
        return false;
    return true;
}

bool importObj(const QString& path, Document& document, ImportResult& result)
{
    std::ifstream stream(path.toStdString());
    if (!stream.is_open()) {
        result.errorMessage = QObject::tr("Unable to open OBJ file: %1").arg(path);
        return false;
    }

    std::vector<Vector3> positions;
    std::vector<Vector3> normals;
    std::vector<Vector2> texCoords;
    std::unordered_set<std::string> materialSet;
    std::vector<std::string> materialOrder;
    std::string activeMaterial;

    HalfEdgeMesh mesh;
    std::unordered_map<VertexKey, int, VertexKeyHasher> vertexMap;

    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty())
            continue;
        if (line[0] == '#')
            continue;
        std::istringstream ls(line);
        std::string command;
        ls >> command;
        if (command == "v") {
            float x = 0.0f, y = 0.0f, z = 0.0f;
            ls >> x >> y >> z;
            positions.emplace_back(x, y, z);
        } else if (command == "vn") {
            float x = 0.0f, y = 0.0f, z = 0.0f;
            ls >> x >> y >> z;
            normals.emplace_back(x, y, z);
        } else if (command == "vt") {
            float u = 0.0f, v = 0.0f;
            ls >> u >> v;
            texCoords.emplace_back(u, v);
        } else if (command == "usemtl") {
            ls >> activeMaterial;
            if (!activeMaterial.empty()) {
                if (materialSet.insert(activeMaterial).second) {
                    materialOrder.push_back(activeMaterial);
                }
            }
        } else if (command == "f") {
            std::vector<int> loop;
            std::string token;
            while (ls >> token) {
                VertexKey key;
                if (!parseFaceVertex(token, static_cast<int>(positions.size()), static_cast<int>(texCoords.size()),
                                     static_cast<int>(normals.size()), key)) {
                    continue;
                }
                auto found = vertexMap.find(key);
                int vertexIndex;
                if (found == vertexMap.end()) {
                    Vector3 normal;
                    bool hasNormal = false;
                    if (key.normal >= 0 && key.normal < static_cast<int>(normals.size())) {
                        normal = normals[static_cast<std::size_t>(key.normal)];
                        hasNormal = true;
                    }
                    Vector2 uv;
                    bool hasUV = false;
                    if (key.texCoord >= 0 && key.texCoord < static_cast<int>(texCoords.size())) {
                        uv = texCoords[static_cast<std::size_t>(key.texCoord)];
                        hasUV = true;
                    }
                    const Vector3& pos = positions[static_cast<std::size_t>(key.position)];
                    vertexIndex = mesh.addVertex(pos, normal, uv, hasNormal, hasUV);
                    vertexMap.emplace(key, vertexIndex);
                } else {
                    vertexIndex = found->second;
                }
                loop.push_back(vertexIndex);
            }
            if (loop.size() >= 3) {
                mesh.addFace(loop);
            }
        }
    }

    if (mesh.getVertices().empty()) {
        result.errorMessage = QObject::tr("No geometry found in OBJ file: %1").arg(path);
        return false;
    }

    mesh.recomputeNormals();

    auto solid = Solid::createFromMesh(std::move(mesh));
    if (!solid) {
        result.errorMessage = QObject::tr("Failed to create mesh from OBJ file: %1").arg(path);
        return false;
    }

    GeometryObject* object = document.geometry().addObject(std::move(solid));
    if (!object) {
        result.errorMessage = QObject::tr("Failed to register OBJ geometry");
        return false;
    }

    QString baseName = QFileInfo(path).completeBaseName();
    Scene::Document::ObjectId objectId = document.ensureObjectForGeometry(object, baseName.toStdString());
    ImportedObjectSummary summary;
    summary.objectId = objectId;
    summary.materialSlots = materialOrder;
    result.objects.push_back(summary);
    return true;
}

bool readBinaryStl(const QString& path, Document& document, ImportResult& result)
{
    std::ifstream stream(path.toStdString(), std::ios::binary);
    if (!stream.is_open()) {
        result.errorMessage = QObject::tr("Unable to open STL file: %1").arg(path);
        return false;
    }

    char header[80];
    stream.read(header, 80);
    if (!stream)
        return false;
    std::uint32_t triangleCount = 0;
    stream.read(reinterpret_cast<char*>(&triangleCount), sizeof(triangleCount));
    if (!stream)
        return false;

    HalfEdgeMesh mesh;
    for (std::uint32_t i = 0; i < triangleCount; ++i) {
        float nx = 0.0f, ny = 0.0f, nz = 0.0f;
        stream.read(reinterpret_cast<char*>(&nx), sizeof(float));
        stream.read(reinterpret_cast<char*>(&ny), sizeof(float));
        stream.read(reinterpret_cast<char*>(&nz), sizeof(float));
        Vector3 faceNormal(nx, ny, nz);
        int indices[3] = { -1, -1, -1 };
        for (int v = 0; v < 3; ++v) {
            float x = 0.0f, y = 0.0f, z = 0.0f;
            stream.read(reinterpret_cast<char*>(&x), sizeof(float));
            stream.read(reinterpret_cast<char*>(&y), sizeof(float));
            stream.read(reinterpret_cast<char*>(&z), sizeof(float));
            Vector3 position(x, y, z);
            indices[v] = mesh.addVertex(position, faceNormal, Vector2(), faceNormal.lengthSquared() > 0.0f, false);
        }
        std::uint16_t attribute = 0;
        stream.read(reinterpret_cast<char*>(&attribute), sizeof(attribute));
        if (!stream)
            return false;
        std::vector<int> loop{ indices[0], indices[1], indices[2] };
        mesh.addFace(loop);
    }

    if (mesh.getVertices().empty()) {
        result.errorMessage = QObject::tr("No geometry found in STL file: %1").arg(path);
        return false;
    }

    mesh.heal();
    mesh.recomputeNormals();

    auto solid = Solid::createFromMesh(std::move(mesh));
    if (!solid) {
        result.errorMessage = QObject::tr("Failed to create mesh from STL file: %1").arg(path);
        return false;
    }

    GeometryObject* object = document.geometry().addObject(std::move(solid));
    if (!object) {
        result.errorMessage = QObject::tr("Failed to register STL geometry");
        return false;
    }

    QString baseName = QFileInfo(path).completeBaseName();
    ImportedObjectSummary summary;
    summary.objectId = document.ensureObjectForGeometry(object, baseName.toStdString());
    result.objects.push_back(summary);
    return true;
}

bool importStl(const QString& path, Document& document, ImportResult& result)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        result.errorMessage = QObject::tr("Unable to open STL file: %1").arg(path);
        return false;
    }

    QByteArray header = file.peek(80);
    bool isAscii = header.startsWith("solid");
    file.close();

    if (!isAscii) {
        return readBinaryStl(path, document, result);
    }

    std::ifstream stream(path.toStdString());
    if (!stream.is_open()) {
        result.errorMessage = QObject::tr("Unable to open STL file: %1").arg(path);
        return false;
    }

    HalfEdgeMesh mesh;
    std::string line;
    Vector3 currentNormal;
    bool normalValid = false;
    while (std::getline(stream, line)) {
        std::istringstream ls(line);
        std::string token;
        ls >> token;
        if (token == "facet") {
            std::string normalKeyword;
            ls >> normalKeyword;
            if (normalKeyword == "normal") {
                float nx = 0.0f, ny = 0.0f, nz = 0.0f;
                ls >> nx >> ny >> nz;
                currentNormal = Vector3(nx, ny, nz);
                normalValid = currentNormal.lengthSquared() > 0.0f;
            }
        } else if (token == "vertex") {
            float x = 0.0f, y = 0.0f, z = 0.0f;
            ls >> x >> y >> z;
            mesh.addVertex(Vector3(x, y, z), currentNormal, Vector2(), normalValid, false);
        } else if (token == "endfacet") {
            std::size_t count = mesh.getVertices().size();
            if (count >= 3) {
                std::vector<int> loop{ static_cast<int>(count - 3), static_cast<int>(count - 2), static_cast<int>(count - 1) };
                mesh.addFace(loop);
            }
        }
    }

    if (mesh.getVertices().empty()) {
        result.errorMessage = QObject::tr("No geometry found in STL file: %1").arg(path);
        return false;
    }

    mesh.heal();
    mesh.recomputeNormals();

    auto solid = Solid::createFromMesh(std::move(mesh));
    if (!solid) {
        result.errorMessage = QObject::tr("Failed to create mesh from STL file: %1").arg(path);
        return false;
    }

    GeometryObject* object = document.geometry().addObject(std::move(solid));
    if (!object) {
        result.errorMessage = QObject::tr("Failed to register STL geometry");
        return false;
    }

    QString baseName = QFileInfo(path).completeBaseName();
    ImportedObjectSummary summary;
    summary.objectId = document.ensureObjectForGeometry(object, baseName.toStdString());
    result.objects.push_back(summary);
    return true;
}

bool importGltf(const QString& path, Document& document, ImportResult& result)
{
    Scene::Document tempDocument;
    std::string errorMessage;
    if (!importScene(tempDocument, path.toStdString(), FileIO::SceneFormat::GLTF, &errorMessage)) {
        if (!errorMessage.empty()) {
            result.errorMessage = QString::fromStdString(errorMessage);
        } else {
            result.errorMessage = QObject::tr("Failed to parse glTF file: %1").arg(path);
        }
        return false;
    }

    const auto& imported = tempDocument.geometry().getObjects();
    if (imported.empty()) {
        result.errorMessage = QObject::tr("glTF file did not contain importable meshes: %1").arg(path);
        return false;
    }

    std::unordered_map<GeometryObject::StableId, std::string> nameMap;
    std::vector<const Scene::Document::ObjectNode*> stack;
    stack.push_back(&tempDocument.objectTree());
    while (!stack.empty()) {
        const Scene::Document::ObjectNode* node = stack.back();
        stack.pop_back();
        if (!node)
            continue;
        if (node->geometry) {
            GeometryObject::StableId id = node->geometry->getStableId();
            if (id != 0)
                nameMap[id] = node->name;
        }
        for (const auto& child : node->children) {
            stack.push_back(child.get());
        }
    }

    const auto& sourceMaterials = tempDocument.geometry().getMaterials();
    for (const auto& objectPtr : imported) {
        if (!objectPtr)
            continue;

        std::unique_ptr<GeometryObject> clone = objectPtr->clone();
        if (!clone) {
            result.errorMessage = QObject::tr("Failed to clone glTF geometry: %1").arg(path);
            return false;
        }

        GeometryObject* target = document.geometry().addObject(std::move(clone));
        if (!target) {
            result.errorMessage = QObject::tr("Failed to register glTF geometry: %1").arg(path);
            return false;
        }

        GeometryObject::StableId sourceId = objectPtr->getStableId();
        auto materialIt = sourceMaterials.find(sourceId);
        if (materialIt != sourceMaterials.end()) {
            document.geometry().assignMaterial(target, materialIt->second);
        }

        std::string nodeName;
        if (auto nameIt = nameMap.find(sourceId); nameIt != nameMap.end()) {
            nodeName = nameIt->second;
        } else {
            nodeName = QFileInfo(path).completeBaseName().toStdString();
        }

        Scene::Document::ObjectId objectId = document.ensureObjectForGeometry(target, nodeName);
        ImportedObjectSummary summary;
        summary.objectId = objectId;
        if (materialIt != sourceMaterials.end()) {
            summary.materialSlots.push_back(materialIt->second);
        }
        result.objects.push_back(std::move(summary));
    }

    return !result.objects.empty();
}

} // namespace

ImportResult FileImporter::import(const QString& filePath, Scene::Document& document, Scene::Document::FileFormat hint)
{
    ImportResult result;
    if (filePath.isEmpty()) {
        result.errorMessage = QObject::tr("Import path is empty");
        return result;
    }

    QFileInfo info(filePath);
    if (!info.exists()) {
        result.errorMessage = QObject::tr("File does not exist: %1").arg(filePath);
        return result;
    }

    auto format = detectFormat(filePath, hint);
    result.resolvedFormat = format;

    bool ok = false;
    switch (format) {
    case Scene::Document::FileFormat::Obj:
        ok = importObj(filePath, document, result);
        break;
    case Scene::Document::FileFormat::Stl:
        ok = importStl(filePath, document, result);
        break;
    case Scene::Document::FileFormat::Gltf:
        ok = importGltf(filePath, document, result);
        break;
    case Scene::Document::FileFormat::Fbx:
    case Scene::Document::FileFormat::Dxf:
    case Scene::Document::FileFormat::Dwg:
#ifdef FREECRAFTER_USE_ASSIMP
        Q_UNUSED(document);
        Q_UNUSED(filePath);
        result.errorMessage = QObject::tr("Assimp support not yet implemented for this format");
#else
        result.errorMessage = QObject::tr("Assimp support not available in this build");
#endif
        ok = false;
        break;
    case Scene::Document::FileFormat::Auto:
    default:
        result.errorMessage = QObject::tr("Unsupported import format for file: %1").arg(filePath);
        ok = false;
        break;
    }

    if (ok && result.objects.empty()) {
        result.errorMessage = QObject::tr("No geometry imported from %1").arg(filePath);
        ok = false;
    }

    result.success = ok;
    return result;
}

} // namespace FileIO::Importers
