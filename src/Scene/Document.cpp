#include "Document.h"

#include "SceneSerializer.h"
#include "SectionPlane.h"
#include "SceneSettings.h"
#include "../CameraController.h"
#include "../FileIO/Importers/FileImporter.h"
#include "../GeometryKernel/Curve.h"
#include "../GeometryKernel/Solid.h"
#include "../GeometryKernel/HalfEdgeMesh.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>
#include <queue>
#include <sstream>
#include <utility>
#include <QString>
#include <QFileInfo>
#include <QObject>

namespace {
constexpr const char* kBeginGeometry = "BEGIN_GEOMETRY";
constexpr const char* kEndGeometry = "END_GEOMETRY";
constexpr const char* kBeginSectionPlanes = "BEGIN_SECTION_PLANES";
constexpr const char* kEndSectionPlanes = "END_SECTION_PLANES";
constexpr const char* kBeginSettings = "BEGIN_SETTINGS";
constexpr const char* kEndSettings = "END_SETTINGS";
constexpr const char* kBeginObjectTree = "BEGIN_OBJECT_TREE";
constexpr const char* kEndObjectTree = "END_OBJECT_TREE";
constexpr const char* kBeginTags = "BEGIN_TAGS";
constexpr const char* kEndTags = "END_TAGS";
constexpr const char* kBeginComponentDefs = "BEGIN_COMPONENT_DEFS";
constexpr const char* kEndComponentDefs = "END_COMPONENT_DEFS";
constexpr const char* kBeginScenes = "BEGIN_SCENES";
constexpr const char* kEndScenes = "END_SCENES";
constexpr const char* kColorByTagToken = "COLOR_BY_TAG";

template <typename Container, typename Value>
bool contains(const Container& container, const Value& value)
{
    return std::find(container.begin(), container.end(), value) != container.end();
}

Vector3 centroidFromGeometry(const GeometryObject& object)
{
    const auto& vertices = object.getMesh().getVertices();
    if (vertices.empty())
        return Vector3();
    Vector3 sum(0.0f, 0.0f, 0.0f);
    for (const auto& vertex : vertices)
        sum += vertex.position;
    const float inv = 1.0f / static_cast<float>(vertices.size());
    return sum * inv;
}

void translateGeometry(GeometryObject& object, const Vector3& delta)
{
    if (object.getType() == ObjectType::Curve)
        static_cast<Curve&>(object).translate(delta);
    else
        static_cast<Solid&>(object).translate(delta);
}

} // namespace

namespace Scene {

Document::Document()
{
    rootNode = std::make_unique<ObjectNode>();
    rootNode->id = 0;
    rootNode->kind = NodeKind::Root;
    rootNode->name = "Root";
    rootNode->visible = true;
    registerNode(rootNode.get());
}

const Document::ObjectNode* Document::findObject(ObjectId id) const
{
    return findConst(id);
}

Document::ObjectNode* Document::findObject(ObjectId id)
{
    return findMutable(id);
}

Document::ObjectId Document::objectIdForGeometry(const GeometryObject* object) const
{
    if (!object)
        return 0;
    GeometryObject::StableId geometryId = object->getStableId();
    if (geometryId == 0)
        return 0;
    auto it = geometryIndex.find(geometryId);
    if (it == geometryIndex.end())
        return 0;
    return it->second;
}

GeometryObject* Document::geometryForObject(ObjectId id)
{
    ObjectNode* node = findMutable(id);
    if (!node)
        return nullptr;
    return node->geometry;
}

const GeometryObject* Document::geometryForObject(ObjectId id) const
{
    const ObjectNode* node = findConst(id);
    if (!node)
        return nullptr;
    return node->geometry;
}

Document::Transform Document::objectTransform(ObjectId id) const
{
    Transform transform;
    const GeometryObject* object = geometryForObject(id);
    if (object)
        transform.position = centroidFromGeometry(*object);
    transform.rotation = Vector3(0.0f, 0.0f, 0.0f);
    transform.scale = Vector3(1.0f, 1.0f, 1.0f);
    return transform;
}

bool Document::applyTransform(ObjectId id, const Transform& transform, const TransformMask& mask)
{
    GeometryObject* object = geometryForObject(id);
    if (!object)
        return false;

    bool changed = false;
    if (mask.position[0] || mask.position[1] || mask.position[2]) {
        Transform current = objectTransform(id);
        Vector3 target = current.position;
        if (mask.position[0])
            target.x = transform.position.x;
        if (mask.position[1])
            target.y = transform.position.y;
        if (mask.position[2])
            target.z = transform.position.z;
        Vector3 delta = target - current.position;
        if (delta.lengthSquared() > 1e-10f) {
            translateGeometry(*object, delta);
            changed = true;
        }
    }

    // Rotation and scale editing are not yet supported.
    return changed;
}

Document::ObjectId Document::ensureObjectForGeometry(GeometryObject* object, const std::string& name)
{
    if (!object)
        return 0;
    GeometryObject::StableId geometryId = object->getStableId();
    if (geometryId != 0) {
        auto it = geometryIndex.find(geometryId);
        if (it != geometryIndex.end())
            return it->second;
    }
    std::string nodeName = name.empty() ? std::string("Object ") + std::to_string(nextObjectId) : name;
    ObjectNode* node = addNode(NodeKind::Geometry, nodeName, rootNode.get());
    node->geometry = object;
    registerGeometry(node, object);
    updateVisibility();
    return node->id;
}

void Document::synchronizeWithGeometry()
{
    std::unordered_set<GeometryObject::StableId> existing;
    existing.reserve(geometryKernel.getObjects().size());
    for (const auto& uptr : geometryKernel.getObjects()) {
        if (!uptr)
            continue;
        GeometryObject::StableId id = uptr->getStableId();
        if (id != 0)
            existing.insert(id);
    }

    std::vector<ObjectId> toRemove;
    for (const auto& [geometryId, objectId] : geometryIndex) {
        if (!existing.count(geometryId)) {
            toRemove.push_back(objectId);
        }
    }
    for (ObjectId id : toRemove) {
        if (ObjectNode* node = findMutable(id)) {
            removeNode(node);
        }
    }

    for (const auto& uptr : geometryKernel.getObjects()) {
        ensureObjectForGeometry(uptr.get());
    }
}

void Document::pruneInvalidObjects()
{
    synchronizeWithGeometry();
}

bool Document::removeObject(ObjectId objectId)
{
    ObjectNode* node = findMutable(objectId);
    if (!node)
        return false;
    removeNode(node);
    return true;
}

bool Document::renameObject(ObjectId objectId, const std::string& name)
{
    ObjectNode* node = findMutable(objectId);
    if (!node)
        return false;
    node->name = name;
    return true;
}

bool Document::setObjectExpanded(ObjectId objectId, bool expanded)
{
    ObjectNode* node = findMutable(objectId);
    if (!node)
        return false;
    node->expanded = expanded;
    return true;
}

Document::ObjectId Document::createGroup(const std::vector<ObjectId>& childIds, const std::string& name)
{
    if (childIds.empty())
        return 0;

    std::vector<ObjectNode*> children;
    children.reserve(childIds.size());
    for (ObjectId id : childIds) {
        ObjectNode* node = findMutable(id);
        if (!node || node == rootNode.get())
            return 0;
        children.push_back(node);
    }

    ObjectNode* firstParent = children.front()->parent ? children.front()->parent : rootNode.get();
    for (ObjectNode* child : children) {
        if (!child->parent)
            continue;
        if (child->parent != firstParent) {
            firstParent = rootNode.get();
            break;
        }
    }

    ObjectNode* group = addNode(NodeKind::Group, name.empty() ? std::string("Group ") + std::to_string(nextObjectId) : name, firstParent);

    for (ObjectNode* child : children) {
        std::unique_ptr<ObjectNode> owned = detachChild(child);
        if (!owned)
            continue;
        owned->parent = group;
        group->children.push_back(std::move(owned));
    }

    updateVisibility();
    return group->id;
}

bool Document::moveObject(ObjectId objectId, ObjectId newParentId, std::size_t index)
{
    ObjectNode* node = findMutable(objectId);
    ObjectNode* newParent = findMutable(newParentId);
    if (!node || !newParent || node == rootNode.get())
        return false;
    if (node == newParent)
        return false;
    if (newParent->kind == NodeKind::Geometry)
        return false;
    if (isDescendantOf(*newParent, *node))
        return false;

    std::unique_ptr<ObjectNode> owned = detachChild(node);
    if (!owned)
        return false;
    owned->parent = newParent;
    auto& children = newParent->children;
    index = std::min<std::size_t>(index, children.size());
    children.insert(children.begin() + static_cast<std::ptrdiff_t>(index), std::move(owned));
    updateVisibility();
    return true;
}

bool Document::setObjectVisible(ObjectId objectId, bool visible)
{
    ObjectNode* node = findMutable(objectId);
    if (!node)
        return false;
    node->visible = visible;
    updateVisibility();
    return true;
}

Document::ComponentDefinitionId Document::createComponentDefinition(const std::vector<ObjectId>& sourceIds, const std::string& name)
{
    if (sourceIds.empty())
        return 0;

    ComponentDefinition definition;
    definition.id = nextDefinitionId++;
    definition.name = name;

    for (ObjectId id : sourceIds) {
        const ObjectNode* node = findConst(id);
        if (!node)
            continue;
        definition.roots.push_back(buildPrototypeFromNode(*node, definition));
    }

    if (definition.roots.empty())
        return 0;

    componentDefinitions.emplace(definition.id, std::move(definition));
    return definition.id;
}

Document::ObjectId Document::instantiateComponent(ComponentDefinitionId definitionId, const std::string& name)
{
    auto it = componentDefinitions.find(definitionId);
    if (it == componentDefinitions.end())
        return 0;

    ComponentDefinition& definition = it->second;
    ObjectNode* instance = addNode(NodeKind::ComponentInstance,
                                   name.empty() ? (!definition.name.empty() ? definition.name : std::string("Component ") + std::to_string(nextObjectId)) : name,
                                   rootNode.get());
    instance->definitionId = definitionId;
    registerNode(instance);

    for (const auto& proto : definition.roots) {
        instance->children.push_back(instantiatePrototype(*proto, definition, instance));
    }
    updateVisibility();
    return instance->id;
}

bool Document::makeComponentUnique(ObjectId instanceId)
{
    ObjectNode* instance = findMutable(instanceId);
    if (!instance || instance->kind != NodeKind::ComponentInstance)
        return false;
    auto defIt = componentDefinitions.find(instance->definitionId);
    if (defIt == componentDefinitions.end())
        return false;

    ComponentDefinition newDefinition;
    newDefinition.id = nextDefinitionId++;
    newDefinition.name = instance->name.empty() ? defIt->second.name : instance->name;

    for (const auto& child : instance->children) {
        newDefinition.roots.push_back(buildPrototypeFromNode(*child, newDefinition));
    }

    ComponentDefinitionId newId = newDefinition.id;
    componentDefinitions.emplace(newId, std::move(newDefinition));

    auto& oldInstances = componentInstances[instance->definitionId];
    oldInstances.erase(std::remove(oldInstances.begin(), oldInstances.end(), instanceId), oldInstances.end());
    if (oldInstances.empty()) {
        componentInstances.erase(instance->definitionId);
    }

    instance->definitionId = newId;
    registerNode(instance);
    return true;
}

void Document::refreshComponentInstances(ComponentDefinitionId definitionId)
{
    auto defIt = componentDefinitions.find(definitionId);
    if (defIt == componentDefinitions.end())
        return;

    auto instIt = componentInstances.find(definitionId);
    if (instIt == componentInstances.end())
        return;

    for (ObjectId id : instIt->second) {
        ObjectNode* instance = findMutable(id);
        if (!instance)
            continue;
        removeChildGeometry(*instance);
        instance->children.clear();
        for (const auto& proto : defIt->second.roots) {
            instance->children.push_back(instantiatePrototype(*proto, defIt->second, instance));
        }
    }
    updateVisibility();
}

Document::TagId Document::createTag(const std::string& name, const SceneSettings::Color& color)
{
    Tag tag;
    tag.id = nextTagId++;
    tag.name = name;
    tag.color = color;
    tag.visible = true;
    tagMap.emplace(tag.id, tag);
    return tag.id;
}

bool Document::renameTag(TagId id, const std::string& name)
{
    auto it = tagMap.find(id);
    if (it == tagMap.end())
        return false;
    it->second.name = name;
    return true;
}

bool Document::setTagColor(TagId id, const SceneSettings::Color& color)
{
    auto it = tagMap.find(id);
    if (it == tagMap.end())
        return false;
    it->second.color = color;
    return true;
}

bool Document::setTagVisible(TagId id, bool visible)
{
    auto it = tagMap.find(id);
    if (it == tagMap.end())
        return false;
    it->second.visible = visible;
    updateVisibility();
    return true;
}

bool Document::assignTag(ObjectId objectId, TagId tagId)
{
    ObjectNode* node = findMutable(objectId);
    if (!node || tagMap.find(tagId) == tagMap.end())
        return false;
    if (!contains(node->tags, tagId)) {
        node->tags.push_back(tagId);
    }
    updateVisibility();
    return true;
}

bool Document::removeTag(ObjectId objectId, TagId tagId)
{
    ObjectNode* node = findMutable(objectId);
    if (!node)
        return false;
    auto it = std::remove(node->tags.begin(), node->tags.end(), tagId);
    if (it == node->tags.end())
        return false;
    node->tags.erase(it, node->tags.end());
    updateVisibility();
    return true;
}

bool Document::deleteTag(TagId id, std::vector<ObjectId>* affectedObjects)
{
    auto it = tagMap.find(id);
    if (it == tagMap.end())
        return false;
    tagMap.erase(it);

    forEachNode([&](ObjectNode& node) {
        if (node.id == 0)
            return;
        auto tagIt = std::remove(node.tags.begin(), node.tags.end(), id);
        if (tagIt != node.tags.end()) {
            if (affectedObjects)
                affectedObjects->push_back(node.id);
            node.tags.erase(tagIt, node.tags.end());
        }
    });
    updateVisibility();
    return true;
}

bool Document::restoreTag(const Tag& tag, const std::vector<ObjectId>& assignments)
{
    if (tag.id == 0)
        return false;
    tagMap[tag.id] = tag;
    nextTagId = std::max(nextTagId, tag.id + 1);

    for (ObjectId objectId : assignments) {
        ObjectNode* node = findMutable(objectId);
        if (!node)
            continue;
        if (!contains(node->tags, tag.id))
            node->tags.push_back(tag.id);
    }
    updateVisibility();
    return true;
}

void Document::setColorByTag(bool enabled)
{
    colorByTagEnabled = enabled;
}

void Document::isolate(ObjectId objectId)
{
    isolationIds.clear();
    if (findConst(objectId)) {
        isolationIds.push_back(objectId);
    }
    updateVisibility();
}

void Document::pushIsolation(ObjectId objectId)
{
    if (!findConst(objectId))
        return;
    isolationIds.push_back(objectId);
    updateVisibility();
}

void Document::clearIsolation()
{
    if (isolationIds.empty())
        return;
    isolationIds.clear();
    updateVisibility();
}

Document::SceneId Document::createScene(const std::string& name, const CameraController& camera)
{
    SceneState state;
    state.id = nextSceneId++;
    state.name = name;
    state.settings = sceneSettings;
    state.colorByTag = colorByTagEnabled;
    for (const auto& [id, tag] : tagMap) {
        state.tagVisibility[id] = tag.visible;
    }
    captureSceneState(camera, state);
    sceneMap.emplace(state.id, state);
    return state.id;
}

bool Document::updateScene(SceneId id, const CameraController& camera)
{
    auto it = sceneMap.find(id);
    if (it == sceneMap.end())
        return false;
    it->second.settings = sceneSettings;
    it->second.colorByTag = colorByTagEnabled;
    it->second.tagVisibility.clear();
    for (const auto& [tagId, tag] : tagMap) {
        it->second.tagVisibility[tagId] = tag.visible;
    }
    captureSceneState(camera, it->second);
    return true;
}

bool Document::applyScene(SceneId id, CameraController& camera)
{
    auto it = sceneMap.find(id);
    if (it == sceneMap.end())
        return false;
    const SceneState& state = it->second;
    sceneSettings = state.settings;
    colorByTagEnabled = state.colorByTag;
    for (auto& [tagId, tag] : tagMap) {
        auto visIt = state.tagVisibility.find(tagId);
        if (visIt != state.tagVisibility.end()) {
            tag.visible = visIt->second;
        }
    }
    applySceneState(state, camera);
    updateVisibility();
    return true;
}

bool Document::removeScene(SceneId id)
{
    return sceneMap.erase(id) > 0;
}

SectionPlane& Document::addSectionPlane(const SectionPlane& plane)
{
    planes.push_back(plane);
    return planes.back();
}

void Document::clearSectionPlanes()
{
    planes.clear();
}

void Document::reset()
{
    resetInternal(true);
}

void Document::resetInternal(bool clearGeometry)
{
    if (clearGeometry)
        geometryKernel.clear();
    planes.clear();
    sceneSettings.reset();
    rootNode = std::make_unique<ObjectNode>();
    rootNode->id = 0;
    rootNode->kind = NodeKind::Root;
    rootNode->name = "Root";
    nodeIndex.clear();
    geometryIndex.clear();
    componentDefinitions.clear();
    componentInstances.clear();
    sceneMap.clear();
    tagMap.clear();
    colorByTagEnabled = false;
    isolationIds.clear();
    importedProvenance.clear();
    imagePlaneMetadata.clear();
    externalReferenceMetadata.clear();
    lastImportErrorMessage.clear();
    lastSceneIoErrorMessage.clear();
    nextObjectId = 1;
    nextTagId = 1;
    nextDefinitionId = 1;
    nextSceneId = 1;
    registerNode(rootNode.get());
}

bool Document::saveToFile(const std::string& filename) const
{
    SceneSerializer::Result result = SceneSerializer::save(*this, filename);
    if (result.status == SceneSerializer::Result::Status::Success) {
        lastSceneIoErrorMessage.clear();
        return true;
    }
    if (result.status == SceneSerializer::Result::Status::Error) {
        lastSceneIoErrorMessage = result.message;
    } else {
        lastSceneIoErrorMessage = "Scene serializer reported an unknown failure";
    }
    return false;
}

bool Document::loadFromFile(const std::string& filename)
{
    SceneSerializer::Result result = SceneSerializer::load(*this, filename);
    if (result.status == SceneSerializer::Result::Status::Success) {
        lastSceneIoErrorMessage.clear();
        return true;
    }
    if (result.status == SceneSerializer::Result::Status::FormatMismatch) {
        if (loadLegacyFromFile(filename)) {
            lastSceneIoErrorMessage.clear();
            return true;
        }
        if (lastSceneIoErrorMessage.empty()) {
            lastSceneIoErrorMessage = "Legacy scene file could not be parsed";
        }
        return false;
    }
    lastSceneIoErrorMessage = result.message;
    return false;
}

bool Document::loadLegacyFromFile(const std::string& filename)
{
    std::ifstream is(filename);
    if (!is) {
        lastSceneIoErrorMessage = "Unable to open legacy scene file";
        return false;
    }

    std::string tag;
    int version = 0;
    if (!(is >> tag >> version)) {
        lastSceneIoErrorMessage = "Legacy scene header is invalid";
        return false;
    }
    if (tag != "FCM") {
        lastSceneIoErrorMessage = "Legacy scene signature mismatch";
        return false;
    }

    if (version <= 1) {
        if (!geometryKernel.loadFromFile(filename)) {
            lastSceneIoErrorMessage = "Legacy kernel payload could not be read";
            return false;
        }
        resetInternal(false);
        synchronizeWithGeometry();
        updateVisibility();
        lastSceneIoErrorMessage.clear();
        return true;
    }

    reset();

    std::string token;
    while (is >> token) {
        if (token == kBeginGeometry) {
            geometryKernel.loadFromStream(is, kEndGeometry);
        } else if (token == kBeginSectionPlanes) {
            size_t count = 0;
            is >> count;
            planes.clear();
            planes.reserve(count);
            for (size_t i = 0; i < count; ++i) {
                SectionPlane plane;
                if (!plane.deserialize(is)) {
                    lastSceneIoErrorMessage = "Legacy section plane payload truncated";
                    return false;
                }
                planes.push_back(plane);
            }
        } else if (token == kBeginSettings) {
            if (!sceneSettings.deserialize(is, version)) {
                lastSceneIoErrorMessage = "Legacy scene settings failed to deserialize";
                return false;
            }
        } else if (token == kBeginTags) {
            if (!deserializeTags(is)) {
                lastSceneIoErrorMessage = "Legacy tags section failed to deserialize";
                return false;
            }
        } else if (token == kBeginObjectTree) {
            std::vector<GeometryObject*> geometryObjects;
            geometryObjects.reserve(geometryKernel.getObjects().size());
            for (const auto& uptr : geometryKernel.getObjects()) {
                geometryObjects.push_back(uptr.get());
            }
            if (!deserializeObjectTree(is, version, geometryObjects)) {
                lastSceneIoErrorMessage = "Legacy object tree failed to deserialize";
                return false;
            }
        } else if (token == kBeginComponentDefs) {
            if (!deserializeComponentDefinitions(is, version)) {
                lastSceneIoErrorMessage = "Legacy component definitions failed to deserialize";
                return false;
            }
        } else if (token == kBeginScenes) {
            if (!deserializeScenes(is, version)) {
                lastSceneIoErrorMessage = "Legacy scenes failed to deserialize";
                return false;
            }
        } else if (token == kColorByTagToken) {
            int enabled = 0;
            is >> enabled;
            colorByTagEnabled = enabled != 0;
        }
    }

    synchronizeWithGeometry();
    updateVisibility();
    lastSceneIoErrorMessage.clear();
    return true;
}

bool Document::importExternalModel(const std::string& path, FileFormat fmt)
{
    lastImportErrorMessage.clear();
    QString qPath = QString::fromStdString(path);
    auto importResult = FileIO::Importers::FileImporter::import(qPath, *this, fmt);
    if (!importResult.success) {
        lastImportErrorMessage = importResult.errorMessage.toStdString();
        return false;
    }

    if (importResult.objects.empty()) {
        lastImportErrorMessage = QObject::tr("No importable geometry found").toStdString();
        return false;
    }

    std::string absolutePath = QFileInfo(qPath).absoluteFilePath().toStdString();
    auto timestamp = std::chrono::system_clock::now();
    for (const auto& summary : importResult.objects) {
        if (summary.objectId == 0)
            continue;
        ImportMetadata metadata;
        metadata.sourcePath = absolutePath;
        metadata.format = importResult.resolvedFormat;
        metadata.materialSlots = summary.materialSlots;
        metadata.importedAt = timestamp;
        importedProvenance[summary.objectId] = std::move(metadata);
    }

    updateVisibility();
    return true;
}

void Document::setImagePlaneMetadata(ObjectId id, const ImagePlaneMetadata& metadata)
{
    if (id == 0)
        return;
    imagePlaneMetadata[id] = metadata;
}

void Document::clearImagePlaneMetadata(ObjectId id)
{
    if (id == 0)
        return;
    imagePlaneMetadata.erase(id);
}

void Document::setExternalReferenceMetadata(ObjectId id, const ExternalReferenceMetadata& metadata)
{
    if (id == 0)
        return;
    externalReferenceMetadata[id] = metadata;
}

void Document::clearExternalReferenceMetadata(ObjectId id)
{
    if (id == 0)
        return;
    externalReferenceMetadata.erase(id);
}

void Document::removeNode(ObjectNode* node)
{
    if (!node || node == rootNode.get())
        return;
    while (!node->children.empty()) {
        removeNode(node->children.back().get());
    }
    if (node->geometry) {
        GeometryObject* geom = node->geometry;
        unregisterGeometry(geom);
        geometryKernel.deleteObject(geom);
        node->geometry = nullptr;
    }
    unregisterNode(node);
    detachFromParent(node);
}

void Document::removeChildGeometry(ObjectNode& node)
{
    while (!node.children.empty()) {
        removeNode(node.children.back().get());
    }
}

void Document::forEachNode(const std::function<void(ObjectNode&)>& fn)
{
    std::queue<ObjectNode*> queue;
    queue.push(rootNode.get());
    while (!queue.empty()) {
        ObjectNode* node = queue.front();
        queue.pop();
        fn(*node);
        for (auto& child : node->children) {
            queue.push(child.get());
        }
    }
}

void Document::forEachNode(const std::function<void(const ObjectNode&)>& fn) const
{
    std::queue<const ObjectNode*> queue;
    queue.push(rootNode.get());
    while (!queue.empty()) {
        const ObjectNode* node = queue.front();
        queue.pop();
        fn(*node);
        for (const auto& child : node->children) {
            queue.push(child.get());
        }
    }
}

void Document::rebuildIndices()
{
    nodeIndex.clear();
    geometryIndex.clear();
    componentInstances.clear();
    forEachNode([&](ObjectNode& node) {
        if (node.id == 0)
            return;
        registerNode(&node);
        if (node.geometry) {
            registerGeometry(&node, node.geometry);
        }
    });
}

void Document::updateVisibility()
{
    std::unordered_set<TagId> hiddenTags;
    for (const auto& [id, tag] : tagMap) {
        if (!tag.visible)
            hiddenTags.insert(id);
    }

    std::unordered_set<ObjectId> isolationSet;
    if (!isolationIds.empty()) {
        for (ObjectId id : isolationIds) {
            const ObjectNode* node = findConst(id);
            if (!node)
                continue;
            const ObjectNode* current = node;
            while (current) {
                isolationSet.insert(current->id);
                current = current->parent;
            }
            collectIsolationIds(*node, isolationSet);
        }
    }

    applyVisibilityRecursive(*rootNode, hiddenTags, isolationSet, true);
}

void Document::applyVisibilityRecursive(ObjectNode& node, const std::unordered_set<TagId>& hiddenTags,
                                        const std::unordered_set<ObjectId>& isolatedIds, bool ancestorVisible)
{
    bool nodeVisible = ancestorVisible && node.visible;
    bool isolationActive = !isolatedIds.empty();
    bool nodeAllowed = !isolationActive || isolatedIds.count(node.id) > 0 || node.kind == NodeKind::Root;

    if (node.kind == NodeKind::Geometry && node.geometry) {
        bool hidden = !(nodeVisible && nodeAllowed);
        if (!hidden) {
            for (TagId tagId : node.tags) {
                if (hiddenTags.count(tagId)) {
                    hidden = true;
                    break;
                }
            }
        }
        node.geometry->setHidden(hidden);
        node.geometry->setVisible(!hidden);
    }

    bool passToChildren = nodeVisible && nodeAllowed;
    for (auto& child : node.children) {
        applyVisibilityRecursive(*child, hiddenTags, isolatedIds, passToChildren);
    }
}

void Document::collectIsolationIds(const ObjectNode& node, std::unordered_set<ObjectId>& ids) const
{
    ids.insert(node.id);
    for (const auto& child : node.children) {
        collectIsolationIds(*child, ids);
    }
}

Document::ObjectNode* Document::findMutable(ObjectId id)
{
    auto it = nodeIndex.find(id);
    if (it == nodeIndex.end())
        return nullptr;
    return it->second;
}

const Document::ObjectNode* Document::findConst(ObjectId id) const
{
    auto it = nodeIndex.find(id);
    if (it == nodeIndex.end())
        return nullptr;
    return it->second;
}

bool Document::isDescendantOf(const ObjectNode& node, const ObjectNode& ancestor) const
{
    const ObjectNode* current = node.parent;
    while (current) {
        if (current == &ancestor)
            return true;
        current = current->parent;
    }
    return false;
}

std::unique_ptr<Document::PrototypeNode> Document::buildPrototypeFromNode(const ObjectNode& node, ComponentDefinition& definition)
{
    auto prototype = std::make_unique<PrototypeNode>();
    prototype->kind = (node.kind == NodeKind::ComponentInstance) ? NodeKind::Group : node.kind;
    prototype->name = node.name;
    prototype->tags = node.tags;
    if (node.kind == NodeKind::Geometry && node.geometry) {
        std::unique_ptr<GeometryObject> clone = node.geometry->clone();
        prototype->geometry = definition.geometry.addObject(std::move(clone));
    }
    for (const auto& child : node.children) {
        prototype->children.push_back(buildPrototypeFromNode(*child, definition));
    }
    return prototype;
}

std::unique_ptr<Document::ObjectNode> Document::instantiatePrototype(const PrototypeNode& proto, ComponentDefinition& definition, ObjectNode* parent)
{
    auto node = std::make_unique<ObjectNode>();
    node->id = nextObjectId++;
    node->kind = proto.kind;
    node->name = proto.name;
    node->parent = parent;
    node->tags = proto.tags;
    node->visible = true;
    node->expanded = true;

    if (proto.kind == NodeKind::Geometry && proto.geometry) {
        std::unique_ptr<GeometryObject> clone = proto.geometry->clone();
        GeometryObject* added = geometryKernel.addObject(std::move(clone));
        node->geometry = added;
        registerGeometry(node.get(), added);
    }

    for (const auto& child : proto.children) {
        node->children.push_back(instantiatePrototype(*child, definition, node.get()));
    }

    registerNode(node.get());
    return node;
}

void Document::registerNode(ObjectNode* node)
{
    if (!node)
        return;
    nodeIndex[node->id] = node;
    if (node->kind == NodeKind::ComponentInstance && node->definitionId != 0) {
        auto& instances = componentInstances[node->definitionId];
        if (!contains(instances, node->id)) {
            instances.push_back(node->id);
        }
    }
}

void Document::unregisterNode(ObjectNode* node)
{
    if (!node)
        return;
    nodeIndex.erase(node->id);
    if (node->kind == NodeKind::Geometry) {
        clearImportMetadata(node->id);
    }
    if (node->kind == NodeKind::ComponentInstance) {
        auto it = componentInstances.find(node->definitionId);
        if (it != componentInstances.end()) {
            auto& instances = it->second;
            instances.erase(std::remove(instances.begin(), instances.end(), node->id), instances.end());
            if (instances.empty()) {
                componentInstances.erase(it);
            }
        }
    }
}

void Document::registerGeometry(ObjectNode* node, GeometryObject* geometry)
{
    if (!node || !geometry)
        return;
    GeometryObject::StableId geometryId = geometry->getStableId();
    if (geometryId == 0)
        return;
    geometryIndex[geometryId] = node->id;
}

void Document::unregisterGeometry(GeometryObject* geometry)
{
    if (!geometry)
        return;
    GeometryObject::StableId geometryId = geometry->getStableId();
    if (geometryId == 0)
        return;
    auto it = geometryIndex.find(geometryId);
    if (it != geometryIndex.end()) {
        clearImportMetadata(it->second);
        geometryIndex.erase(it);
    }
}

void Document::clearImportMetadata(ObjectId id)
{
    importedProvenance.erase(id);
    imagePlaneMetadata.erase(id);
    externalReferenceMetadata.erase(id);
}

void Document::detachFromParent(ObjectNode* node)
{
    if (!node || !node->parent)
        return;
    auto& children = node->parent->children;
    for (auto it = children.begin(); it != children.end(); ++it) {
        if (it->get() == node) {
            children.erase(it);
            break;
        }
    }
}

std::unique_ptr<Document::ObjectNode> Document::detachChild(ObjectNode* node)
{
    if (!node || !node->parent)
        return nullptr;
    auto& children = node->parent->children;
    for (auto it = children.begin(); it != children.end(); ++it) {
        if (it->get() == node) {
            std::unique_ptr<ObjectNode> owned = std::move(*it);
            children.erase(it);
            owned->parent = nullptr;
            return owned;
        }
    }
    return nullptr;
}

void Document::captureSceneState(const CameraController& camera, SceneState& state) const
{
    state.camera.yaw = camera.getYaw();
    state.camera.pitch = camera.getPitch();
    state.camera.distance = camera.getDistance();
    float tx = 0.0f, ty = 0.0f, tz = 0.0f;
    camera.getTarget(tx, ty, tz);
    state.camera.target = Vector3(tx, ty, tz);
    state.camera.projection = camera.getProjectionMode();
    state.camera.fieldOfView = camera.getFieldOfView();
    state.camera.orthoHeight = camera.getOrthoHeight();
}

void Document::applySceneState(const SceneState& state, CameraController& camera)
{
    camera.setProjectionMode(state.camera.projection);
    camera.setTarget(state.camera.target.x, state.camera.target.y, state.camera.target.z);
    camera.setYawPitch(state.camera.yaw, state.camera.pitch);
    camera.setDistance(state.camera.distance);
    camera.setFieldOfView(state.camera.fieldOfView);
    camera.setOrthoHeight(state.camera.orthoHeight);
}

Document::ObjectNode* Document::addNode(NodeKind kind, const std::string& name, ObjectNode* parent)
{
    if (!parent)
        parent = rootNode.get();
    auto node = std::make_unique<ObjectNode>();
    node->id = nextObjectId++;
    node->kind = kind;
    node->name = name;
    node->parent = parent;
    node->visible = true;
    node->expanded = true;
    ObjectNode* raw = node.get();
    parent->children.push_back(std::move(node));
    registerNode(raw);
    return raw;
}

void Document::serializeObjectTree(std::ostream& os) const
{
    std::unordered_map<GeometryObject::StableId, std::size_t> geometryLookup;
    const auto& objects = geometryKernel.getObjects();
    for (std::size_t i = 0; i < objects.size(); ++i) {
        if (!objects[i])
            continue;
        GeometryObject::StableId id = objects[i]->getStableId();
        if (id != 0)
            geometryLookup[id] = i;
    }

    os << kBeginObjectTree << '\n';
    serializeNode(os, *rootNode, geometryLookup);
    os << kEndObjectTree << '\n';
}

void Document::serializeNode(std::ostream& os, const ObjectNode& node,
                             const std::unordered_map<GeometryObject::StableId, std::size_t>& geometryLookup) const
{
    if (&node != rootNode.get()) {
        std::size_t geometryIndexValue = std::numeric_limits<std::size_t>::max();
        if (node.geometry) {
            GeometryObject::StableId id = node.geometry->getStableId();
            auto it = geometryLookup.find(id);
            if (it != geometryLookup.end()) {
                geometryIndexValue = it->second;
            }
        }
        os << "NODE " << node.id << ' ' << static_cast<int>(node.kind) << ' ' << std::quoted(node.name)
           << ' ' << geometryIndexValue << ' ' << node.definitionId << ' ' << (node.visible ? 1 : 0)
           << ' ' << (node.expanded ? 1 : 0) << ' ' << node.tags.size();
        for (TagId tagId : node.tags) {
            os << ' ' << tagId;
        }
        os << ' ' << node.children.size() << '\n';
    }

    for (const auto& child : node.children) {
        serializeNode(os, *child, geometryLookup);
    }
}

bool Document::deserializeObjectTree(std::istream& is, int version, const std::vector<GeometryObject*>& geometryObjects)
{
    (void)version;
    rootNode->children.clear();
    nodeIndex.clear();
    geometryIndex.clear();
    registerNode(rootNode.get());

    std::string token;
    while (is >> token) {
        if (token == kEndObjectTree)
            break;
        if (token == "NODE") {
            if (!deserializeNode(is, rootNode.get(), geometryObjects, version))
                return false;
        }
    }
    return true;
}

bool Document::deserializeNode(std::istream& is, ObjectNode* parent, const std::vector<GeometryObject*>& geometryObjects, int version)
{
    ObjectId id = 0;
    int kindValue = 0;
    std::string name;
    std::size_t geometryIndexValue = std::numeric_limits<std::size_t>::max();
    ComponentDefinitionId definitionId = 0;
    int visible = 1;
    int expanded = 1;
    std::size_t tagCount = 0;
    std::size_t childCount = 0;

    if (!(is >> id >> kindValue >> std::quoted(name) >> geometryIndexValue >> definitionId >> visible >> expanded >> tagCount))
        return false;

    auto node = std::make_unique<ObjectNode>();
    node->id = id;
    node->kind = static_cast<NodeKind>(kindValue);
    node->name = name;
    node->parent = parent;
    node->visible = visible != 0;
    node->expanded = expanded != 0;

    node->tags.reserve(tagCount);
    for (std::size_t i = 0; i < tagCount; ++i) {
        TagId tagId = 0;
        is >> tagId;
        node->tags.push_back(tagId);
    }
    if (!(is >> childCount))
        return false;

    if (geometryIndexValue < geometryObjects.size()) {
        node->geometry = geometryObjects[geometryIndexValue];
        registerGeometry(node.get(), node->geometry);
    }
    if (node->kind == NodeKind::ComponentInstance) {
        node->definitionId = definitionId;
    }

    ObjectNode* raw = node.get();
    parent->children.push_back(std::move(node));
    registerNode(raw);
    nextObjectId = std::max(nextObjectId, raw->id + 1);

    for (std::size_t i = 0; i < childCount; ++i) {
        std::string childToken;
        if (!(is >> childToken) || childToken != "NODE")
            return false;
        if (!deserializeNode(is, raw, geometryObjects, version))
            return false;
    }
    return true;
}

void Document::serializeTags(std::ostream& os) const
{
    os << kBeginTags << ' ' << tagMap.size() << '\n';
    for (const auto& [id, tag] : tagMap) {
        os << "TAG " << id << ' ' << std::quoted(tag.name) << ' '
           << tag.color.r << ' ' << tag.color.g << ' ' << tag.color.b << ' ' << tag.color.a
           << ' ' << (tag.visible ? 1 : 0) << '\n';
    }
    os << kEndTags << '\n';
}

bool Document::deserializeTags(std::istream& is)
{
    size_t count = 0;
    if (!(is >> count))
        return false;
    tagMap.clear();
    for (size_t i = 0; i < count; ++i) {
        std::string token;
        if (!(is >> token) || token != "TAG")
            return false;
        Tag tag;
        if (!(is >> tag.id >> std::quoted(tag.name) >> tag.color.r >> tag.color.g >> tag.color.b >> tag.color.a))
            return false;
        int visible = 1;
        is >> visible;
        tag.visible = visible != 0;
        tagMap.emplace(tag.id, tag);
        nextTagId = std::max(nextTagId, tag.id + 1);
    }
    std::string endToken;
    if (!(is >> endToken) || endToken != kEndTags)
        return false;
    return true;
}

void Document::serializeComponentDefinitions(std::ostream& os) const
{
    os << kBeginComponentDefs << ' ' << componentDefinitions.size() << '\n';
    for (const auto& [id, definition] : componentDefinitions) {
        os << "DEF " << id << ' ' << std::quoted(definition.name) << '\n';
        os << "DEF_GEOMETRY\n";
        definition.geometry.saveToStream(os);
        os << "END_DEF_GEOMETRY\n";

        std::unordered_map<GeometryObject::StableId, std::size_t> lookup;
        const auto& objects = definition.geometry.getObjects();
        for (std::size_t i = 0; i < objects.size(); ++i) {
            if (!objects[i])
                continue;
            GeometryObject::StableId geometryId = objects[i]->getStableId();
            if (geometryId != 0)
                lookup[geometryId] = i;
        }

        std::function<void(const PrototypeNode&)> serializeProto = [&](const PrototypeNode& proto) {
            os << "PROTO " << static_cast<int>(proto.kind) << ' ' << std::quoted(proto.name) << ' ';
            std::size_t geometryIndexValue = std::numeric_limits<std::size_t>::max();
            if (proto.geometry) {
                GeometryObject::StableId geometryId = proto.geometry->getStableId();
                auto it = lookup.find(geometryId);
                if (it != lookup.end()) {
                    geometryIndexValue = it->second;
                }
            }
            os << geometryIndexValue << ' ' << proto.tags.size();
            for (TagId tagId : proto.tags) {
                os << ' ' << tagId;
            }
            os << ' ' << proto.children.size() << '\n';
            for (const auto& child : proto.children) {
                serializeProto(*child);
            }
            os << "END_PROTO\n";
        };

        os << "BEGIN_PROTOTYPES\n";
        for (const auto& proto : definition.roots) {
            serializeProto(*proto);
        }
        os << "END_PROTOTYPES\n";
    }
    os << kEndComponentDefs << '\n';
}

bool Document::deserializeComponentDefinitions(std::istream& is, int version)
{
    (void)version;
    size_t count = 0;
    if (!(is >> count))
        return false;
    componentDefinitions.clear();
    componentInstances.clear();
    for (size_t i = 0; i < count; ++i) {
        std::string token;
        if (!(is >> token) || token != "DEF")
            return false;
        ComponentDefinition definition;
        if (!(is >> definition.id >> std::quoted(definition.name)))
            return false;
        nextDefinitionId = std::max(nextDefinitionId, definition.id + 1);

        std::string geometryToken;
        if (!(is >> geometryToken) || geometryToken != "DEF_GEOMETRY")
            return false;
        definition.geometry.loadFromStream(is, "END_DEF_GEOMETRY");

        std::vector<GeometryObject*> geometryObjects;
        const auto& objects = definition.geometry.getObjects();
        geometryObjects.reserve(objects.size());
        for (const auto& uptr : objects) {
            geometryObjects.push_back(uptr.get());
        }

        std::string protoToken;
        if (!(is >> protoToken) || protoToken != "BEGIN_PROTOTYPES")
            return false;

        while (true) {
            if (!(is >> protoToken))
                return false;
            if (protoToken == "END_PROTOTYPES")
                break;
            if (protoToken != "PROTO")
                return false;
            auto proto = std::make_unique<PrototypeNode>();
            if (!parsePrototype(is, geometryObjects, *proto))
                return false;
            std::string endToken;
            if (!(is >> endToken) || endToken != "END_PROTO")
                return false;
            definition.roots.push_back(std::move(proto));
        }

        componentDefinitions.emplace(definition.id, std::move(definition));
    }
    std::string endToken;
    if (!(is >> endToken) || endToken != kEndComponentDefs)
        return false;
    return true;
}

void Document::serializeScenes(std::ostream& os) const
{
    os << kBeginScenes << ' ' << sceneMap.size() << '\n';
    for (const auto& [id, scene] : sceneMap) {
        const auto& palette = scene.settings.palette();
        os << "SCENE " << id << ' ' << std::quoted(scene.name) << ' '
           << scene.camera.yaw << ' ' << scene.camera.pitch << ' ' << scene.camera.distance << ' '
           << scene.camera.target.x << ' ' << scene.camera.target.y << ' ' << scene.camera.target.z << ' '
           << static_cast<int>(scene.camera.projection) << ' ' << scene.camera.fieldOfView << ' ' << scene.camera.orthoHeight << ' '
           << (scene.colorByTag ? 1 : 0) << ' ' << (scene.settings.sectionPlanesVisible() ? 1 : 0) << ' '
           << (scene.settings.sectionFillsVisible() ? 1 : 0) << ' ' << std::quoted(palette.id) << ' '
           << palette.fill.r << ' ' << palette.fill.g << ' ' << palette.fill.b << ' ' << palette.fill.a << ' '
           << palette.edge.r << ' ' << palette.edge.g << ' ' << palette.edge.b << ' ' << palette.edge.a << ' '
           << palette.highlight.r << ' ' << palette.highlight.g << ' ' << palette.highlight.b << ' ' << palette.highlight.a << ' '
           << scene.tagVisibility.size() << '\n';
        for (const auto& [tagId, visible] : scene.tagVisibility) {
            os << "SCENE_TAG " << tagId << ' ' << (visible ? 1 : 0) << '\n';
        }
        os << "END_SCENE\n";
    }
    os << kEndScenes << '\n';
}

bool Document::deserializeScenes(std::istream& is, int version)
{
    (void)version;
    size_t count = 0;
    if (!(is >> count))
        return false;
    sceneMap.clear();
    for (size_t i = 0; i < count; ++i) {
        std::string token;
        if (!(is >> token) || token != "SCENE")
            return false;
        SceneState scene;
        int projection = 0;
        int colorBy = 0;
        int planesVisible = 1;
        int fillsVisible = 1;
        std::string paletteId;
        std::size_t tagCount = 0;
        SceneSettings::PaletteState palette;
        if (!(is >> scene.id >> std::quoted(scene.name) >> scene.camera.yaw >> scene.camera.pitch >> scene.camera.distance
              >> scene.camera.target.x >> scene.camera.target.y >> scene.camera.target.z >> projection
              >> scene.camera.fieldOfView >> scene.camera.orthoHeight >> colorBy >> planesVisible >> fillsVisible
              >> std::quoted(paletteId)
              >> palette.fill.r >> palette.fill.g >> palette.fill.b >> palette.fill.a
              >> palette.edge.r >> palette.edge.g >> palette.edge.b >> palette.edge.a
              >> palette.highlight.r >> palette.highlight.g >> palette.highlight.b >> palette.highlight.a
              >> tagCount))
            return false;

        palette.id = paletteId;
        scene.settings.reset();
        scene.settings.setPalette(palette);
        scene.settings.setSectionPlanesVisible(planesVisible != 0);
        scene.settings.setSectionFillsVisible(fillsVisible != 0);
        scene.colorByTag = colorBy != 0;
        scene.camera.projection = static_cast<CameraController::ProjectionMode>(projection);

        for (std::size_t j = 0; j < tagCount; ++j) {
            std::string tagToken;
            if (!(is >> tagToken) || tagToken != "SCENE_TAG")
                return false;
            TagId tagId = 0;
            int visible = 1;
            if (!(is >> tagId >> visible))
                return false;
            scene.tagVisibility[tagId] = visible != 0;
        }

        std::string endScene;
        if (!(is >> endScene) || endScene != "END_SCENE")
            return false;

        nextSceneId = std::max(nextSceneId, scene.id + 1);
        sceneMap.emplace(scene.id, scene);
    }

    std::string endToken;
    if (!(is >> endToken) || endToken != kEndScenes)
        return false;
    return true;
}

bool Document::parsePrototype(std::istream& is, const std::vector<GeometryObject*>& geometryObjects, PrototypeNode& proto)
{
    int kindValue = 0;
    std::size_t geometryIndexValue = std::numeric_limits<std::size_t>::max();
    std::size_t tagCount = 0;
    std::size_t childCount = 0;
    if (!(is >> kindValue >> std::quoted(proto.name) >> geometryIndexValue >> tagCount))
        return false;
    proto.kind = static_cast<NodeKind>(kindValue);
    proto.tags.clear();
    proto.tags.reserve(tagCount);
    for (std::size_t i = 0; i < tagCount; ++i) {
        TagId tagId = 0;
        is >> tagId;
        proto.tags.push_back(tagId);
    }
    if (!(is >> childCount))
        return false;
    proto.geometry = geometryIndexValue < geometryObjects.size() ? geometryObjects[geometryIndexValue] : nullptr;
    proto.children.clear();
    proto.children.reserve(childCount);
    for (std::size_t i = 0; i < childCount; ++i) {
        std::string token;
        if (!(is >> token) || token != "PROTO")
            return false;
        auto child = std::make_unique<PrototypeNode>();
        if (!parsePrototype(is, geometryObjects, *child))
            return false;
        std::string endToken;
        if (!(is >> endToken) || endToken != "END_PROTO")
            return false;
        proto.children.push_back(std::move(child));
    }
    return true;
}

} // namespace Scene

