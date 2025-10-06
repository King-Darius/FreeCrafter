#pragma once

#include "SectionPlane.h"
#include "SceneSettings.h"
#include "../CameraController.h"
#include "../GeometryKernel/GeometryKernel.h"

#include <iosfwd>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <chrono>

namespace Scene {

class Document {
public:
    enum class FileFormat {
        Auto,
        Obj,
        Stl,
        Fbx,
        Dxf,
        Dwg
    };

    enum class NodeKind {
        Root,
        Geometry,
        Group,
        ComponentInstance
    };

    using ObjectId = std::uint64_t;
    using TagId = std::uint64_t;
    using ComponentDefinitionId = std::uint64_t;
    using SceneId = std::uint64_t;

    struct ObjectNode {
        ObjectId id = 0;
        NodeKind kind = NodeKind::Geometry;
        std::string name;
        GeometryObject* geometry = nullptr;
        ComponentDefinitionId definitionId = 0;
        ObjectNode* parent = nullptr;
        std::vector<TagId> tags;
        bool visible = true;
        bool expanded = true;
        std::vector<std::unique_ptr<ObjectNode>> children;
    };

    struct Tag {
        TagId id = 0;
        std::string name;
        SceneSettings::Color color{};
        bool visible = true;
    };

    struct SceneState {
        struct CameraState {
            float yaw = 0.0f;
            float pitch = 0.0f;
            float distance = 0.0f;
            Vector3 target{ 0.0f, 0.0f, 0.0f };
            CameraController::ProjectionMode projection = CameraController::ProjectionMode::Perspective;
            float fieldOfView = 35.0f;
            float orthoHeight = 10.0f;
        };

        SceneId id = 0;
        std::string name;
        CameraState camera;
        SceneSettings settings;
        bool colorByTag = false;
        std::unordered_map<TagId, bool> tagVisibility;
    };

    Document();

    GeometryKernel& geometry() { return geometryKernel; }
    const GeometryKernel& geometry() const { return geometryKernel; }

    std::vector<SectionPlane>& sectionPlanes() { return planes; }
    const std::vector<SectionPlane>& sectionPlanes() const { return planes; }

    SceneSettings& settings() { return sceneSettings; }
    const SceneSettings& settings() const { return sceneSettings; }

    const ObjectNode& objectTree() const { return *rootNode; }
    const ObjectNode* findObject(ObjectId id) const;
    ObjectNode* findObject(ObjectId id);

    ObjectId ensureObjectForGeometry(GeometryObject* object, const std::string& name = std::string());
    void synchronizeWithGeometry();
    void pruneInvalidObjects();

    ObjectId createGroup(const std::vector<ObjectId>& childIds, const std::string& name);
    bool moveObject(ObjectId objectId, ObjectId newParentId, std::size_t index);
    bool setObjectVisible(ObjectId objectId, bool visible);

    ComponentDefinitionId createComponentDefinition(const std::vector<ObjectId>& sourceIds, const std::string& name);
    ObjectId instantiateComponent(ComponentDefinitionId definitionId, const std::string& name);
    bool makeComponentUnique(ObjectId instanceId);
    void refreshComponentInstances(ComponentDefinitionId definitionId);

    TagId createTag(const std::string& name, const SceneSettings::Color& color);
    bool renameTag(TagId id, const std::string& name);
    bool setTagColor(TagId id, const SceneSettings::Color& color);
    bool setTagVisible(TagId id, bool visible);
    bool assignTag(ObjectId objectId, TagId tagId);
    bool removeTag(ObjectId objectId, TagId tagId);
    const std::unordered_map<TagId, Tag>& tags() const { return tagMap; }
    bool colorByTag() const { return colorByTagEnabled; }
    void setColorByTag(bool enabled);

    void isolate(ObjectId objectId);
    void pushIsolation(ObjectId objectId);
    void clearIsolation();
    const std::vector<ObjectId>& isolationStack() const { return isolationIds; }

    SceneId createScene(const std::string& name, const CameraController& camera);
    bool updateScene(SceneId id, const CameraController& camera);
    bool applyScene(SceneId id, CameraController& camera);
    bool removeScene(SceneId id);
    const std::unordered_map<SceneId, SceneState>& scenes() const { return sceneMap; }

    SectionPlane& addSectionPlane(const SectionPlane& plane);
    void clearSectionPlanes();
    void reset();

    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);

    struct ImportMetadata {
        std::string sourcePath;
        FileFormat format = FileFormat::Auto;
        std::vector<std::string> materialSlots;
        std::chrono::system_clock::time_point importedAt{};
    };

    bool importExternalModel(const std::string& path, FileFormat fmt = FileFormat::Auto);
    const std::string& lastImportError() const { return lastImportErrorMessage; }
    const std::unordered_map<ObjectId, ImportMetadata>& importedObjectMetadata() const { return importedProvenance; }

private:
    struct PrototypeNode {
        NodeKind kind = NodeKind::Geometry;
        std::string name;
        GeometryObject* geometry = nullptr;
        std::vector<TagId> tags;
        std::vector<std::unique_ptr<PrototypeNode>> children;
    };

    struct ComponentDefinition {
        ComponentDefinitionId id = 0;
        std::string name;
        GeometryKernel geometry;
        std::vector<std::unique_ptr<PrototypeNode>> roots;
    };

    ObjectNode* addNode(NodeKind kind, const std::string& name, ObjectNode* parent);
    void removeNode(ObjectNode* node);
    void removeChildGeometry(ObjectNode& node);
    void removeChildNode(ObjectNode& node, ObjectNode* child);
    void forEachNode(const std::function<void(ObjectNode&)>& fn);
    void forEachNode(const std::function<void(const ObjectNode&)>& fn) const;
    void rebuildIndices();
    void updateVisibility();
    void applyVisibilityRecursive(ObjectNode& node, const std::unordered_set<TagId>& hiddenTags,
                                  const std::unordered_set<ObjectId>& isolatedIds, bool ancestorVisible);
    void collectIsolationIds(const ObjectNode& node, std::unordered_set<ObjectId>& ids) const;
    ObjectNode* findMutable(ObjectId id);
    const ObjectNode* findConst(ObjectId id) const;
    bool isDescendantOf(const ObjectNode& node, const ObjectNode& ancestor) const;
    std::unique_ptr<PrototypeNode> buildPrototypeFromNode(const ObjectNode& node, ComponentDefinition& definition);
    std::unique_ptr<ObjectNode> instantiatePrototype(const PrototypeNode& proto, ComponentDefinition& definition, ObjectNode* parent);
    void registerNode(ObjectNode* node);
    void unregisterNode(ObjectNode* node);
    void registerGeometry(ObjectNode* node, GeometryObject* geometry);
    void unregisterGeometry(GeometryObject* geometry);
    void detachFromParent(ObjectNode* node);
    std::unique_ptr<ObjectNode> detachChild(ObjectNode* node);
    void captureSceneState(const CameraController& camera, SceneState& state) const;
    void applySceneState(const SceneState& state, CameraController& camera);
    void serializeObjectTree(std::ostream& os) const;
    void serializeNode(std::ostream& os, const ObjectNode& node,
                       const std::unordered_map<const GeometryObject*, std::size_t>& geometryLookup) const;
    bool deserializeObjectTree(std::istream& is, int version, const std::vector<GeometryObject*>& geometryObjects);
    bool deserializeNode(std::istream& is, ObjectNode* parent, const std::vector<GeometryObject*>& geometryObjects, int version);
    void serializeTags(std::ostream& os) const;
    bool deserializeTags(std::istream& is);
    void serializeComponentDefinitions(std::ostream& os) const;
    bool deserializeComponentDefinitions(std::istream& is, int version);
    void serializeScenes(std::ostream& os) const;
    bool deserializeScenes(std::istream& is, int version);
    bool parsePrototype(std::istream& is, const std::vector<GeometryObject*>& geometryObjects, PrototypeNode& proto);
    void resetInternal(bool clearGeometry);

    void clearImportMetadata(ObjectId id);

    GeometryKernel geometryKernel;
    std::vector<SectionPlane> planes;
    SceneSettings sceneSettings;

    std::unique_ptr<ObjectNode> rootNode;
    ObjectId nextObjectId = 1;
    TagId nextTagId = 1;
    ComponentDefinitionId nextDefinitionId = 1;
    SceneId nextSceneId = 1;

    std::unordered_map<ObjectId, ObjectNode*> nodeIndex;
    std::unordered_map<const GeometryObject*, ObjectId> geometryIndex;
    std::unordered_map<TagId, Tag> tagMap;
    std::unordered_map<ComponentDefinitionId, ComponentDefinition> componentDefinitions;
    std::unordered_map<ComponentDefinitionId, std::vector<ObjectId>> componentInstances;
    std::unordered_map<SceneId, SceneState> sceneMap;

    bool colorByTagEnabled = false;
    std::vector<ObjectId> isolationIds;

    std::unordered_map<ObjectId, ImportMetadata> importedProvenance;
    std::string lastImportErrorMessage;
};

} // namespace Scene
