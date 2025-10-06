#include "SceneImporter.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "GeometryKernel/GeometryKernel.h"
#include "GeometryKernel/Solid.h"
#include "Scene/Document.h"

namespace FileIO::Importers {
namespace {

bool hasAssimpSupport()
{
#ifdef FREECRAFTER_HAS_ASSIMP
    return true;
#else
    return false;
#endif
}

bool hasSkpSupport()
{
#ifdef FREECRAFTER_HAS_SKP_SDK
    return true;
#else
    return false;
#endif
}

bool formatEnabled(SceneFormat format)
{
    if (formatRequiresAssimp(format) && !hasAssimpSupport()) {
        return false;
    }
    if (formatRequiresSkp(format) && !hasSkpSupport()) {
        return false;
    }
    return true;
}

std::string trim(const std::string& input)
{
    const auto first = std::find_if_not(input.begin(), input.end(), [](unsigned char c) { return std::isspace(c); });
    const auto last = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) { return std::isspace(c); }).base();
    if (first >= last) {
        return {};
    }
    return std::string(first, last);
}

struct ImportedMesh {
    std::string name;
    std::vector<Vector3> positions;
    std::vector<std::uint32_t> indices;
    std::string material;
    std::array<float, 16> transform = GeometryKernel::identityTransform();
};

bool parseObj(const std::filesystem::path& path, std::vector<ImportedMesh>& meshes, std::string* error)
{
    std::ifstream file(path);
    if (!file) {
        if (error) {
            *error = "Unable to open OBJ file";
        }
        return false;
    }

    std::vector<Vector3> globalPositions;
    ImportedMesh current;
    bool haveCurrent = false;
    std::unordered_map<int, std::uint32_t> remap;
    std::size_t counter = 0;

    auto startObject = [&](const std::string& name) {
        if (haveCurrent && !current.indices.empty()) {
            meshes.push_back(current);
        }
        current = ImportedMesh{};
        current.name = name.empty() ? std::string("Object_") + std::to_string(++counter) : name;
        current.transform = GeometryKernel::identityTransform();
        remap.clear();
        haveCurrent = true;
    };

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream stream(line);
        std::string tag;
        stream >> tag;
        if (tag == "v") {
            float x = 0.0f, y = 0.0f, z = 0.0f;
            stream >> x >> y >> z;
            globalPositions.emplace_back(x, y, z);
        } else if (tag == "o" || tag == "g") {
            std::string name;
            std::getline(stream, name);
            startObject(trim(name));
        } else if (tag == "usemtl") {
            std::string name;
            stream >> name;
            if (!haveCurrent) {
                startObject(std::string());
            }
            current.material = name;
        } else if (tag == "f") {
            if (!haveCurrent) {
                startObject(std::string());
            }
            std::vector<std::uint32_t> faceIndices;
            std::string token;
            while (stream >> token) {
                if (token.empty()) {
                    continue;
                }
                std::string_view part(token);
                auto slash = part.find('/');
                if (slash != std::string_view::npos) {
                    part = part.substr(0, slash);
                }
                int index = std::stoi(std::string(part));
                if (index < 0) {
                    index = static_cast<int>(globalPositions.size()) + index + 1;
                }
                index -= 1;
                if (index < 0 || static_cast<std::size_t>(index) >= globalPositions.size()) {
                    continue;
                }
                auto it = remap.find(index);
                if (it == remap.end()) {
                    std::uint32_t mapped = static_cast<std::uint32_t>(current.positions.size());
                    current.positions.push_back(globalPositions[static_cast<std::size_t>(index)]);
                    remap[index] = mapped;
                    faceIndices.push_back(mapped);
                } else {
                    faceIndices.push_back(it->second);
                }
            }
            if (faceIndices.size() >= 3) {
                for (std::size_t i = 1; i + 1 < faceIndices.size(); ++i) {
                    current.indices.push_back(faceIndices[0]);
                    current.indices.push_back(faceIndices[i]);
                    current.indices.push_back(faceIndices[i + 1]);
                }
            }
        }
    }

    if (haveCurrent && !current.indices.empty()) {
        meshes.push_back(current);
    }

    if (meshes.empty()) {
        if (error) {
            *error = "OBJ file contained no mesh data";
        }
        return false;
    }
    return true;
}

bool parseStl(const std::filesystem::path& path, std::vector<ImportedMesh>& meshes, std::string* error)
{
    std::ifstream file(path);
    if (!file) {
        if (error) {
            *error = "Unable to open STL file";
        }
        return false;
    }

    ImportedMesh mesh;
    mesh.name = path.stem().string();
    mesh.transform = GeometryKernel::identityTransform();

    std::string token;
    while (file >> token) {
        if (token == "vertex") {
            float x = 0.0f, y = 0.0f, z = 0.0f;
            file >> x >> y >> z;
            mesh.positions.emplace_back(x, y, z);
            mesh.indices.push_back(static_cast<std::uint32_t>(mesh.positions.size() - 1));
        }
    }

    if (mesh.indices.empty()) {
        if (error) {
            *error = "STL file did not contain any facets";
        }
        return false;
    }
    meshes.push_back(std::move(mesh));
    return true;
}

int componentWidth(int componentType)
{
    switch (componentType) {
    case 5126: // float
        return 4;
    case 5125: // unsigned int
        return 4;
    case 5123: // unsigned short
        return 2;
    case 5121: // unsigned byte
        return 1;
    default:
        return 0;
    }
}

int typeElementCount(const QString& type)
{
    if (type == "SCALAR") {
        return 1;
    }
    if (type == "VEC2") {
        return 2;
    }
    if (type == "VEC3") {
        return 3;
    }
    if (type == "VEC4") {
        return 4;
    }
    if (type == "MAT4") {
        return 16;
    }
    return 0;
}

std::vector<Vector3> readVec3Accessor(const QJsonArray& bufferViews,
                                      const QJsonArray& accessors,
                                      int accessorIndex,
                                      const QByteArray& binary)
{
    std::vector<Vector3> result;
    if (accessorIndex < 0 || accessorIndex >= accessors.size()) {
        return result;
    }
    QJsonObject accessor = accessors[accessorIndex].toObject();
    int bufferViewIndex = accessor.value("bufferView").toInt(-1);
    if (bufferViewIndex < 0 || bufferViewIndex >= bufferViews.size()) {
        return result;
    }
    QJsonObject view = bufferViews[bufferViewIndex].toObject();
    int componentType = accessor.value("componentType").toInt();
    int elements = typeElementCount(accessor.value("type").toString());
    if (componentType != 5126 || elements != 3) {
        return result;
    }
    int componentSize = componentWidth(componentType);
    int stride = view.value("byteStride").toInt(componentSize * elements);
    int count = accessor.value("count").toInt();
    int accessorOffset = accessor.value("byteOffset").toInt(0);
    int viewOffset = view.value("byteOffset").toInt(0);
    std::size_t base = static_cast<std::size_t>(viewOffset + accessorOffset);
    result.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        std::size_t offset = base + static_cast<std::size_t>(i * stride);
        if (offset + 3 * componentSize > static_cast<std::size_t>(binary.size())) {
            break;
        }
        float values[3];
        std::memcpy(&values[0], binary.constData() + static_cast<int>(offset), sizeof(float));
        std::memcpy(&values[1], binary.constData() + static_cast<int>(offset + componentSize), sizeof(float));
        std::memcpy(&values[2], binary.constData() + static_cast<int>(offset + 2 * componentSize), sizeof(float));
        result.emplace_back(values[0], values[1], values[2]);
    }
    return result;
}

std::vector<std::uint32_t> readIndexAccessor(const QJsonArray& bufferViews,
                                             const QJsonArray& accessors,
                                             int accessorIndex,
                                             const QByteArray& binary)
{
    std::vector<std::uint32_t> result;
    if (accessorIndex < 0 || accessorIndex >= accessors.size()) {
        return result;
    }
    QJsonObject accessor = accessors[accessorIndex].toObject();
    int bufferViewIndex = accessor.value("bufferView").toInt(-1);
    if (bufferViewIndex < 0 || bufferViewIndex >= bufferViews.size()) {
        return result;
    }
    QJsonObject view = bufferViews[bufferViewIndex].toObject();
    int componentType = accessor.value("componentType").toInt();
    int componentSize = componentWidth(componentType);
    int elements = typeElementCount(accessor.value("type").toString());
    if (elements != 1 || componentSize == 0) {
        return result;
    }
    int stride = view.value("byteStride").toInt(componentSize * elements);
    int count = accessor.value("count").toInt();
    int accessorOffset = accessor.value("byteOffset").toInt(0);
    int viewOffset = view.value("byteOffset").toInt(0);
    std::size_t base = static_cast<std::size_t>(viewOffset + accessorOffset);
    result.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        std::size_t offset = base + static_cast<std::size_t>(i * stride);
        if (offset + componentSize > static_cast<std::size_t>(binary.size())) {
            break;
        }
        if (componentType == 5125) {
            std::uint32_t value = 0;
            std::memcpy(&value, binary.constData() + static_cast<int>(offset), sizeof(std::uint32_t));
            result.push_back(value);
        } else if (componentType == 5123) {
            std::uint16_t value = 0;
            std::memcpy(&value, binary.constData() + static_cast<int>(offset), sizeof(std::uint16_t));
            result.push_back(static_cast<std::uint32_t>(value));
        } else if (componentType == 5121) {
            std::uint8_t value = 0;
            std::memcpy(&value, binary.constData() + static_cast<int>(offset), sizeof(std::uint8_t));
            result.push_back(static_cast<std::uint32_t>(value));
        }
    }
    return result;
}

std::array<float, 16> readMatrix(const QJsonObject& node)
{
    std::array<float, 16> matrix = GeometryKernel::identityTransform();
    QJsonArray matrixArray = node.value("matrix").toArray();
    if (matrixArray.size() == 16) {
        for (int i = 0; i < 16; ++i) {
            matrix[i] = static_cast<float>(matrixArray[i].toDouble());
        }
    }
    return matrix;
}

Vector3 applyTransform(const std::array<float, 16>& m, const Vector3& v)
{
    // Column-major 4x4 matrix multiplication
    float x = m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12];
    float y = m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13];
    float z = m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14];
    return Vector3(x, y, z);
}

bool parseGltf(const std::filesystem::path& path, std::vector<ImportedMesh>& meshes, std::string* error)
{
    QFile jsonFile(QString::fromStdString(path.string()));
    if (!jsonFile.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = "Unable to open glTF file";
        }
        return false;
    }
    QJsonParseError parseError{};
    QJsonDocument document = QJsonDocument::fromJson(jsonFile.readAll(), &parseError);
    jsonFile.close();
    if (parseError.error != QJsonParseError::NoError || document.isNull()) {
        if (error) {
            *error = "Failed to parse glTF JSON";
        }
        return false;
    }
    QJsonObject root = document.object();
    QJsonArray buffers = root.value("buffers").toArray();
    if (buffers.isEmpty()) {
        if (error) {
            *error = "glTF file missing buffer definitions";
        }
        return false;
    }
    QString uri = buffers[0].toObject().value("uri").toString();
    QFileInfo info(QString::fromStdString(path.string()));
    QString binPath = info.dir().filePath(uri);
    QFile binFile(binPath);
    if (!binFile.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = "Unable to open glTF buffer";
        }
        return false;
    }
    QByteArray binary = binFile.readAll();
    binFile.close();

    QJsonArray bufferViews = root.value("bufferViews").toArray();
    QJsonArray accessors = root.value("accessors").toArray();
    QJsonArray meshesArray = root.value("meshes").toArray();
    QJsonArray nodes = root.value("nodes").toArray();
    QJsonArray materials = root.value("materials").toArray();

    if (nodes.isEmpty()) {
        if (error) {
            *error = "glTF file does not contain nodes";
        }
        return false;
    }

    const auto identity = GeometryKernel::identityTransform();

    for (const auto& nodeValue : nodes) {
        QJsonObject nodeObj = nodeValue.toObject();
        int meshIndex = nodeObj.value("mesh").toInt(-1);
        if (meshIndex < 0 || meshIndex >= meshesArray.size()) {
            continue;
        }
        QJsonObject meshObj = meshesArray[meshIndex].toObject();
        QJsonArray primitives = meshObj.value("primitives").toArray();
        if (primitives.isEmpty()) {
            continue;
        }
        QJsonObject primitive = primitives[0].toObject();
        QJsonObject attributes = primitive.value("attributes").toObject();
        int positionAccessor = attributes.value("POSITION").toInt(-1);
        int indexAccessor = primitive.value("indices").toInt(-1);
        if (positionAccessor < 0 || indexAccessor < 0) {
            continue;
        }
        auto positions = readVec3Accessor(bufferViews, accessors, positionAccessor, binary);
        auto indices = readIndexAccessor(bufferViews, accessors, indexAccessor, binary);
        if (positions.empty() || indices.empty()) {
            continue;
        }
        ImportedMesh mesh;
        QString defaultName = meshObj.value("name").toString(QStringLiteral("Mesh"));
        mesh.name = nodeObj.value("name").toString(defaultName).toStdString();
        mesh.positions = std::move(positions);
        mesh.indices = std::move(indices);
        mesh.transform = readMatrix(nodeObj);
        int materialIndex = primitive.value("material").toInt(-1);
        if (materialIndex >= 0 && materialIndex < materials.size()) {
            mesh.material = materials[materialIndex].toObject().value("name").toString().toStdString();
        }
        if (mesh.transform != identity) {
            for (auto& v : mesh.positions) {
                v = applyTransform(mesh.transform, v);
            }
            mesh.transform = identity;
        }
        meshes.push_back(std::move(mesh));
    }

    if (meshes.empty()) {
        if (error) {
            *error = "glTF file did not contain primitives";
        }
        return false;
    }
    return true;
}

bool buildDocument(Scene::Document& document, std::vector<ImportedMesh>& meshes, std::string* error)
{
    document.reset();
    auto& kernel = document.geometry();
    for (auto& mesh : meshes) {
        if (mesh.positions.empty() || mesh.indices.size() < 3) {
            continue;
        }
        HalfEdgeMesh halfEdge = GeometryKernel::meshFromIndexedData(mesh.positions, mesh.indices);
        auto solidPtr = Solid::createFromMesh(std::move(halfEdge));
        if (!solidPtr) {
            if (error) {
                *error = "Failed to reconstruct solid from mesh";
            }
            return false;
        }
        GeometryObject* object = kernel.addObject(std::move(solidPtr));
        if (!mesh.material.empty()) {
            kernel.assignMaterial(object, mesh.material);
        }
        document.ensureObjectForGeometry(object, mesh.name);
    }
    document.synchronizeWithGeometry();
    return true;
}

}

bool isFormatAvailable(SceneFormat format)
{
    return formatEnabled(format);
}

bool importScene(Scene::Document& document, const std::string& filePath, SceneFormat format, std::string* errorMessage)
{
    if (!isFormatAvailable(format)) {
        if (errorMessage) {
            *errorMessage = "Requested format is not available";
        }
        return false;
    }

    std::filesystem::path path(filePath);
    std::vector<ImportedMesh> meshes;

    bool parsed = false;
    switch (format) {
    case SceneFormat::OBJ:
        parsed = parseObj(path, meshes, errorMessage);
        break;
    case SceneFormat::STL:
        parsed = parseStl(path, meshes, errorMessage);
        break;
    case SceneFormat::GLTF:
        parsed = parseGltf(path, meshes, errorMessage);
        break;
    case SceneFormat::FBX:
    case SceneFormat::DAE:
        if (errorMessage) {
            *errorMessage = "Assimp importer not available";
        }
        return false;
    case SceneFormat::SKP:
        if (errorMessage) {
            *errorMessage = "SketchUp SDK integration is unavailable";
        }
        return false;
    }

    if (!parsed) {
        return false;
    }

    return buildDocument(document, meshes, errorMessage);
}

}
