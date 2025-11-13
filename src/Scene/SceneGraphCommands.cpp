#include "SceneGraphCommands.h"

#include "GeometryKernel/GeometryKernel.h"

#include <QObject>
#include <algorithm>
#include <optional>

namespace Scene {

RenameObjectCommand::RenameObjectCommand(Document::ObjectId id, const QString& name)
    : Core::Command(QObject::tr("Rename Object"))
    , objectId(id)
    , newName(name)
{
}

void RenameObjectCommand::initialize()
{
    if (captured || !document())
        return;
    if (const Document::ObjectNode* node = document()->findObject(objectId)) {
        previousName = node->name;
        captured = true;
    }
}

void RenameObjectCommand::performRedo()
{
    if (!document())
        return;
    document()->renameObject(objectId, newName.toStdString());
}

void RenameObjectCommand::performUndo()
{
    if (!document())
        return;
    document()->renameObject(objectId, previousName);
}

RenameObjectsCommand::RenameObjectsCommand(const std::vector<Document::ObjectId>& ids, const QString& name)
    : Core::Command(QObject::tr("Rename Selection"))
    , objectIds(ids)
    , newName(name)
{
    objectIds.erase(std::remove(objectIds.begin(), objectIds.end(), Document::ObjectId(0)), objectIds.end());
}

void RenameObjectsCommand::initialize()
{
    previousNames.clear();
    if (!document())
        return;
    previousNames.reserve(objectIds.size());
    for (Document::ObjectId id : objectIds) {
        if (const Document::ObjectNode* node = document()->findObject(id))
            previousNames.push_back(node->name);
        else
            previousNames.push_back(std::string());
    }
}

void RenameObjectsCommand::performRedo()
{
    if (!document())
        return;
    std::string name = newName.toStdString();
    for (Document::ObjectId id : objectIds)
        document()->renameObject(id, name);
}

void RenameObjectsCommand::performUndo()
{
    if (!document())
        return;
    for (std::size_t i = 0; i < objectIds.size() && i < previousNames.size(); ++i)
        document()->renameObject(objectIds[i], previousNames[i]);
}

SetObjectVisibilityCommand::SetObjectVisibilityCommand(Document::ObjectId id, bool visibleValue)
    : Core::Command(QObject::tr("Toggle Visibility"))
    , objectId(id)
    , visible(visibleValue)
{
}

void SetObjectVisibilityCommand::initialize()
{
    if (captured || !document())
        return;
    if (const Document::ObjectNode* node = document()->findObject(objectId)) {
        previous = node->visible;
        captured = true;
    }
}

void SetObjectVisibilityCommand::performRedo()
{
    if (!document())
        return;
    document()->setObjectVisible(objectId, visible);
}

void SetObjectVisibilityCommand::performUndo()
{
    if (!document())
        return;
    document()->setObjectVisible(objectId, previous);
}

SetObjectsVisibilityCommand::SetObjectsVisibilityCommand(const std::vector<Document::ObjectId>& ids, bool visible)
    : Core::Command(QObject::tr("Toggle Visibility"))
    , objectIds(ids)
    , newValue(visible)
{
    objectIds.erase(std::remove(objectIds.begin(), objectIds.end(), Document::ObjectId(0)), objectIds.end());
}

void SetObjectsVisibilityCommand::initialize()
{
    previousValues.clear();
    if (!document())
        return;
    previousValues.reserve(objectIds.size());
    for (Document::ObjectId id : objectIds) {
        if (const Document::ObjectNode* node = document()->findObject(id))
            previousValues.push_back(node->visible);
        else
            previousValues.push_back(true);
    }
}

void SetObjectsVisibilityCommand::performRedo()
{
    if (!document())
        return;
    for (Document::ObjectId id : objectIds)
        document()->setObjectVisible(id, newValue);
}

void SetObjectsVisibilityCommand::performUndo()
{
    if (!document())
        return;
    for (std::size_t i = 0; i < objectIds.size() && i < previousValues.size(); ++i)
        document()->setObjectVisible(objectIds[i], previousValues[i]);
}

AssignMaterialCommand::AssignMaterialCommand(const std::vector<Document::ObjectId>& ids, const QString& materialName)
    : Core::Command(QObject::tr("Assign Material"))
    , objectIds(ids)
    , material(materialName)
{
}

void AssignMaterialCommand::initialize()
{
    previous.clear();
    if (!document() || !geometry())
        return;
    for (Document::ObjectId id : objectIds) {
        const GeometryObject* object = document()->geometryForObject(id);
        if (!object)
            continue;
        previous[id] = geometry()->getMaterial(object);
    }
}

void AssignMaterialCommand::performRedo()
{
    if (!document() || !geometry())
        return;
    std::string materialName = material.toStdString();
    for (Document::ObjectId id : objectIds) {
        GeometryObject* object = document()->geometryForObject(id);
        if (!object)
            continue;
        geometry()->assignMaterial(object, materialName);
    }
}

void AssignMaterialCommand::performUndo()
{
    if (!document() || !geometry())
        return;
    for (const auto& entry : previous) {
        GeometryObject* object = document()->geometryForObject(entry.first);
        if (!object)
            continue;
        geometry()->assignMaterial(object, entry.second);
    }
}

SetObjectTransformCommand::SetObjectTransformCommand(std::vector<TransformChange> changes, const QString& description)
    : Core::Command(description)
    , changes(std::move(changes))
{
    changes.erase(std::remove_if(changes.begin(), changes.end(), [](const TransformChange& change) {
        return change.id == 0;
    }), changes.end());
}

void SetObjectTransformCommand::initialize()
{
    Core::Command::initialize();
    if (!document())
        changes.clear();
}

void SetObjectTransformCommand::performRedo()
{
    if (!document())
        return;

    std::vector<Document::ObjectId> ids;
    ids.reserve(changes.size());
    for (const auto& change : changes) {
        document()->applyTransform(change.id, change.after, change.mask);
        ids.push_back(change.id);
    }
    if (!ids.empty())
        setFinalSelection(ids);
}

void SetObjectTransformCommand::performUndo()
{
    if (!document())
        return;

    for (const auto& change : changes)
        document()->applyTransform(change.id, change.before, change.mask);
}

CreateTagCommand::CreateTagCommand(const QString& tagName, const SceneSettings::Color& tagColor)
    : Core::Command(QObject::tr("Create Tag"))
    , name(tagName)
    , color(tagColor)
{
}

void CreateTagCommand::initialize()
{
    if (!document() || created)
        return;
    savedAssignments.clear();
}

void CreateTagCommand::performRedo()
{
    if (!document())
        return;
    if (!created) {
        Document::TagId id = document()->createTag(name.toStdString(), color);
        auto it = document()->tags().find(id);
        if (it != document()->tags().end())
            savedTag = it->second;
        created = true;
    } else {
        document()->restoreTag(savedTag, savedAssignments);
    }
}

void CreateTagCommand::performUndo()
{
    if (!document() || !created)
        return;
    savedAssignments.clear();
    document()->deleteTag(savedTag.id, &savedAssignments);
}

RenameTagCommand::RenameTagCommand(Document::TagId id, const QString& nameValue)
    : Core::Command(QObject::tr("Rename Tag"))
    , tagId(id)
    , newName(nameValue)
{
}

void RenameTagCommand::initialize()
{
    if (captured || !document())
        return;
    auto it = document()->tags().find(tagId);
    if (it != document()->tags().end()) {
        previousName = it->second.name;
        captured = true;
    }
}

void RenameTagCommand::performRedo()
{
    if (!document())
        return;
    document()->renameTag(tagId, newName.toStdString());
}

void RenameTagCommand::performUndo()
{
    if (!document())
        return;
    document()->renameTag(tagId, previousName);
}

SetTagVisibilityCommand::SetTagVisibilityCommand(Document::TagId id, bool visibleValue)
    : Core::Command(QObject::tr("Toggle Tag Visibility"))
    , tagId(id)
    , newValue(visibleValue)
{
}

void SetTagVisibilityCommand::initialize()
{
    if (captured || !document())
        return;
    auto it = document()->tags().find(tagId);
    if (it != document()->tags().end()) {
        previousValue = it->second.visible;
        captured = true;
    }
}

void SetTagVisibilityCommand::performRedo()
{
    if (!document())
        return;
    document()->setTagVisible(tagId, newValue);
}

void SetTagVisibilityCommand::performUndo()
{
    if (!document())
        return;
    document()->setTagVisible(tagId, previousValue);
}

SetTagColorCommand::SetTagColorCommand(Document::TagId id, const SceneSettings::Color& colorValue)
    : Core::Command(QObject::tr("Change Tag Color"))
    , tagId(id)
    , newColor(colorValue)
{
}

void SetTagColorCommand::initialize()
{
    if (captured || !document())
        return;
    auto it = document()->tags().find(tagId);
    if (it != document()->tags().end()) {
        previousColor = it->second.color;
        captured = true;
    }
}

void SetTagColorCommand::performRedo()
{
    if (!document())
        return;
    document()->setTagColor(tagId, newColor);
}

void SetTagColorCommand::performUndo()
{
    if (!document())
        return;
    document()->setTagColor(tagId, previousColor);
}

RebuildCurveFromMetadataCommand::RebuildCurveFromMetadataCommand(Document::ObjectId id,
                                                                 const GeometryKernel::ShapeMetadata& metadata)
    : Core::Command(QObject::tr("Edit Curve"))
    , objectId(id)
    , newMetadata(metadata)
{
}

void RebuildCurveFromMetadataCommand::initialize()
{
    if (captured || !document() || !geometry())
        return;
    GeometryObject* object = document()->geometryForObject(objectId);
    if (!object)
        return;
    auto metadata = geometry()->shapeMetadata(object);
    if (!metadata.has_value())
        return;
    previousMetadata = *metadata;
    captured = true;
}

void RebuildCurveFromMetadataCommand::performRedo()
{
    applied = false;
    if (!captured || !document() || !geometry())
        return;
    GeometryObject* object = document()->geometryForObject(objectId);
    if (!object)
        return;
    applied = geometry()->rebuildShapeFromMetadata(object, newMetadata);
    if (applied)
        setFinalSelection({ objectId });
}

void RebuildCurveFromMetadataCommand::performUndo()
{
    if (!captured || !document() || !geometry())
        return;
    GeometryObject* object = document()->geometryForObject(objectId);
    if (!object)
        return;
    geometry()->rebuildShapeFromMetadata(object, previousMetadata);
}

} // namespace Scene

