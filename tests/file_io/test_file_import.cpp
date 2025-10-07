#include "Scene/Document.h"
#include "GeometryKernel/GeometryKernel.h"
#include "GeometryKernel/GeometryObject.h"
#include "GeometryKernel/HalfEdgeMesh.h"
#include "GeometryKernel/Vector3.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <cstring>
#include <cstdint>
#include <sstream>
#include <cmath>

namespace {
namespace fs = std::filesystem;

bool writeBinaryStl(const fs::path& path)
{
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }
    char header[80] = {};
    std::memcpy(header, "FreeCrafterBinarySTL", 19);
    out.write(header, sizeof(header));
    std::uint32_t triangleCount = 1;
    out.write(reinterpret_cast<const char*>(&triangleCount), sizeof(triangleCount));

    const float normal[3] = { 0.0f, 0.0f, 1.0f };
    out.write(reinterpret_cast<const char*>(normal), sizeof(normal));

    const float vertices[9] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };
    out.write(reinterpret_cast<const char*>(vertices), sizeof(vertices));

    std::uint16_t attribute = 0;
    out.write(reinterpret_cast<const char*>(&attribute), sizeof(attribute));
    return static_cast<bool>(out);
}

std::size_t appendBytes(std::vector<std::uint8_t>& buffer, const void* data, std::size_t byteCount, std::size_t alignment)
{
    if (alignment > 0) {
        std::size_t padding = (alignment - (buffer.size() % alignment)) % alignment;
        buffer.insert(buffer.end(), padding, 0);
    }
    std::size_t offset = buffer.size();
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    buffer.insert(buffer.end(), bytes, bytes + byteCount);
    return offset;
}

bool writeTransformGltf(const fs::path& gltfPath, const fs::path& binPath)
{
    const std::vector<float> positionsA = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };
    const std::vector<float> positionsB = {
        0.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        0.0f, -1.0f, 0.0f
    };
    const std::vector<std::uint16_t> indicesA = { 0, 1, 2 };
    const std::vector<std::uint16_t> indicesB = { 0, 1, 2 };

    std::vector<std::uint8_t> buffer;
    std::size_t posOffsetA = appendBytes(buffer, positionsA.data(), positionsA.size() * sizeof(float), 4);
    std::size_t idxOffsetA = appendBytes(buffer, indicesA.data(), indicesA.size() * sizeof(std::uint16_t), 2);
    std::size_t posOffsetB = appendBytes(buffer, positionsB.data(), positionsB.size() * sizeof(float), 4);
    std::size_t idxOffsetB = appendBytes(buffer, indicesB.data(), indicesB.size() * sizeof(std::uint16_t), 2);

    std::ostringstream json;
    json << R"({ "asset": { "version": "2.0" }, )";
    json << R"("buffers": [{ "uri": ")" << binPath.filename().string() << R"(", "byteLength": )" << buffer.size() << " }],";
    json << R"("bufferViews": [)";
    json << R"({ "buffer": 0, "byteOffset": )" << posOffsetA << R"(, "byteLength": )" << positionsA.size() * sizeof(float) << R"(, "target": 34962 },)";
    json << R"({ "buffer": 0, "byteOffset": )" << idxOffsetA << R"(, "byteLength": )" << indicesA.size() * sizeof(std::uint16_t) << R"(, "target": 34963 },)";
    json << R"({ "buffer": 0, "byteOffset": )" << posOffsetB << R"(, "byteLength": )" << positionsB.size() * sizeof(float) << R"(, "target": 34962 },)";
    json << R"({ "buffer": 0, "byteOffset": )" << idxOffsetB << R"(, "byteLength": )" << indicesB.size() * sizeof(std::uint16_t) << R"(, "target": 34963 })";
    json << "],";

    auto appendAccessor = [&](int bufferViewIndex, int componentType, int count, const char* type, const std::vector<float>& mins, const std::vector<float>& maxs) {
        json << R"({ "bufferView": )" << bufferViewIndex << R"(, "componentType": )" << componentType
             << R"(, "count": )" << count << R"(, "type": ")" << type << R"(")";
        if (!mins.empty()) {
            json << R"(, "min": [)";
            for (std::size_t i = 0; i < mins.size(); ++i) {
                if (i > 0)
                    json << ", ";
                json << mins[i];
            }
            json << "]";
        }
        if (!maxs.empty()) {
            json << R"(, "max": [)";
            for (std::size_t i = 0; i < maxs.size(); ++i) {
                if (i > 0)
                    json << ", ";
                json << maxs[i];
            }
            json << "]";
        }
        json << "}";
    };

    json << R"("accessors": [)";
    appendAccessor(0, 5126, 3, "VEC3", { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.0f });
    json << ",";
    appendAccessor(1, 5123, 3, "SCALAR", {}, {});
    json << ",";
    appendAccessor(2, 5126, 3, "VEC3", { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
    json << ",";
    appendAccessor(3, 5123, 3, "SCALAR", {}, {});
    json << "],";

    const float rotationZ = static_cast<float>(std::sqrt(0.5));

    json << R"("materials": [ { "name": "MatA" }, { "name": "MatB" } ],)";
    json << R"("meshes": [{ "name": "CompositeMesh", "primitives": [)";
    json << R"({ "attributes": { "POSITION": 0 }, "indices": 1, "material": 0 },)";
    json << R"({ "attributes": { "POSITION": 2 }, "indices": 3, "material": 1 }";
    json << "] }],";

    json << R"("nodes": [)";
    json << R"({ "name": "Parent", "translation": [4.0, -1.0, 2.0], "children": [1] },)";
    json << R"({ "name": "Child", "mesh": 0, "rotation": [0.0, 0.0, )" << rotationZ << ", " << rotationZ
         << R"(], "scale": [2.0, 3.0, 1.0] })";
    json << "],";
    json << R"("scenes": [{ "nodes": [0] }], "scene": 0 })";

    std::ofstream jsonOut(gltfPath, std::ios::trunc);
    if (!jsonOut) {
        return false;
    }
    jsonOut << json.str();
    jsonOut.close();

    std::ofstream binOut(binPath, std::ios::binary | std::ios::trunc);
    if (!binOut) {
        return false;
    }
    binOut.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
    return static_cast<bool>(binOut);
}

Vector3 applyTestTransform(const Vector3& v)
{
    Vector3 scaled(v.x * 2.0f, v.y * 3.0f, v.z * 1.0f);
    Vector3 rotated(-scaled.y, scaled.x, scaled.z);
    rotated.x += 4.0f;
    rotated.y += -1.0f;
    rotated.z += 2.0f;
    return rotated;
}

bool positionNear(const Vector3& a, const Vector3& b, float tolerance)
{
    Vector3 delta = a - b;
    return delta.lengthSquared() <= tolerance * tolerance;
}

std::vector<Vector3> uniquePositions(const std::vector<Vector3>& positions, float tolerance)
{
    std::vector<Vector3> unique;
    for (const auto& pos : positions) {
        bool exists = false;
        for (const auto& existing : unique) {
            if (positionNear(existing, pos, tolerance)) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            unique.push_back(pos);
        }
    }
    return unique;
}

}

int main()
{
    fs::path dataDir = fs::path(__FILE__).parent_path();
    fs::path objPath = dataDir / "cube.obj";
    fs::path stlPath = dataDir / "tetrahedron.stl";

    Scene::Document objDoc;
    if (!objDoc.importExternalModel(objPath.string(), Scene::Document::FileFormat::Auto)) {
        std::cerr << "OBJ import failed: " << objDoc.lastImportError() << '\n';
        return 1;
    }

    const auto& objects = objDoc.geometry().getObjects();
    if (objects.size() != 1) {
        std::cerr << "Expected 1 imported object, got " << objects.size() << '\n';
        return 2;
    }

    const auto& metadata = objDoc.importedObjectMetadata();
    if (metadata.size() != 1) {
        std::cerr << "Expected metadata for imported object" << '\n';
        return 3;
    }

    const auto& metaEntry = *metadata.begin();
    fs::path expectedPath = fs::weakly_canonical(objPath);
    fs::path recordedPath = fs::weakly_canonical(fs::path(metaEntry.second.sourcePath));
    if (expectedPath != recordedPath) {
        std::cerr << "Source path mismatch" << '\n';
        return 4;
    }

    if (metaEntry.second.materialSlots.size() != 2) {
        std::cerr << "Expected two material slots" << '\n';
        return 5;
    }

    if (metaEntry.second.materialSlots[0] != "MaterialOne" || metaEntry.second.materialSlots[1] != "MaterialTwo") {
        std::cerr << "Unexpected material slot names" << '\n';
        return 6;
    }

    const HalfEdgeMesh& mesh = objects.front()->getMesh();
    if (mesh.getVertices().empty()) {
        std::cerr << "Mesh has no vertices" << '\n';
        return 7;
    }

    const HalfEdgeVertex& v0 = mesh.getVertices().front();
    if (!v0.hasNormal || !v0.hasUV) {
        std::cerr << "Vertex attributes missing" << '\n';
        return 8;
    }

    Scene::Document stlDoc;
    if (!stlDoc.importExternalModel(stlPath.string(), Scene::Document::FileFormat::Auto)) {
        std::cerr << "STL import failed: " << stlDoc.lastImportError() << '\n';
        return 9;
    }

    if (stlDoc.geometry().getObjects().size() != 1) {
        std::cerr << "STL import produced unexpected object count" << '\n';
        return 10;
    }

    if (stlDoc.importedObjectMetadata().size() != 1) {
        std::cerr << "STL metadata missing" << '\n';
        return 11;
    }

    fs::path tempDir = fs::temp_directory_path() / "freecrafter_file_io_tests";
    std::error_code tempEc;
    fs::create_directories(tempDir, tempEc);

    fs::path binaryStlPath = tempDir / "binary_generated.stl";
    if (!writeBinaryStl(binaryStlPath)) {
        std::cerr << "Failed to write binary STL test file" << '\n';
        return 15;
    }

    Scene::Document binaryDoc;
    if (!binaryDoc.importExternalModel(binaryStlPath.string(), Scene::Document::FileFormat::Auto)) {
        std::cerr << "Binary STL import failed: " << binaryDoc.lastImportError() << '\n';
        return 16;
    }

    const auto& binaryObjects = binaryDoc.geometry().getObjects();
    if (binaryObjects.size() != 1) {
        std::cerr << "Binary STL import produced unexpected object count" << '\n';
        return 17;
    }
    if (binaryObjects.front()->getMesh().getTriangles().size() != 1) {
        std::cerr << "Binary STL import triangle count mismatch" << '\n';
        return 18;
    }

    fs::path gltfPath = tempDir / "hierarchy_transform.gltf";
    fs::path binPath = tempDir / "hierarchy_transform.bin";
    if (!writeTransformGltf(gltfPath, binPath)) {
        std::cerr << "Failed to write glTF transform test assets" << '\n';
        return 19;
    }

    Scene::Document gltfDoc;
    if (!gltfDoc.importExternalModel(gltfPath.string(), Scene::Document::FileFormat::Auto)) {
        std::cerr << "glTF import failed: " << gltfDoc.lastImportError() << '\n';
        return 20;
    }

    std::vector<Vector3> collectedPositions;
    for (const auto& object : gltfDoc.geometry().getObjects()) {
        if (!object) {
            continue;
        }
        const auto& vertices = object->getMesh().getVertices();
        for (const auto& vtx : vertices) {
            collectedPositions.push_back(vtx.position);
        }
    }
    if (collectedPositions.empty()) {
        std::cerr << "glTF import produced no vertex data" << '\n';
        return 21;
    }

    const float tolerance = 1e-4f;
    auto uniqueActual = uniquePositions(collectedPositions, tolerance);

    const std::vector<Vector3> baseVertices = {
        { 0.0f, 0.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
        { -1.0f, 0.0f, 0.0f },
        { 0.0f, -1.0f, 0.0f }
    };
    std::vector<Vector3> expectedPositions;
    expectedPositions.reserve(baseVertices.size());
    for (const auto& v : baseVertices) {
        expectedPositions.push_back(applyTestTransform(v));
    }
    auto uniqueExpected = uniquePositions(expectedPositions, tolerance);
    if (uniqueActual.size() != uniqueExpected.size()) {
        std::cerr << "glTF transform mismatch: unexpected unique vertex count" << '\n';
        return 22;
    }
    for (const auto& expected : uniqueExpected) {
        bool matched = false;
        for (const auto& candidate : uniqueActual) {
            if (positionNear(candidate, expected, tolerance)) {
                matched = true;
                break;
            }
        }
        if (!matched) {
            std::cerr << "glTF transform mismatch: expected vertex not found" << '\n';
            return 23;
        }
    }

    Scene::Document missingDoc;
    if (missingDoc.importExternalModel((dataDir / "does_not_exist.obj").string(), Scene::Document::FileFormat::Auto)) {
        std::cerr << "Missing file import should have failed" << '\n';
        return 12;
    }

    if (missingDoc.lastImportError().empty()) {
        std::cerr << "Missing file should report an error" << '\n';
        return 13;
    }

    if (!missingDoc.importedObjectMetadata().empty()) {
        std::cerr << "Metadata should remain empty after failed import" << '\n';
        return 14;
    }

    fs::remove(gltfPath, tempEc);
    fs::remove(binPath, tempEc);
    fs::remove(binaryStlPath, tempEc);
    fs::remove(tempDir, tempEc);

    return 0;
}
