#include <cassert>
#include <filesystem>
#include <vector>

#include "CameraController.h"
#include "Scene/Document.h"
#include "GeometryKernel/Vector3.h"

using Scene::Document;

std::vector<Vector3> makeRectangle(float width, float depth)
{
    float hw = width * 0.5f;
    float hd = depth * 0.5f;
    return {
        { -hw, 0.0f, -hd },
        { hw, 0.0f, -hd },
        { hw, 0.0f, hd },
        { -hw, 0.0f, hd }
    };
}

void testGroupingAndOutliner()
{
    Document doc;
    GeometryObject* a = doc.geometry().addCurve(makeRectangle(2.0f, 1.0f));
    GeometryObject* b = doc.geometry().addCurve(makeRectangle(1.5f, 1.5f));
    Document::ObjectId idA = doc.ensureObjectForGeometry(a, "A");
    Document::ObjectId idB = doc.ensureObjectForGeometry(b, "B");

    Document::ObjectId groupId = doc.createGroup({ idA, idB }, "Group");
    assert(groupId != 0);
    const auto* groupNode = doc.findObject(groupId);
    assert(groupNode);
    assert(groupNode->children.size() == 2);
    for (const auto& child : groupNode->children) {
        assert(child->parent == groupNode);
    }

    bool moved = doc.moveObject(idA, 0, 0);
    assert(moved);
    const auto& root = doc.objectTree();
    bool foundA = false;
    for (const auto& child : root.children) {
        if (child->id == idA) {
            foundA = true;
            assert(child->parent == &root);
        }
    }
    assert(foundA);
}

void testComponents()
{
    Document doc;
    GeometryObject* a = doc.geometry().addCurve(makeRectangle(2.0f, 2.0f));
    GeometryObject* b = doc.geometry().addCurve(makeRectangle(1.0f, 1.0f));
    Document::ObjectId idA = doc.ensureObjectForGeometry(a, "PanelA");
    Document::ObjectId idB = doc.ensureObjectForGeometry(b, "PanelB");
    Document::ObjectId groupId = doc.createGroup({ idA, idB }, "Panel");
    assert(groupId != 0);

    Document::ComponentDefinitionId defId = doc.createComponentDefinition({ groupId }, "PanelDef");
    assert(defId != 0);
    std::size_t before = doc.geometry().getObjects().size();
    Document::ObjectId instId = doc.instantiateComponent(defId, "PanelInstance");
    assert(instId != 0);
    const auto* instanceNode = doc.findObject(instId);
    assert(instanceNode);
    assert(instanceNode->kind == Document::NodeKind::ComponentInstance);
    assert(doc.geometry().getObjects().size() > before);
    assert(!instanceNode->children.empty());
    const GeometryObject* original = doc.findObject(idA)->geometry;
    const GeometryObject* instGeom = instanceNode->children.front()->geometry;
    assert(original != instGeom);

    bool unique = doc.makeComponentUnique(instId);
    assert(unique);
    const auto* updatedInstance = doc.findObject(instId);
    assert(updatedInstance->definitionId != defId);

    doc.refreshComponentInstances(updatedInstance->definitionId);
}

void testTagsAndVisibility()
{
    Document doc;
    GeometryObject* a = doc.geometry().addCurve(makeRectangle(1.0f, 1.0f));
    Document::ObjectId idA = doc.ensureObjectForGeometry(a, "Tagged");
    Scene::SceneSettings::Color color{ 1.0f, 0.0f, 0.0f, 1.0f };
    Document::TagId tagId = doc.createTag("Exterior", color);
    bool assigned = doc.assignTag(idA, tagId);
    assert(assigned);
    bool hidden = doc.setTagVisible(tagId, false);
    assert(hidden);
    assert(a->isHidden());
    doc.setTagVisible(tagId, true);
    assert(!a->isHidden());
    doc.setColorByTag(true);
    assert(doc.colorByTag());
}

void testIsolation()
{
    Document doc;
    GeometryObject* a = doc.geometry().addCurve(makeRectangle(1.0f, 1.0f));
    GeometryObject* b = doc.geometry().addCurve(makeRectangle(1.0f, 2.0f));
    Document::ObjectId idA = doc.ensureObjectForGeometry(a, "IsoA");
    Document::ObjectId idB = doc.ensureObjectForGeometry(b, "IsoB");
    Document::ObjectId groupId = doc.createGroup({ idA }, "IsoGroup");
    (void)groupId;
    doc.isolate(idA);
    assert(!a->isHidden());
    assert(b->isHidden());
    doc.clearIsolation();
    assert(!a->isHidden());
    assert(!b->isHidden());
}

void testScenes()
{
    Document doc;
    GeometryObject* a = doc.geometry().addCurve(makeRectangle(1.0f, 1.0f));
    Document::ObjectId idA = doc.ensureObjectForGeometry(a, "SceneObj");
    Scene::SceneSettings::Color color{ 0.2f, 0.3f, 0.4f, 1.0f };
    Document::TagId tagId = doc.createTag("SceneTag", color);
    doc.assignTag(idA, tagId);
    doc.setTagVisible(tagId, false);
    doc.setColorByTag(true);
    doc.settings().setSectionPlanesVisible(false);
    doc.settings().setSectionFillsVisible(false);

    CameraController camera;
    camera.setTarget(5.0f, 2.0f, -3.0f);
    camera.setYawPitch(0.5f, 0.25f);
    camera.setDistance(12.0f);
    camera.setFieldOfView(50.0f);
    camera.setProjectionMode(CameraController::ProjectionMode::Parallel);
    camera.setOrthoHeight(6.0f);

    Document::SceneId sceneId = doc.createScene("Primary", camera);
    assert(sceneId != 0);

    // mutate state
    doc.setTagVisible(tagId, true);
    doc.setColorByTag(false);
    doc.settings().setSectionPlanesVisible(true);
    doc.settings().setSectionFillsVisible(true);
    camera.setTarget(0.0f, 0.0f, 0.0f);
    camera.setYawPitch(1.0f, 0.8f);
    camera.setDistance(20.0f);
    camera.setProjectionMode(CameraController::ProjectionMode::Perspective);

    bool applied = doc.applyScene(sceneId, camera);
    assert(applied);
    assert(doc.colorByTag());
    assert(!doc.settings().sectionPlanesVisible());
    assert(!doc.settings().sectionFillsVisible());
    const auto& sceneTags = doc.tags();
    auto it = sceneTags.find(tagId);
    assert(it != sceneTags.end());
    assert(!it->second.visible);
    assert(camera.getProjectionMode() == CameraController::ProjectionMode::Parallel);
}

void testSerializationRoundTrip()
{
    Document doc;
    GeometryObject* a = doc.geometry().addCurve(makeRectangle(1.0f, 1.0f));
    Document::ObjectId idA = doc.ensureObjectForGeometry(a, "SaveA");
    Document::TagId tagId = doc.createTag("Persist", { 0.1f, 0.2f, 0.3f, 1.0f });
    doc.assignTag(idA, tagId);
    doc.setTagVisible(tagId, false);

    CameraController camera;
    Document::SceneId sceneId = doc.createScene("Snapshot", camera);
    assert(sceneId != 0);

    std::filesystem::path path = std::filesystem::temp_directory_path() / "phase5_scene.fcm";
    bool saved = doc.saveToFile(path.string());
    assert(saved);

    Document loaded;
    bool loadedOk = loaded.loadFromFile(path.string());
    std::filesystem::remove(path);
    assert(loadedOk);
    assert(loaded.tags().size() == doc.tags().size());
    assert(!loaded.tags().empty());
    assert(!loaded.tags().begin()->second.visible);
    assert(loaded.scenes().size() == doc.scenes().size());
    assert(!loaded.colorByTag());
    assert(!loaded.objectTree().children.empty());
}

int main()
{
    testGroupingAndOutliner();
    testComponents();
    testTagsAndVisibility();
    testIsolation();
    testScenes();
    testSerializationRoundTrip();
    return 0;
}

