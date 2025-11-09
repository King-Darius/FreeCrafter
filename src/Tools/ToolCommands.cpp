#include "ToolCommands.h"

#include "GeometryKernel/GeometryKernel.h"
#include "GeometryKernel/GeometryObject.h"
#include "GeometryKernel/Curve.h"
#include "GeometryKernel/Solid.h"
#include "ToolGeometryUtils.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>
#include <utility>

namespace Tools {

namespace {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float ix, float iy)
        : x(ix)
        , y(iy)
    {
    }

    explicit Vec2(const Vector3& v)
        : x(v.x)
        , y(v.z)
    {
    }

    Vec2 operator+(const Vec2& other) const { return { x + other.x, y + other.y }; }
    Vec2 operator-(const Vec2& other) const { return { x - other.x, y - other.y }; }
    Vec2 operator*(float s) const { return { x * s, y * s }; }
    float dot(const Vec2& other) const { return x * other.x + y * other.y; }
    Vec2 normalized() const
    {
        float len = std::sqrt(x * x + y * y);
        if (len <= 1e-6f)
            return {};
        return { x / len, y / len };
    }
};

Vector3 toVec3(const Vec2& v)
{
    return { v.x, 0.0f, v.y };
}

std::vector<Vector3> offsetLoop(const std::vector<Vector3>& loop, float distance)
{
    const size_t count = loop.size();
    if (count < 3)
        return {};

    std::vector<Vector3> result;
    result.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        Vec2 prev(loop[(i + count - 1) % count]);
        Vec2 curr(loop[i]);
        Vec2 next(loop[(i + 1) % count]);

        Vec2 dir1 = (curr - prev).normalized();
        Vec2 dir2 = (next - curr).normalized();
        Vec2 normal1(-dir1.y, dir1.x);
        Vec2 normal2(-dir2.y, dir2.x);
        Vec2 bisector = (normal1 + normal2).normalized();
        if (std::sqrt(bisector.x * bisector.x + bisector.y * bisector.y) <= 1e-5f)
            bisector = normal1;

        float denom = bisector.dot(normal1);
        if (std::fabs(denom) < 1e-5f)
            denom = denom < 0.0f ? -1.0f : 1.0f;

        Vec2 offset = bisector * (distance / denom);
        result.push_back(toVec3(Vec2(curr) + offset));
    }
    return result;
}

} // namespace

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
    }
    setFinalSelection({});
}

ApplyChamferCommand::ApplyChamferCommand(Scene::Document::ObjectId curveId, Phase6::RoundCornerOptions options,
                                         const QString& description)
    : Core::Command(description)
    , curveId_(curveId)
    , options_(std::move(options))
{
}

void ApplyChamferCommand::initialize()
{
    Core::Command::initialize();
    if (captured_)
        return;
    if (!document())
        return;
    GeometryObject* object = document()->geometryForObject(curveId_);
    if (!object || object->getType() != ObjectType::Curve)
        return;
    auto* curve = static_cast<Curve*>(object);
    originalLoop_ = curve->getBoundaryLoop();
    originalHardness_ = curve->getEdgeHardness();
    captured_ = true;
}

void ApplyChamferCommand::performRedo()
{
    if (!geometry() || !document())
        return;
    GeometryObject* object = document()->geometryForObject(curveId_);
    if (!object || object->getType() != ObjectType::Curve)
        return;
    auto* curve = static_cast<Curve*>(object);
    Phase6::RoundCorner round(*geometry());
    if (round.filletCurve(*curve, options_))
        setFinalSelection({ curveId_ });
}

void ApplyChamferCommand::performUndo()
{
    if (!geometry() || !document())
        return;
    GeometryObject* object = document()->geometryForObject(curveId_);
    if (!object || object->getType() != ObjectType::Curve)
        return;
    auto* curve = static_cast<Curve*>(object);
    if (!originalLoop_.empty())
        curve->rebuildFromPoints(originalLoop_, originalHardness_);
}

CreateLoftCommand::CreateLoftCommand(Scene::Document::ObjectId startId, Scene::Document::ObjectId endId,
                                     Phase6::LoftOptions options, const QString& description, std::string name)
    : Core::Command(description)
    , startId_(startId)
    , endId_(endId)
    , options_(std::move(options))
    , name_(std::move(name))
{
    if (name_.empty())
        name_ = "Loft";
}

void CreateLoftCommand::performRedo()
{
    if (!geometry() || !document())
        return;
    GeometryObject* start = document()->geometryForObject(startId_);
    GeometryObject* end = document()->geometryForObject(endId_);
    if (!start || !end || start->getType() != ObjectType::Curve || end->getType() != ObjectType::Curve)
        return;
    Phase6::CurveIt op(*geometry());
    Solid* solid = op.loft(*static_cast<Curve*>(start), *static_cast<Curve*>(end), options_);
    if (!solid)
        return;
    createdId_ = document()->ensureObjectForGeometry(solid, name_);
    if (createdId_ != 0)
        setFinalSelection({ createdId_ });
    else
        setFinalSelection({});
}

void CreateLoftCommand::performUndo()
{
    if (!document())
        return;
    if (createdId_ != 0) {
        document()->removeObject(createdId_);
        createdId_ = 0;
    }
}

OffsetCurveCommand::OffsetCurveCommand(std::vector<Scene::Document::ObjectId> sourceIds, float distance,
                                       const QString& description, std::string name)
    : Core::Command(description)
    , entries_()
    , distance_(distance)
    , fallbackName_(std::move(name))
{
    entries_.reserve(sourceIds.size());
    for (Scene::Document::ObjectId id : sourceIds) {
        if (id == 0)
            continue;
        entries_.push_back(Entry{ id });
    }
}

void OffsetCurveCommand::initialize()
{
    entries_.erase(std::remove_if(entries_.begin(), entries_.end(), [](const Entry& entry) {
                       return entry.sourceId == 0;
                   }),
        entries_.end());

    if (!document())
        return;

    const std::string defaultName = fallbackName_.empty() ? "Offset Curve" : fallbackName_;
    for (auto& entry : entries_) {
        if (!entry.name.empty())
            continue;
        const auto* node = document()->findObject(entry.sourceId);
        if (node && !node->name.empty())
            entry.name = node->name + " Offset";
        else
            entry.name = defaultName;
    }
}

void OffsetCurveCommand::performRedo()
{
    if (!geometry() || !document())
        return;
    if (std::fabs(distance_) <= 1e-6f)
        return;

    std::vector<Scene::Document::ObjectId> selection;
    for (auto& entry : entries_) {
        GeometryObject* object = document()->geometryForObject(entry.sourceId);
        if (!object || object->getType() != ObjectType::Curve)
            continue;
        auto* curve = static_cast<Curve*>(object);

        if (entry.offsetPoints.empty()) {
            entry.offsetPoints = offsetLoop(curve->getBoundaryLoop(), distance_);
        }

        if (entry.offsetPoints.empty())
            continue;

        GeometryObject* created = geometry()->addCurve(entry.offsetPoints);
        if (!created)
            continue;

        const std::string& name = entry.name.empty() ? (fallbackName_.empty() ? "Offset Curve" : fallbackName_) : entry.name;
        entry.createdId = document()->ensureObjectForGeometry(created, name);
        if (entry.createdId != 0)
            selection.push_back(entry.createdId);
    }

    if (selection.empty())
        setFinalSelection({});
    else
        setFinalSelection(selection);
}

void OffsetCurveCommand::performUndo()
{
    if (!document())
        return;

    for (auto& entry : entries_) {
        if (entry.createdId == 0)
            continue;
        document()->removeObject(entry.createdId);
        entry.createdId = 0;
    }
}

PushPullCommand::PushPullCommand(std::vector<Scene::Document::ObjectId> sourceIds, float distance,
                                 const QString& description, std::string name)
    : Core::Command(description)
    , entries_()
    , distance_(distance)
    , fallbackName_(std::move(name))
{
    entries_.reserve(sourceIds.size());
    for (Scene::Document::ObjectId id : sourceIds) {
        if (id == 0)
            continue;
        entries_.push_back(Entry{ id });
    }
}

void PushPullCommand::initialize()
{
    entries_.erase(std::remove_if(entries_.begin(), entries_.end(), [](const Entry& entry) {
                       return entry.sourceId == 0;
                   }),
        entries_.end());

    if (!document())
        return;

    const std::string defaultName = fallbackName_.empty() ? "Push/Pull" : fallbackName_;
    for (auto& entry : entries_) {
        if (!entry.name.empty())
            continue;
        const auto* node = document()->findObject(entry.sourceId);
        if (node && !node->name.empty())
            entry.name = node->name + " Extrusion";
        else
            entry.name = defaultName;
    }
}

void PushPullCommand::performRedo()
{
    if (!geometry() || !document())
        return;
    if (std::fabs(distance_) <= 1e-6f)
        return;

    std::vector<Scene::Document::ObjectId> selection;
    for (auto& entry : entries_) {
        GeometryObject* object = document()->geometryForObject(entry.sourceId);
        if (!object || object->getType() != ObjectType::Curve)
            continue;

        GeometryObject* created = geometry()->extrudeCurve(object, distance_);
        if (!created)
            continue;

        const std::string& name = entry.name.empty() ? (fallbackName_.empty() ? "Push/Pull" : fallbackName_) : entry.name;
        entry.createdId = document()->ensureObjectForGeometry(created, name);
        if (entry.createdId != 0)
            selection.push_back(entry.createdId);
    }

    if (selection.empty())
        setFinalSelection({});
    else
        setFinalSelection(selection);
}

void PushPullCommand::performUndo()
{
    if (!document())
        return;

    for (auto& entry : entries_) {
        if (entry.createdId == 0)
            continue;
        document()->removeObject(entry.createdId);
        entry.createdId = 0;
    }
}

FollowMeCommand::FollowMeCommand(Scene::Document::ObjectId profileId, Scene::Document::ObjectId pathId,
                                 const QString& description, std::string name)
    : Core::Command(description)
    , profileId_(profileId)
    , pathId_(pathId)
    , fallbackName_(std::move(name))
{
}

void FollowMeCommand::initialize()
{
    sections_.clear();
    createdIds_.clear();
    names_.clear();
}

void FollowMeCommand::performRedo()
{
    if (!geometry() || !document())
        return;

    GeometryObject* profile = document()->geometryForObject(profileId_);
    GeometryObject* path = document()->geometryForObject(pathId_);
    if (!profile || !path || profile->getType() != ObjectType::Curve || path->getType() != ObjectType::Curve)
        return;

    auto* profileCurve = static_cast<Curve*>(profile);
    auto* pathCurve = static_cast<Curve*>(path);
    const auto& profileLoop = profileCurve->getBoundaryLoop();
    const auto& pathLoop = pathCurve->getBoundaryLoop();
    if (profileLoop.empty() || pathLoop.size() < 2)
        return;

    if (sections_.empty()) {
        sections_.reserve(pathLoop.size());
        Vector3 start = pathLoop.front();
        for (const auto& point : pathLoop) {
            Vector3 offset = point - start;
            std::vector<Vector3> section;
            section.reserve(profileLoop.size());
            for (const auto& p : profileLoop)
                section.push_back(p + Vector3(offset.x, 0.0f, offset.z));
            sections_.push_back(std::move(section));
        }
        createdIds_.assign(sections_.size(), 0);
        names_.resize(sections_.size());
    }

    const Scene::Document::ObjectNode* pathNode = document()->findObject(pathId_);
    std::string baseName;
    if (pathNode && !pathNode->name.empty())
        baseName = pathNode->name + " Section";
    else
        baseName = fallbackName_.empty() ? "Follow Me Section" : fallbackName_;

    std::vector<Scene::Document::ObjectId> selection;
    for (std::size_t i = 0; i < sections_.size(); ++i) {
        GeometryObject* created = geometry()->addCurve(sections_[i]);
        if (!created)
            continue;

        if (names_.size() > i && names_[i].empty()) {
            if (sections_.size() > 1)
                names_[i] = baseName + " " + std::to_string(i + 1);
            else
                names_[i] = baseName;
        }

        std::string name = (names_.size() > i && !names_[i].empty()) ? names_[i]
                               : (sections_.size() > 1 ? baseName + " " + std::to_string(i + 1) : baseName);

        Scene::Document::ObjectId createdId = document()->ensureObjectForGeometry(created, name);
        if (names_.size() > i)
            names_[i] = name;
        if (createdId == 0)
            continue;

        if (createdIds_.size() > i)
            createdIds_[i] = createdId;
        else
            createdIds_.push_back(createdId);
        selection.push_back(createdId);
    }

    if (selection.empty())
        setFinalSelection({});
    else
        setFinalSelection(selection);
}

void FollowMeCommand::performUndo()
{
    if (!document())
        return;

    for (auto& id : createdIds_) {
        if (id == 0)
            continue;
        document()->removeObject(id);
        id = 0;
    }
}

CreateTextAnnotationCommand::CreateTextAnnotationCommand(Vector3 position, std::string text, float height,
                                                         const QString& description)
    : Core::Command(description)
    , position_(position)
    , text_(std::move(text))
    , height_(height)
{
}

void CreateTextAnnotationCommand::performRedo()
{
    if (!geometry())
        return;
    if (text_.empty())
        return;

    if (!created_) {
        index_ = geometry()->addTextAnnotation(position_, text_, height_);
        if (index_ == std::numeric_limits<std::size_t>::max())
            return;
        if (index_ < geometry()->getTextAnnotations().size()) {
            const auto& annotation = geometry()->getTextAnnotations().at(index_);
            position_ = annotation.position;
            text_ = annotation.text;
            height_ = annotation.height;
        }
        created_ = true;
    } else {
        geometry()->insertTextAnnotation(index_, position_, text_, height_);
    }
}

void CreateTextAnnotationCommand::performUndo()
{
    if (!geometry())
        return;
    if (!created_)
        return;
    geometry()->removeTextAnnotation(index_);
}

} // namespace Tools

