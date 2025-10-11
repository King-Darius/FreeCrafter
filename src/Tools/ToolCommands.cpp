#include "ToolCommands.h"

#include "GeometryKernel/GeometryKernel.h"
#include "GeometryKernel/GeometryObject.h"
#include "ToolGeometryUtils.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace Tools {

CreateCurveCommand::CreateCurveCommand(std::vector<Vector3> points, const QString& description,
                                       std::optional<GeometryKernel::ShapeMetadata> metadata, std::string name)
    : Core::Command(description)
    , points_(std::move(points))
    , metadata_(std::move(metadata))
    , name_(std::move(name))
{
}

void CreateCurveCommand::performRedo()
{
    if (!geometry())
        return;
    GeometryObject* object = geometry()->addCurve(points_);
    if (!object)
        return;
    if (metadata_)
        geometry()->setShapeMetadata(object, *metadata_);
    if (document()) {
        createdId_ = document()->ensureObjectForGeometry(object, name_);
        if (createdId_ != 0)
            setFinalSelection({ createdId_ });
        else
            setFinalSelection({});
    }
}

void CreateCurveCommand::performUndo()
{
    if (!document())
        return;
    if (createdId_ != 0)
        document()->removeObject(createdId_);
    createdId_ = 0;
}

ExtrudeProfileCommand::ExtrudeProfileCommand(Scene::Document::ObjectId profileId,
    std::optional<Scene::Document::ObjectId> pathId, Vector3 direction, bool capStart, bool capEnd,
    const QString& description, std::string name)
    : Core::Command(description)
    , profileId_(profileId)
    , pathId_(std::move(pathId))
    , direction_(direction)
    , capStart_(capStart)
    , capEnd_(capEnd)
    , name_(std::move(name))
{
    if (name_.empty()) {
        name_ = "Extrusion";
    }
}

void ExtrudeProfileCommand::performRedo()
{
    if (!geometry() || !document())
        return;

    if (direction_.lengthSquared() <= 1e-10f)
        return;

    GeometryObject* profile = document()->geometryForObject(profileId_);
    if (!profile || profile->getType() != ObjectType::Curve)
        return;

    GeometryKernel::ExtrudeOptions options;
    options.capStart = capStart_;
    options.capEnd = capEnd_;

    GeometryObject* created = geometry()->extrudeCurveAlongVector(profile, direction_, options);
    if (!created)
        return;

    createdId_ = document()->ensureObjectForGeometry(created, name_);
    if (createdId_ != 0)
        setFinalSelection({ createdId_ });
    else
        setFinalSelection({});
}

void ExtrudeProfileCommand::performUndo()
{
    if (!document())
        return;
    if (createdId_ != 0)
        document()->removeObject(createdId_);
    createdId_ = 0;
}

TranslateObjectsCommand::TranslateObjectsCommand(std::vector<Scene::Document::ObjectId> ids, Vector3 delta,
                                                 const QString& description)
    : Core::Command(description)
    , ids_(std::move(ids))
    , delta_(delta)
{
    ids_.erase(std::remove(ids_.begin(), ids_.end(), Scene::Document::ObjectId(0)), ids_.end());
}

void TranslateObjectsCommand::performRedo()
{
    if (!geometry() || !document())
        return;
    for (Scene::Document::ObjectId id : ids_) {
        if (GeometryObject* object = document()->geometryForObject(id))
            translateObject(*object, delta_);
    }
    setFinalSelection(ids_);
}

void TranslateObjectsCommand::performUndo()
{
    if (!geometry() || !document())
        return;
    for (Scene::Document::ObjectId id : ids_) {
        if (GeometryObject* object = document()->geometryForObject(id))
            translateObject(*object, Vector3(-delta_.x, -delta_.y, -delta_.z));
    }
}

RotateObjectsCommand::RotateObjectsCommand(std::vector<Scene::Document::ObjectId> ids, Vector3 pivot, Vector3 axis,
                                           float angleRadians, const QString& description)
    : Core::Command(description)
    , ids_(std::move(ids))
    , pivot_(pivot)
    , axis_(axis)
    , angleRadians_(angleRadians)
{
    ids_.erase(std::remove(ids_.begin(), ids_.end(), Scene::Document::ObjectId(0)), ids_.end());
}

void RotateObjectsCommand::performRedo()
{
    if (!geometry() || !document())
        return;
    for (Scene::Document::ObjectId id : ids_) {
        if (GeometryObject* object = document()->geometryForObject(id))
            rotateObject(*object, pivot_, axis_, angleRadians_);
    }
    setFinalSelection(ids_);
}

void RotateObjectsCommand::performUndo()
{
    if (!geometry() || !document())
        return;
    for (Scene::Document::ObjectId id : ids_) {
        if (GeometryObject* object = document()->geometryForObject(id))
            rotateObject(*object, pivot_, axis_, -angleRadians_);
    }
}

ScaleObjectsCommand::ScaleObjectsCommand(std::vector<Scene::Document::ObjectId> ids, Vector3 pivot, Vector3 factors,
                                         const QString& description)
    : Core::Command(description)
    , ids_(std::move(ids))
    , pivot_(pivot)
    , factors_(factors)
{
    ids_.erase(std::remove(ids_.begin(), ids_.end(), Scene::Document::ObjectId(0)), ids_.end());
}

void ScaleObjectsCommand::performRedo()
{
    if (!geometry() || !document())
        return;
    for (Scene::Document::ObjectId id : ids_) {
        if (GeometryObject* object = document()->geometryForObject(id))
            scaleObject(*object, pivot_, factors_);
    }
    setFinalSelection(ids_);
}

void ScaleObjectsCommand::performUndo()
{
    if (!geometry() || !document())
        return;
    Vector3 inverse(1.0f, 1.0f, 1.0f);
    const float epsilon = 1e-5f;
    inverse.x = std::fabs(factors_.x) <= epsilon ? 1.0f : 1.0f / factors_.x;
    inverse.y = std::fabs(factors_.y) <= epsilon ? 1.0f : 1.0f / factors_.y;
    inverse.z = std::fabs(factors_.z) <= epsilon ? 1.0f : 1.0f / factors_.z;
    for (Scene::Document::ObjectId id : ids_) {
        if (GeometryObject* object = document()->geometryForObject(id))
            scaleObject(*object, pivot_, inverse);
    }
}

DeleteObjectsCommand::DeleteObjectsCommand(std::vector<Scene::Document::ObjectId> ids, const QString& description)
    : Core::Command(description)
    , requestedIds_(std::move(ids))
{
}

void DeleteObjectsCommand::initialize()
{
    entries_.clear();
    if (!document() || !geometry())
        return;

    std::unordered_set<Scene::Document::ObjectId> seen;
    for (Scene::Document::ObjectId id : requestedIds_) {
        if (id == 0)
            continue;
        if (!seen.insert(id).second)
            continue;
        GeometryObject* object = document()->geometryForObject(id);
        if (!object)
            continue;

        Entry entry;
        entry.originalId = id;
        entry.currentId = id;
        entry.prototype = object->clone();
        if (!entry.prototype)
            continue;

        std::string material = geometry()->getMaterial(object);
        if (!material.empty())
            entry.material = material;
        if (auto metadata = geometry()->shapeMetadata(object))
            entry.metadata = *metadata;

        if (const auto* node = document()->findObject(id)) {
            entry.name = node->name;
            entry.tags = node->tags;
            entry.parentId = node->parent ? node->parent->id : 0;
            if (node->parent) {
                const auto& siblings = node->parent->children;
                for (std::size_t idx = 0; idx < siblings.size(); ++idx) {
                    if (siblings[idx].get() == node) {
                        entry.childIndex = idx;
                        break;
                    }
                }
            }
        }

        entries_.push_back(std::move(entry));
    }
}

void DeleteObjectsCommand::performRedo()
{
    if (!document())
        return;
    for (auto& entry : entries_) {
        if (entry.currentId != 0)
            document()->removeObject(entry.currentId);
        entry.currentId = 0;
    }
    setFinalSelection({});
}

void DeleteObjectsCommand::performUndo()
{
    if (!document() || !geometry())
        return;

    std::vector<Scene::Document::ObjectId> updatedSelection = initialSelection();

    for (auto& entry : entries_) {
        if (!entry.prototype)
            continue;

        std::unique_ptr<GeometryObject> clone = entry.prototype->clone();
        if (!clone)
            continue;

        GeometryObject* added = geometry()->addObject(std::move(clone));
        if (!added)
            continue;

        if (entry.metadata)
            geometry()->setShapeMetadata(added, *entry.metadata);
        if (entry.material)
            geometry()->assignMaterial(added, *entry.material);

        Scene::Document::ObjectId newId = document()->ensureObjectForGeometry(added, entry.name);
        if (newId == 0)
            continue;

        for (Scene::Document::TagId tag : entry.tags)
            document()->assignTag(newId, tag);

        if (entry.parentId != 0 || entry.childIndex != 0)
            document()->moveObject(newId, entry.parentId, entry.childIndex);

        entry.currentId = newId;

        for (auto& id : updatedSelection) {
            if (id == entry.originalId)
                id = newId;
        }
    }

    overrideInitialSelection(updatedSelection);
    setFinalSelection({});
}

} // namespace Tools

