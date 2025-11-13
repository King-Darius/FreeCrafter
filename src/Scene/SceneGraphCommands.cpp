#include "SceneGraphCommands.h"

#include "GeometryKernel/GeometryKernel.h"

#include <QObject>
#include <algorithm>

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

RenameObjectsCommand::RenameObjectsCommand(std::vector<Document::ObjectId> ids, const QString& name)
    : Core::Command(QObject::tr("Rename Objects"))
    , objectIds(std::move(ids))
    , newName(name)
{
    objectIds.erase(std::remove(objectIds.begin(), objectIds.end(), Document::ObjectId(0)), objectIds.end());
}

void RenameObjectsCommand::initialize()
{
    if (captured || !document())
        return;
    previousNames.clear();
    previousNames.reserve(objectIds.size());
    for (Document::ObjectId id : objectIds) {
        const Document::ObjectNode* node = document()->findObject(id);
        previousNames.push_back(node ? node->name : std::string());
    }
    captured = true;
}

void RenameObjectsCommand::performRedo()
{
    if (!document())
        return;
    for (Document::ObjectId id : objectIds)
        document()->renameObject(id, newName.toStdString());
}

void RenameObjectsCommand::performUndo()
{
    if (!document())
        return;
    for (std::size_t index = 0; index < objectIds.size(); ++index)
        document()->renameObject(objectIds[index], previousNames[index]);
}

SetObjectsVisibilityCommand::SetObjectsVisibilityCommand(std::vector<Document::ObjectId> ids, bool visibleValue)
    : Core::Command(QObject::tr("Set Visibility"))
    , objectIds(std::move(ids))
    , visible(visibleValue)
{
    objectIds.erase(std::remove(objectIds.begin(), objectIds.end(), Document::ObjectId(0)), objectIds.end());
}

void SetObjectsVisibilityCommand::initialize()
{
    if (captured || !document())
        return;
    previousValues.clear();
    previousValues.reserve(objectIds.size());
    for (Document::ObjectId id : objectIds) {
        const Document::ObjectNode* node = document()->findObject(id);
        previousValues.push_back(node ? node->visible : true);
    }
    captured = true;
}

void SetObjectsVisibilityCommand::performRedo()
{
    if (!document())
        return;
    for (Document::ObjectId id : objectIds)
        document()->setObjectVisible(id, visible);
}

void SetObjectsVisibilityCommand::performUndo()
{
    if (!document())
        return;
    for (std::size_t index = 0; index < objectIds.size(); ++index)
        document()->setObjectVisible(objectIds[index], previousValues[index]);
}

SetTagAssignmentsCommand::SetTagAssignmentsCommand(Document::TagId id, std::vector<Document::ObjectId> ids, bool assignValue)
    : Core::Command(assignValue ? QObject::tr("Assign Tag") : QObject::tr("Remove Tag"))
    , tagId(id)
    , objectIds(std::move(ids))
    , assign(assignValue)
{
    objectIds.erase(std::remove(objectIds.begin(), objectIds.end(), Document::ObjectId(0)), objectIds.end());
}

void SetTagAssignmentsCommand::initialize()
{
    if (captured || !document())
        return;
    previouslyHad.clear();
    previouslyHad.reserve(objectIds.size());
    for (Document::ObjectId id : objectIds) {
        const Document::ObjectNode* node = document()->findObject(id);
        bool had = false;
        if (node) {
            had = std::find(node->tags.begin(), node->tags.end(), tagId) != node->tags.end();
        }
        previouslyHad.push_back(had);
    }
    captured = true;
}

void SetTagAssignmentsCommand::performRedo()
{
    if (!document())
        return;
    for (Document::ObjectId id : objectIds) {
        if (assign)
            document()->assignTag(id, tagId);
        else
            document()->removeTag(id, tagId);
    }
}

void SetTagAssignmentsCommand::performUndo()
{
    if (!document())
        return;
    for (std::size_t index = 0; index < objectIds.size(); ++index) {
        Document::ObjectId id = objectIds[index];
        bool had = previouslyHad[index];
        if (had)
            document()->assignTag(id, tagId);
        else
            document()->removeTag(id, tagId);
    }
}

RebuildCurveFromMetadataCommand::RebuildCurveFromMetadataCommand(Document::ObjectId id,
                                                                 GeometryKernel::ShapeMetadata metadata,
                                                                 const QString& description)
    : Core::Command(description)
    , objectId(id)
    , newMetadata(std::move(metadata))
{
}

void RebuildCurveFromMetadataCommand::initialize()
{
    if (captured || !document() || !geometry())
        return;
    GeometryObject* object = document()->geometryForObject(objectId);
    if (!object || object->getType() != ObjectType::Curve)
        return;
    previousMetadata = geometry()->shapeMetadata(object);
    captured = true;
}

void RebuildCurveFromMetadataCommand::performRedo()
{
    if (!document() || !geometry())
        return;
    GeometryObject* object = document()->geometryForObject(objectId);
    if (!object)
        return;
    geometry()->rebuildShapeFromMetadata(object, newMetadata);
    setFinalSelection({ objectId });
}

void RebuildCurveFromMetadataCommand::performUndo()
{
    if (!document() || !geometry() || !previousMetadata)
        return;
    GeometryObject* object = document()->geometryForObject(objectId);
    if (!object)
        return;
    geometry()->rebuildShapeFromMetadata(object, *previousMetadata);
}

} // namespace Scene

