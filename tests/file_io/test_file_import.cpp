#include "Scene/Document.h"
#include "GeometryKernel/GeometryKernel.h"
#include "GeometryKernel/GeometryObject.h"
#include "GeometryKernel/HalfEdgeMesh.h"
#include <filesystem>
#include <iostream>

int main()
{
    namespace fs = std::filesystem;

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

    return 0;
}
