#include "SceneExporter.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include "GeometryKernel/GeometryKernel.h"
#include "Scene/Document.h"

namespace FileIO::Exporters {
namespace {

struct SceneInstance {
    std::string name;
    GeometryKernel::MeshBuffer mesh;
    std::string material;
    std::array<float, 16> transform;
    bool visible = true;
};

SceneInstance makeInstance(const Scene::Document::ObjectNode& node, const GeometryKernel& kernel, std::size_t indexFallback)
{
    SceneInstance instance;
    instance.name = node.name.empty() ? (std::string("Object_") + std::to_string(indexFallback)) : node.name;
    instance.material = kernel.getMaterial(node.geometry);
    instance.mesh = kernel.buildMeshBuffer(*node.geometry);
    instance.transform = GeometryKernel::identityTransform();
    instance.visible = node.visible;
    return instance;
}

void collectFromNode(const Scene::Document::ObjectNode& node,
                     const GeometryKernel& kernel,
                     std::vector<SceneInstance>& out,
                     std::unordered_set<const GeometryObject*>& visited,
                     std::size_t& counter,
                     bool ancestorsVisible)
{
    bool nodeVisible = ancestorsVisible && node.visible;
    if (node.kind == Scene::Document::NodeKind::Geometry && node.geometry && nodeVisible) {
        if (visited.insert(node.geometry).second) {
            out.push_back(makeInstance(node, kernel, ++counter));
        }
    }
    for (const auto& child : node.children) {
        collectFromNode(*child, kernel, out, visited, counter, nodeVisible);
    }
}

std::vector<SceneInstance> gatherSceneInstances(const Scene::Document& document)
{
    std::vector<SceneInstance> instances;
    std::unordered_set<const GeometryObject*> visited;
    std::size_t counter = 0;
    const auto& root = document.objectTree();
    collectFromNode(root, document.geometry(), instances, visited, counter, true);

    for (const auto& object : document.geometry().getObjects()) {
        if (!object) {
            continue;
        }
        if (visited.count(object.get())) {
            continue;
        }
        if (object->getType() != ObjectType::Solid) {
            continue;
        }
        Scene::Document::ObjectNode placeholder;
        placeholder.geometry = object.get();
        placeholder.name = std::string("LooseObject_") + std::to_string(++counter);
        placeholder.visible = true;
        instances.push_back(makeInstance(placeholder, document.geometry(), counter));
    }

    return instances;
}

bool ensureParentDirectory(const std::filesystem::path& filePath, std::string* error)
{
    const auto parent = filePath.parent_path();
    if (parent.empty()) {
        return true;
    }
    std::error_code ec;
    std::filesystem::create_directories(parent, ec);
    if (ec) {
        if (error) {
            *error = std::string("Failed to create output directory: ") + ec.message();
        }
        return false;
    }
    return true;
}

bool writeObj(const std::filesystem::path& output,
              const std::vector<SceneInstance>& instances,
              std::string* error)
{
    if (!ensureParentDirectory(output, error)) {
        return false;
    }
    std::ofstream obj(output, std::ios::out | std::ios::trunc);
    if (!obj) {
        if (error) {
            *error = "Unable to open OBJ file for writing";
        }
        return false;
    }

    std::unordered_set<std::string> materialNames;
    for (const auto& instance : instances) {
        if (!instance.material.empty()) {
            materialNames.insert(instance.material);
        }
    }

    std::filesystem::path mtlPath = output;
    mtlPath.replace_extension(".mtl");

    if (!materialNames.empty()) {
        obj << "mtllib " << mtlPath.filename().string() << "\n";
    }

    std::size_t vertexOffset = 1;
    std::vector<Vector3> normals;
    for (const auto& instance : instances) {
        if (instance.mesh.indices.empty() || instance.mesh.positions.empty()) {
            continue;
        }
        obj << "o " << instance.name << "\n";
        if (!instance.material.empty()) {
            obj << "usemtl " << instance.material << "\n";
        }
        for (const auto& v : instance.mesh.positions) {
            obj << "v " << v.x << ' ' << v.y << ' ' << v.z << "\n";
        }
        for (const auto& n : instance.mesh.normals) {
            obj << "vn " << n.x << ' ' << n.y << ' ' << n.z << "\n";
        }
        const auto& indices = instance.mesh.indices;
        for (std::size_t i = 0; i + 2 < indices.size(); i += 3) {
            const auto i0 = indices[i] + vertexOffset;
            const auto i1 = indices[i + 1] + vertexOffset;
            const auto i2 = indices[i + 2] + vertexOffset;
            obj << "f " << i0 << "//" << i0 << ' '
                << i1 << "//" << i1 << ' '
                << i2 << "//" << i2 << "\n";
        }
        vertexOffset += instance.mesh.positions.size();
    }

    if (!materialNames.empty()) {
        std::ofstream mtl(mtlPath, std::ios::out | std::ios::trunc);
        if (!mtl) {
            if (error) {
                *error = "Unable to write material library";
            }
            return false;
        }
        for (const auto& material : materialNames) {
            mtl << "newmtl " << material << "\n";
            mtl << "Kd 0.8 0.8 0.8\n";
            mtl << "Ks 0.0 0.0 0.0\n";
            mtl << "d 1.0\n\n";
        }
    }

    return true;
}

bool writeStl(const std::filesystem::path& output,
              const std::vector<SceneInstance>& instances,
              std::string* error)
{
    if (!ensureParentDirectory(output, error)) {
        return false;
    }
    std::ofstream stl(output, std::ios::out | std::ios::trunc);
    if (!stl) {
        if (error) {
            *error = "Unable to open STL file for writing";
        }
        return false;
    }
    stl << "solid FreeCrafter\n";
    QJsonArray metadata;
    std::size_t autoNameCounter = 0;
    for (const auto& instance : instances) {
        const auto& mesh = instance.mesh;
        for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            const auto& a = mesh.positions[mesh.indices[i]];
            const auto& b = mesh.positions[mesh.indices[i + 1]];
            const auto& c = mesh.positions[mesh.indices[i + 2]];
            Vector3 normal = (b - a).cross(c - a).normalized();
            stl << "  facet normal " << normal.x << ' ' << normal.y << ' ' << normal.z << "\n";
            stl << "    outer loop\n";
            stl << "      vertex " << a.x << ' ' << a.y << ' ' << a.z << "\n";
            stl << "      vertex " << b.x << ' ' << b.y << ' ' << b.z << "\n";
            stl << "      vertex " << c.x << ' ' << c.y << ' ' << c.z << "\n";
            stl << "    endloop\n";
            stl << "  endfacet\n";
        }
        if (!mesh.indices.empty()) {
            QJsonObject entry;
            std::string safeName = instance.name;
            if (safeName.empty()) {
                safeName = std::string("Instance_") + std::to_string(++autoNameCounter);
            }
            entry.insert("name", QString::fromStdString(safeName));
            if (!instance.material.empty()) {
                entry.insert("material", QString::fromStdString(instance.material));
            }
            entry.insert("triangles", static_cast<int>(mesh.indices.size() / 3));
            metadata.append(entry);
        }
    }
    stl << "endsolid FreeCrafter\n";
    stl.close();

    if (!metadata.isEmpty()) {
        QJsonObject root;
        root.insert("version", 1);
        root.insert("instances", metadata);
        QFile metaFile(QString::fromStdString(output.string()) + QStringLiteral(".meta.json"));
        if (!metaFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            if (error) {
                *error = "Failed to write STL metadata";
            }
            std::error_code removeError;
            std::filesystem::remove(output, removeError);
            return false;
        }
        QJsonDocument doc(root);
        metaFile.write(doc.toJson(QJsonDocument::Compact));
        metaFile.close();
    } else {
        QFile::remove(QString::fromStdString(output.string()) + QStringLiteral(".meta.json"));
    }
    return true;
}

void appendAligned(QByteArray& data, const void* bytes, std::size_t length)
{
    while (data.size() % 4 != 0) {
        data.append('\0');
    }
    data.append(static_cast<const char*>(bytes), static_cast<int>(length));
}

QJsonArray toMatrixArray(const std::array<float, 16>& matrix)
{
    QJsonArray arr;
    for (float v : matrix) {
        arr.append(v);
    }
    return arr;
}

bool writeGltf(const std::filesystem::path& output,
               const std::vector<SceneInstance>& instances,
               std::string* error)
{
    if (!ensureParentDirectory(output, error)) {
        return false;
    }

    QByteArray binary;
    QJsonArray bufferViews;
    QJsonArray accessors;
    QJsonArray meshes;
    QJsonArray nodes;
    QJsonArray materialsJson;
    QJsonArray sceneNodes;
    std::unordered_map<std::string, int> materialLookup;

    int materialCounter = 0;
    for (const auto& instance : instances) {
        if (instance.mesh.indices.empty()) {
            continue;
        }
        if (!instance.material.empty() && !materialLookup.count(instance.material)) {
            QJsonObject mat;
            mat.insert("name", QString::fromStdString(instance.material));
            QJsonObject pbr;
            QJsonArray baseColor;
            baseColor.append(0.8);
            baseColor.append(0.8);
            baseColor.append(0.8);
            baseColor.append(1.0);
            pbr.insert("baseColorFactor", baseColor);
            pbr.insert("metallicFactor", 0.0);
            pbr.insert("roughnessFactor", 0.9);
            mat.insert("pbrMetallicRoughness", pbr);
            materialsJson.append(mat);
            materialLookup[instance.material] = materialCounter++;
        }
    }

    int bufferViewCounter = 0;
    int accessorCounter = 0;
    int meshCounter = 0;

    for (const auto& instance : instances) {
        if (instance.mesh.indices.empty() || instance.mesh.positions.empty()) {
            continue;
        }

        const auto& mesh = instance.mesh;
        std::size_t positionOffset = static_cast<std::size_t>(binary.size());
        for (const auto& v : mesh.positions) {
            appendAligned(binary, &v.x, sizeof(float));
            appendAligned(binary, &v.y, sizeof(float));
            appendAligned(binary, &v.z, sizeof(float));
        }
        std::size_t positionByteLength = static_cast<std::size_t>(binary.size()) - positionOffset;

        std::size_t normalOffset = static_cast<std::size_t>(binary.size());
        for (const auto& n : mesh.normals) {
            appendAligned(binary, &n.x, sizeof(float));
            appendAligned(binary, &n.y, sizeof(float));
            appendAligned(binary, &n.z, sizeof(float));
        }
        std::size_t normalByteLength = static_cast<std::size_t>(binary.size()) - normalOffset;

        std::size_t indexOffset = static_cast<std::size_t>(binary.size());
        for (std::uint32_t idx : mesh.indices) {
            appendAligned(binary, &idx, sizeof(std::uint32_t));
        }
        std::size_t indexByteLength = static_cast<std::size_t>(binary.size()) - indexOffset;

        QJsonObject positionView;
        positionView.insert("buffer", 0);
        positionView.insert("byteOffset", static_cast<int>(positionOffset));
        positionView.insert("byteLength", static_cast<int>(positionByteLength));
        positionView.insert("target", 34962);
        bufferViews.append(positionView);
        int positionViewIndex = bufferViewCounter++;

        QJsonObject normalView;
        normalView.insert("buffer", 0);
        normalView.insert("byteOffset", static_cast<int>(normalOffset));
        normalView.insert("byteLength", static_cast<int>(normalByteLength));
        normalView.insert("target", 34962);
        bufferViews.append(normalView);
        int normalViewIndex = bufferViewCounter++;

        QJsonObject indexView;
        indexView.insert("buffer", 0);
        indexView.insert("byteOffset", static_cast<int>(indexOffset));
        indexView.insert("byteLength", static_cast<int>(indexByteLength));
        indexView.insert("target", 34963);
        bufferViews.append(indexView);
        int indexViewIndex = bufferViewCounter++;

        Vector3 minPos = mesh.positions.front();
        Vector3 maxPos = mesh.positions.front();
        for (const auto& v : mesh.positions) {
            minPos.x = std::min(minPos.x, v.x);
            minPos.y = std::min(minPos.y, v.y);
            minPos.z = std::min(minPos.z, v.z);
            maxPos.x = std::max(maxPos.x, v.x);
            maxPos.y = std::max(maxPos.y, v.y);
            maxPos.z = std::max(maxPos.z, v.z);
        }

        QJsonArray minArray;
        minArray.append(minPos.x);
        minArray.append(minPos.y);
        minArray.append(minPos.z);
        QJsonArray maxArray;
        maxArray.append(maxPos.x);
        maxArray.append(maxPos.y);
        maxArray.append(maxPos.z);

        QJsonObject positionAccessor;
        positionAccessor.insert("bufferView", positionViewIndex);
        positionAccessor.insert("componentType", 5126);
        positionAccessor.insert("count", static_cast<int>(mesh.positions.size()));
        positionAccessor.insert("type", "VEC3");
        positionAccessor.insert("min", minArray);
        positionAccessor.insert("max", maxArray);
        accessors.append(positionAccessor);
        int positionAccessorIndex = accessorCounter++;

        QJsonObject normalAccessor;
        normalAccessor.insert("bufferView", normalViewIndex);
        normalAccessor.insert("componentType", 5126);
        normalAccessor.insert("count", static_cast<int>(mesh.normals.size()));
        normalAccessor.insert("type", "VEC3");
        accessors.append(normalAccessor);
        int normalAccessorIndex = accessorCounter++;

        QJsonObject indexAccessor;
        indexAccessor.insert("bufferView", indexViewIndex);
        indexAccessor.insert("componentType", 5125);
        indexAccessor.insert("count", static_cast<int>(mesh.indices.size()));
        indexAccessor.insert("type", "SCALAR");
        accessors.append(indexAccessor);
        int indexAccessorIndex = accessorCounter++;

        QJsonObject attributes;
        attributes.insert("POSITION", positionAccessorIndex);
        attributes.insert("NORMAL", normalAccessorIndex);

        QJsonObject primitive;
        primitive.insert("attributes", attributes);
        primitive.insert("indices", indexAccessorIndex);
        if (!instance.material.empty() && materialLookup.count(instance.material)) {
            primitive.insert("material", materialLookup[instance.material]);
        }

        QJsonArray primitives;
        primitives.append(primitive);

        QJsonObject meshJson;
        meshJson.insert("name", QString::fromStdString(instance.name));
        meshJson.insert("primitives", primitives);
        meshes.append(meshJson);
        int meshIndex = meshCounter++;

        QJsonObject node;
        node.insert("name", QString::fromStdString(instance.name));
        node.insert("mesh", meshIndex);
        node.insert("matrix", toMatrixArray(instance.transform));
        nodes.append(node);
        sceneNodes.append(nodes.size() - 1);
    }

    QJsonObject buffer;
    buffer.insert("byteLength", static_cast<int>(binary.size()));
    buffer.insert("uri", QFileInfo(QString::fromStdString(output.string())).completeBaseName() + ".bin");
    QJsonArray buffers;
    buffers.append(buffer);

    QJsonObject scene;
    scene.insert("nodes", sceneNodes);
    QJsonArray scenes;
    scenes.append(scene);

    QJsonObject root;
    QJsonObject asset;
    asset.insert("version", "2.0");
    asset.insert("generator", "FreeCrafter SceneExporter");
    root.insert("asset", asset);
    root.insert("buffers", buffers);
    root.insert("bufferViews", bufferViews);
    root.insert("accessors", accessors);
    if (!materialsJson.isEmpty()) {
        root.insert("materials", materialsJson);
    }
    root.insert("meshes", meshes);
    root.insert("nodes", nodes);
    root.insert("scenes", scenes);
    root.insert("scene", 0);

    QFile jsonFile(QString::fromStdString(output.string()));
    if (!jsonFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = "Failed to open glTF file for writing";
        }
        return false;
    }
    QJsonDocument doc(root);
    jsonFile.write(doc.toJson(QJsonDocument::Indented));
    jsonFile.close();

    QFileInfo outputInfo(QString::fromStdString(output.string()));
    QString binPath = outputInfo.dir().filePath(outputInfo.completeBaseName() + ".bin");
    QFile binFile(binPath);
    if (!binFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = "Failed to write glTF buffer";
        }
        return false;
    }
    binFile.write(binary);
    binFile.close();
    return true;
}

}

bool isFormatAvailable(SceneFormat format)
{
    return isSceneFormatAvailable(format);
}

std::vector<SceneFormat> supportedFormats()
{
    return availableSceneFormats();
}

std::vector<std::string> supportedFormatFilters()
{
    std::vector<std::string> filters;
    for (auto format : supportedFormats()) {
        filters.push_back(formatFilterString(format));
    }
    return filters;
}

std::optional<SceneFormat> guessFormatFromFilename(const std::string& filename)
{
    auto ext = std::filesystem::path(filename).extension().string();
    return sceneFormatFromExtension(ext);
}

bool exportScene(const Scene::Document& document, const std::string& filePath, SceneFormat format, std::string* errorMessage)
{
    if (!isFormatAvailable(format)) {
        if (errorMessage) {
            *errorMessage = "Requested format is not available in this build";
        }
        return false;
    }

    auto instances = gatherSceneInstances(document);
    if (instances.empty()) {
        if (errorMessage) {
            *errorMessage = "Scene contains no exportable geometry";
        }
        return false;
    }

    std::filesystem::path outputPath(filePath);

    switch (format) {
    case SceneFormat::OBJ:
        return writeObj(outputPath, instances, errorMessage);
    case SceneFormat::STL:
        return writeStl(outputPath, instances, errorMessage);
    case SceneFormat::GLTF:
        return writeGltf(outputPath, instances, errorMessage);
    case SceneFormat::FBX:
    case SceneFormat::DAE:
        if (errorMessage) {
            *errorMessage = "Assimp-backed exporters are not linked in this configuration";
        }
        return false;
    case SceneFormat::SKP:
        if (errorMessage) {
            *errorMessage = "SketchUp SDK integration is not available; use glTF or DAE exports";
        }
        return false;
    }
    if (errorMessage) {
        *errorMessage = "Unsupported export format";
    }
    return false;
}

}
