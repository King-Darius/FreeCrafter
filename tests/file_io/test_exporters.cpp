#include "FileIO/Exporters/SceneExporter.h"
#include "FileIO/Importers/SceneImporter.h"
#include "GeometryKernel/GeometryKernel.h"
#include "GeometryKernel/Solid.h"
#include "Scene/Document.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <limits>
#include <unordered_map>
#include <utility>

namespace {

void buildSampleScene(Scene::Document& document)
{
    document.reset();
    auto& kernel = document.geometry();
    std::vector<Vector3> baseA{
        { -0.5f, 0.0f, -0.5f },
        { 0.5f, 0.0f, -0.5f },
        { 0.5f, 0.0f, 0.5f },
        { -0.5f, 0.0f, 0.5f }
    };
    std::vector<Vector3> baseB{
        { 0.0f, 0.0f, -0.25f },
        { 1.0f, 0.0f, -0.25f },
        { 1.0f, 0.0f, 0.25f },
        { 0.0f, 0.0f, 0.25f }
    };

    auto solidA = Solid::createFromProfile(baseA, 1.5f);
    auto solidB = Solid::createFromProfile(baseB, 0.75f);
    if (solidB) {
        solidB->translate(Vector3(1.5f, 0.0f, 0.5f));
    }

    GeometryObject* objectA = kernel.addObject(std::move(solidA));
    GeometryObject* objectB = kernel.addObject(std::move(solidB));

    if (objectA) {
        kernel.assignMaterial(objectA, "Brick");
        document.ensureObjectForGeometry(objectA, "Block_A");
    }
    if (objectB) {
        kernel.assignMaterial(objectB, "Glass");
        document.ensureObjectForGeometry(objectB, "Block_B");
    }
    document.synchronizeWithGeometry();
}

struct SceneStats {
    std::size_t triangleCount = 0;
    std::unordered_map<std::string, std::size_t> materialTriangles;
    Vector3 minBounds{ std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
    Vector3 maxBounds{ std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };
    bool hasGeometry = false;
};

SceneStats summarizeScene(const Scene::Document& document)
{
    SceneStats stats;
    const auto& objects = document.geometry().getObjects();
    for (const auto& obj : objects) {
        if (!obj || obj->getType() != ObjectType::Solid) {
            continue;
        }
        auto buffer = document.geometry().buildMeshBuffer(*obj);
        if (buffer.indices.empty()) {
            continue;
        }
        std::size_t triangles = buffer.indices.size() / 3;
        stats.triangleCount += triangles;
        std::string material = document.geometry().getMaterial(obj.get());
        stats.materialTriangles[material] += triangles;
        for (const auto& v : buffer.positions) {
            stats.minBounds.x = std::min(stats.minBounds.x, v.x);
            stats.minBounds.y = std::min(stats.minBounds.y, v.y);
            stats.minBounds.z = std::min(stats.minBounds.z, v.z);
            stats.maxBounds.x = std::max(stats.maxBounds.x, v.x);
            stats.maxBounds.y = std::max(stats.maxBounds.y, v.y);
            stats.maxBounds.z = std::max(stats.maxBounds.z, v.z);
        }
        stats.hasGeometry = true;
    }
    return stats;
}

void verifyRoundTrip(FileIO::SceneFormat format, const std::filesystem::path& basePath)
{
    Scene::Document source;
    buildSampleScene(source);
    SceneStats expected = summarizeScene(source);
    assert(expected.hasGeometry);

    std::filesystem::create_directories(basePath.parent_path());
    std::string error;
    bool exported = FileIO::Exporters::exportScene(source, basePath.string(), format, &error);
    assert(exported);
    assert(error.empty());

    Scene::Document imported;
    std::string importError;
    bool importedOk = FileIO::Importers::importScene(imported, basePath.string(), format, &importError);
    assert(importedOk);
    assert(importError.empty());

    SceneStats actual = summarizeScene(imported);
    assert(actual.hasGeometry);
    assert(actual.triangleCount == expected.triangleCount);
    assert(actual.materialTriangles == expected.materialTriangles);
    const float tolerance = 1e-3f;
    auto within = [&](float a, float b) {
        return std::abs(a - b) <= tolerance;
    };
    assert(within(actual.minBounds.x, expected.minBounds.x));
    assert(within(actual.minBounds.y, expected.minBounds.y));
    assert(within(actual.minBounds.z, expected.minBounds.z));
    assert(within(actual.maxBounds.x, expected.maxBounds.x));
    assert(within(actual.maxBounds.y, expected.maxBounds.y));
    assert(within(actual.maxBounds.z, expected.maxBounds.z));
}

}

int main()
{
    const auto tempRoot = std::filesystem::temp_directory_path() / "freecrafter_file_io_tests";
    verifyRoundTrip(FileIO::SceneFormat::OBJ, tempRoot / "scene.obj");
    verifyRoundTrip(FileIO::SceneFormat::STL, tempRoot / "scene.stl");
    verifyRoundTrip(FileIO::SceneFormat::GLTF, tempRoot / "scene.gltf");
    return 0;
}
