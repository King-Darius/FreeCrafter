#include "SceneSerializer.h"

#include "Document.h"
#include "SceneSettings.h"
#include "SectionPlane.h"

#include "../CameraController.h"
#include "../GeometryKernel/GeometryKernel.h"
#include "../GeometryKernel/GeometryObject.h"
#include "../GeometryKernel/Vector3.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <memory>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>

namespace Scene {

namespace {

constexpr char kMagic[4] = {'F', 'C', 'S', 'N'};

QString nodeKindToString(Document::NodeKind kind)
{
    switch (kind) {
    case Document::NodeKind::Root:
        return QStringLiteral("root");
    case Document::NodeKind::Geometry:
        return QStringLiteral("geometry");
    case Document::NodeKind::Group:
        return QStringLiteral("group");
    case Document::NodeKind::ComponentInstance:
        return QStringLiteral("componentInstance");
    }
    return QStringLiteral("geometry");
}

Document::NodeKind nodeKindFromString(const QString& value)
{
    if (value == QLatin1String("root"))
        return Document::NodeKind::Root;
    if (value == QLatin1String("group"))
        return Document::NodeKind::Group;
    if (value == QLatin1String("componentInstance"))
        return Document::NodeKind::ComponentInstance;
    return Document::NodeKind::Geometry;
}

QString projectionModeToString(CameraController::ProjectionMode mode)
{
    switch (mode) {
    case CameraController::ProjectionMode::Perspective:
        return QStringLiteral("perspective");
    case CameraController::ProjectionMode::Parallel:
        return QStringLiteral("parallel");
    }
    return QStringLiteral("perspective");
}

CameraController::ProjectionMode projectionModeFromString(const QString& value)
{
    if (value == QLatin1String("parallel"))
        return CameraController::ProjectionMode::Parallel;
    return CameraController::ProjectionMode::Perspective;
}

QString shadowQualityToString(SceneSettings::ShadowQuality quality)
{
    switch (quality) {
    case SceneSettings::ShadowQuality::Low:
        return QStringLiteral("low");
    case SceneSettings::ShadowQuality::Medium:
        return QStringLiteral("medium");
    case SceneSettings::ShadowQuality::High:
        return QStringLiteral("high");
    }
    return QStringLiteral("medium");
}

SceneSettings::ShadowQuality shadowQualityFromString(const QString& value)
{
    if (value == QLatin1String("low"))
        return SceneSettings::ShadowQuality::Low;
    if (value == QLatin1String("high"))
        return SceneSettings::ShadowQuality::High;
    return SceneSettings::ShadowQuality::Medium;
}

QString fileFormatToString(Document::FileFormat fmt)
{
    switch (fmt) {
    case Document::FileFormat::Auto:
        return QStringLiteral("auto");
    case Document::FileFormat::Obj:
        return QStringLiteral("obj");
    case Document::FileFormat::Stl:
        return QStringLiteral("stl");
    case Document::FileFormat::Gltf:
        return QStringLiteral("gltf");
    case Document::FileFormat::Fbx:
        return QStringLiteral("fbx");
    case Document::FileFormat::Dxf:
        return QStringLiteral("dxf");
    case Document::FileFormat::Dwg:
        return QStringLiteral("dwg");
    }
    return QStringLiteral("auto");
}

Document::FileFormat fileFormatFromString(const QString& value)
{
    if (value == QLatin1String("obj"))
        return Document::FileFormat::Obj;
    if (value == QLatin1String("stl"))
        return Document::FileFormat::Stl;
    if (value == QLatin1String("gltf"))
        return Document::FileFormat::Gltf;
    if (value == QLatin1String("fbx"))
        return Document::FileFormat::Fbx;
    if (value == QLatin1String("dxf"))
        return Document::FileFormat::Dxf;
    if (value == QLatin1String("dwg"))
        return Document::FileFormat::Dwg;
    return Document::FileFormat::Auto;
}

QJsonObject colorToJson(const SceneSettings::Color& color)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("r"), color.r);
    obj.insert(QStringLiteral("g"), color.g);
    obj.insert(QStringLiteral("b"), color.b);
    obj.insert(QStringLiteral("a"), color.a);
    return obj;
}

SceneSettings::Color colorFromJson(const QJsonObject& obj)
{
    SceneSettings::Color color;
    color.r = static_cast<float>(obj.value(QStringLiteral("r")).toDouble(color.r));
    color.g = static_cast<float>(obj.value(QStringLiteral("g")).toDouble(color.g));
    color.b = static_cast<float>(obj.value(QStringLiteral("b")).toDouble(color.b));
    color.a = static_cast<float>(obj.value(QStringLiteral("a")).toDouble(color.a));
    return color;
}

QJsonArray vectorToJson(const Vector3& v)
{
    QJsonArray arr;
    arr.append(v.x);
    arr.append(v.y);
    arr.append(v.z);
    return arr;
}

Vector3 vectorFromJson(const QJsonArray& arr)
{
    Vector3 v;
    if (arr.size() >= 3) {
        v.x = static_cast<float>(arr.at(0).toDouble(v.x));
        v.y = static_cast<float>(arr.at(1).toDouble(v.y));
        v.z = static_cast<float>(arr.at(2).toDouble(v.z));
    }
    return v;
}

QJsonArray transformToJson(const std::array<float, 16>& matrix)
{
    QJsonArray arr;
    for (float value : matrix)
        arr.append(value);
    return arr;
}

std::array<float, 16> transformFromJson(const QJsonArray& arr)
{
    std::array<float, 16> matrix{};
    matrix.fill(0.0f);
    const int count = std::min(arr.size(), 16);
    for (int i = 0; i < count; ++i)
        matrix[static_cast<std::size_t>(i)] = static_cast<float>(arr.at(i).toDouble(matrix[i]));
    return matrix;
}

QJsonObject sectionPlaneToJson(const SectionPlane& plane)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("normal"), vectorToJson(plane.getNormal()));
    obj.insert(QStringLiteral("offset"), plane.getOffset());
    obj.insert(QStringLiteral("origin"), vectorToJson(plane.getOrigin()));
    obj.insert(QStringLiteral("transform"), transformToJson(plane.getTransform()));
    const auto& fill = plane.fillStyle();
    QJsonObject fillObj;
    fillObj.insert(QStringLiteral("color"), colorToJson({fill.red, fill.green, fill.blue, fill.alpha}));
    fillObj.insert(QStringLiteral("extent"), fill.extent);
    fillObj.insert(QStringLiteral("enabled"), fill.fillEnabled);
    obj.insert(QStringLiteral("fill"), fillObj);
    obj.insert(QStringLiteral("visible"), plane.isVisible());
    obj.insert(QStringLiteral("active"), plane.isActive());
    return obj;
}

SectionPlane sectionPlaneFromJson(const QJsonObject& obj)
{
    SectionPlane plane;
    const QJsonArray normalArray = obj.value(QStringLiteral("normal")).toArray();
    const QJsonArray originArray = obj.value(QStringLiteral("origin")).toArray();
    plane.setFromOriginAndNormal(vectorFromJson(originArray), vectorFromJson(normalArray));
    plane.setTransform(transformFromJson(obj.value(QStringLiteral("transform")).toArray()));
    SectionFillStyle style = plane.fillStyle();
    const QJsonObject fillObj = obj.value(QStringLiteral("fill")).toObject();
    const SceneSettings::Color color = colorFromJson(fillObj.value(QStringLiteral("color")).toObject());
    style.red = color.r;
    style.green = color.g;
    style.blue = color.b;
    style.alpha = color.a;
    style.extent = static_cast<float>(fillObj.value(QStringLiteral("extent")).toDouble(style.extent));
    style.fillEnabled = fillObj.value(QStringLiteral("enabled")).toBool(style.fillEnabled);
    plane.fillStyle() = style;
    plane.setVisible(obj.value(QStringLiteral("visible")).toBool(plane.isVisible()));
    plane.setActive(obj.value(QStringLiteral("active")).toBool(plane.isActive()));
    return plane;
}

QJsonObject gridSettingsToJson(const SceneSettings::GridSettings& grid)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("majorSpacing"), grid.majorSpacing);
    obj.insert(QStringLiteral("minorDivisions"), grid.minorDivisions);
    obj.insert(QStringLiteral("majorExtent"), grid.majorExtent);
    return obj;
}

SceneSettings::GridSettings gridSettingsFromJson(const QJsonObject& obj)
{
    SceneSettings::GridSettings grid;
    grid.majorSpacing = static_cast<float>(obj.value(QStringLiteral("majorSpacing")).toDouble(grid.majorSpacing));
    grid.minorDivisions = obj.value(QStringLiteral("minorDivisions")).toInt(grid.minorDivisions);
    grid.majorExtent = obj.value(QStringLiteral("majorExtent")).toInt(grid.majorExtent);
    return grid;
}

QJsonObject shadowSettingsToJson(const SceneSettings::ShadowSettings& shadow)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("enabled"), shadow.enabled);
    obj.insert(QStringLiteral("quality"), shadowQualityToString(shadow.quality));
    obj.insert(QStringLiteral("strength"), shadow.strength);
    obj.insert(QStringLiteral("bias"), shadow.bias);
    return obj;
}

SceneSettings::ShadowSettings shadowSettingsFromJson(const QJsonObject& obj)
{
    SceneSettings::ShadowSettings shadow;
    shadow.enabled = obj.value(QStringLiteral("enabled")).toBool(shadow.enabled);
    shadow.quality = shadowQualityFromString(obj.value(QStringLiteral("quality")).toString());
    shadow.strength = static_cast<float>(obj.value(QStringLiteral("strength")).toDouble(shadow.strength));
    shadow.bias = static_cast<float>(obj.value(QStringLiteral("bias")).toDouble(shadow.bias));
    return shadow;
}

QJsonObject paletteToJson(const SceneSettings::PaletteState& palette)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("id"), QString::fromStdString(palette.id));
    obj.insert(QStringLiteral("fill"), colorToJson(palette.fill));
    obj.insert(QStringLiteral("edge"), colorToJson(palette.edge));
    obj.insert(QStringLiteral("highlight"), colorToJson(palette.highlight));
    return obj;
}

SceneSettings::PaletteState paletteFromJson(const QJsonObject& obj)
{
    SceneSettings::PaletteState palette = {};
    palette.id = obj.value(QStringLiteral("id")).toString(QString::fromStdString(palette.id)).toStdString();
    palette.fill = colorFromJson(obj.value(QStringLiteral("fill")).toObject());
    palette.edge = colorFromJson(obj.value(QStringLiteral("edge")).toObject());
    palette.highlight = colorFromJson(obj.value(QStringLiteral("highlight")).toObject());
    return palette;
}

QJsonObject sceneSettingsToJson(const SceneSettings& settings)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("planesVisible"), settings.sectionPlanesVisible());
    obj.insert(QStringLiteral("fillsVisible"), settings.sectionFillsVisible());
    obj.insert(QStringLiteral("guidesVisible"), settings.guidesVisible());
    obj.insert(QStringLiteral("grid"), gridSettingsToJson(settings.grid()));
    obj.insert(QStringLiteral("shadows"), shadowSettingsToJson(settings.shadows()));
    obj.insert(QStringLiteral("palette"), paletteToJson(settings.palette()));
    return obj;
}

void applySceneSettings(const QJsonObject& obj, SceneSettings& settings)
{
    settings.setSectionPlanesVisible(obj.value(QStringLiteral("planesVisible")).toBool(settings.sectionPlanesVisible()));
    settings.setSectionFillsVisible(obj.value(QStringLiteral("fillsVisible")).toBool(settings.sectionFillsVisible()));
    settings.setGuidesVisible(obj.value(QStringLiteral("guidesVisible")).toBool(settings.guidesVisible()));
    settings.setGrid(gridSettingsFromJson(obj.value(QStringLiteral("grid")).toObject()));
    settings.setShadows(shadowSettingsFromJson(obj.value(QStringLiteral("shadows")).toObject()));
    settings.setPalette(paletteFromJson(obj.value(QStringLiteral("palette")).toObject()));
}

QJsonObject cameraStateToJson(const Document::SceneState::CameraState& camera)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("yaw"), camera.yaw);
    obj.insert(QStringLiteral("pitch"), camera.pitch);
    obj.insert(QStringLiteral("distance"), camera.distance);
    obj.insert(QStringLiteral("target"), vectorToJson(camera.target));
    obj.insert(QStringLiteral("projection"), projectionModeToString(camera.projection));
    obj.insert(QStringLiteral("fieldOfView"), camera.fieldOfView);
    obj.insert(QStringLiteral("orthoHeight"), camera.orthoHeight);
    return obj;
}

Document::SceneState::CameraState cameraStateFromJson(const QJsonObject& obj)
{
    Document::SceneState::CameraState camera;
    camera.yaw = static_cast<float>(obj.value(QStringLiteral("yaw")).toDouble(camera.yaw));
    camera.pitch = static_cast<float>(obj.value(QStringLiteral("pitch")).toDouble(camera.pitch));
    camera.distance = static_cast<float>(obj.value(QStringLiteral("distance")).toDouble(camera.distance));
    camera.target = vectorFromJson(obj.value(QStringLiteral("target")).toArray());
    camera.projection = projectionModeFromString(obj.value(QStringLiteral("projection")).toString());
    camera.fieldOfView = static_cast<float>(obj.value(QStringLiteral("fieldOfView")).toDouble(camera.fieldOfView));
    camera.orthoHeight = static_cast<float>(obj.value(QStringLiteral("orthoHeight")).toDouble(camera.orthoHeight));
    return camera;
}

QJsonObject prototypeToJson(const Document::PrototypeNode& proto,
                            const std::unordered_map<const GeometryObject*, std::size_t>& lookup)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("kind"), nodeKindToString(proto.kind));
    obj.insert(QStringLiteral("name"), QString::fromStdString(proto.name));
    if (proto.geometry) {
        auto it = lookup.find(proto.geometry);
        if (it != lookup.end())
            obj.insert(QStringLiteral("geometry"), static_cast<double>(it->second));
    }
    QJsonArray tags;
    for (Document::TagId id : proto.tags)
        tags.append(static_cast<double>(id));
    obj.insert(QStringLiteral("tags"), tags);

    QJsonArray children;
    for (const auto& child : proto.children)
        children.append(prototypeToJson(*child, lookup));
    obj.insert(QStringLiteral("children"), children);
    return obj;
}

std::unique_ptr<Document::PrototypeNode> prototypeFromJson(
    const QJsonObject& obj,
    const std::vector<GeometryObject*>& geometryObjects)
{
    auto proto = std::make_unique<Document::PrototypeNode>();
    proto->kind = nodeKindFromString(obj.value(QStringLiteral("kind")).toString());
    proto->name = obj.value(QStringLiteral("name")).toString().toStdString();
    const QJsonValue geometryValue = obj.value(QStringLiteral("geometry"));
    if (geometryValue.isDouble()) {
        const double index = geometryValue.toDouble(-1.0);
        if (index >= 0.0 && std::isfinite(index)) {
            const std::size_t idx = static_cast<std::size_t>(index);
            if (idx < geometryObjects.size())
                proto->geometry = geometryObjects[idx];
        }
    }
    const QJsonArray tagArray = obj.value(QStringLiteral("tags")).toArray();
    proto->tags.reserve(tagArray.size());
    for (const auto& entry : tagArray) {
        const double value = entry.toDouble(0.0);
        if (value >= 0.0 && std::isfinite(value))
            proto->tags.push_back(static_cast<Document::TagId>(value));
    }
    const QJsonArray childrenArray = obj.value(QStringLiteral("children")).toArray();
    proto->children.reserve(childrenArray.size());
    for (const auto& childValue : childrenArray) {
        proto->children.push_back(prototypeFromJson(childValue.toObject(), geometryObjects));
    }
    return proto;
}

QJsonObject objectNodeToJson(const Document::ObjectNode& node,
                             const std::unordered_map<const GeometryObject*, std::size_t>& geometryLookup)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("id"), static_cast<double>(node.id));
    obj.insert(QStringLiteral("kind"), nodeKindToString(node.kind));
    obj.insert(QStringLiteral("name"), QString::fromStdString(node.name));
    obj.insert(QStringLiteral("visible"), node.visible);
    obj.insert(QStringLiteral("expanded"), node.expanded);
    if (node.definitionId != 0)
        obj.insert(QStringLiteral("definitionId"), static_cast<double>(node.definitionId));
    if (node.geometry) {
        auto it = geometryLookup.find(node.geometry);
        if (it != geometryLookup.end())
            obj.insert(QStringLiteral("geometry"), static_cast<double>(it->second));
    }
    QJsonArray tags;
    for (Document::TagId tagId : node.tags)
        tags.append(static_cast<double>(tagId));
    obj.insert(QStringLiteral("tags"), tags);

    QJsonArray children;
    children.reserve(static_cast<int>(node.children.size()));
    for (const auto& child : node.children)
        children.append(objectNodeToJson(*child, geometryLookup));
    obj.insert(QStringLiteral("children"), children);
    return obj;
}

std::unique_ptr<Document::ObjectNode> objectNodeFromJson(
    const QJsonObject& obj,
    const std::vector<GeometryObject*>& geometryObjects,
    std::uint64_t& maxObjectId)
{
    auto node = std::make_unique<Document::ObjectNode>();
    node->id = static_cast<Document::ObjectId>(obj.value(QStringLiteral("id")).toDouble(0.0));
    node->kind = nodeKindFromString(obj.value(QStringLiteral("kind")).toString());
    node->name = obj.value(QStringLiteral("name")).toString().toStdString();
    node->visible = obj.value(QStringLiteral("visible")).toBool(true);
    node->expanded = obj.value(QStringLiteral("expanded")).toBool(true);
    node->definitionId = static_cast<Document::ComponentDefinitionId>(
        obj.value(QStringLiteral("definitionId")).toDouble(0.0));
    const QJsonValue geometryValue = obj.value(QStringLiteral("geometry"));
    if (geometryValue.isDouble()) {
        const double index = geometryValue.toDouble(-1.0);
        if (index >= 0.0 && std::isfinite(index)) {
            const std::size_t idx = static_cast<std::size_t>(index);
            if (idx < geometryObjects.size())
                node->geometry = geometryObjects[idx];
        }
    }
    const QJsonArray tagArray = obj.value(QStringLiteral("tags")).toArray();
    node->tags.reserve(tagArray.size());
    for (const auto& entry : tagArray) {
        const double value = entry.toDouble(0.0);
        if (value >= 0.0 && std::isfinite(value))
            node->tags.push_back(static_cast<Document::TagId>(value));
    }
    maxObjectId = std::max<std::uint64_t>(maxObjectId, node->id);

    const QJsonArray childrenArray = obj.value(QStringLiteral("children")).toArray();
    node->children.reserve(childrenArray.size());
    for (const auto& childValue : childrenArray) {
        auto childNode = objectNodeFromJson(childValue.toObject(), geometryObjects, maxObjectId);
        childNode->parent = node.get();
        node->children.push_back(std::move(childNode));
    }
    return node;
}

QJsonObject importMetadataToJson(Document::ObjectId id, const Document::ImportMetadata& meta)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("objectId"), static_cast<double>(id));
    obj.insert(QStringLiteral("path"), QString::fromStdString(meta.sourcePath));
    obj.insert(QStringLiteral("format"), fileFormatToString(meta.format));
    QJsonArray slots;
    for (const auto& slot : meta.materialSlots)
        slots.append(QString::fromStdString(slot));
    obj.insert(QStringLiteral("materialSlots"), slots);
    if (meta.importedAt.time_since_epoch().count() != 0) {
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            meta.importedAt.time_since_epoch()).count();
        obj.insert(QStringLiteral("timestamp"), static_cast<double>(ms));
    }
    return obj;
}

Document::ImportMetadata importMetadataFromJson(const QJsonObject& obj)
{
    Document::ImportMetadata meta;
    meta.sourcePath = obj.value(QStringLiteral("path")).toString().toStdString();
    meta.format = fileFormatFromString(obj.value(QStringLiteral("format")).toString());
    const QJsonArray slots = obj.value(QStringLiteral("materialSlots")).toArray();
    meta.materialSlots.reserve(slots.size());
    for (const auto& entry : slots)
        meta.materialSlots.push_back(entry.toString().toStdString());
    const double ts = obj.value(QStringLiteral("timestamp")).toDouble(std::numeric_limits<double>::quiet_NaN());
    if (std::isfinite(ts)) {
        auto millis = std::chrono::milliseconds(static_cast<std::int64_t>(ts));
        meta.importedAt = std::chrono::system_clock::time_point(millis);
    }
    return meta;
}

QJsonObject imagePlaneToJson(Document::ObjectId id, const Document::ImagePlaneMetadata& meta)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("objectId"), static_cast<double>(id));
    obj.insert(QStringLiteral("path"), QString::fromStdString(meta.sourcePath));
    obj.insert(QStringLiteral("width"), meta.width);
    obj.insert(QStringLiteral("height"), meta.height);
    return obj;
}

Document::ImagePlaneMetadata imagePlaneFromJson(const QJsonObject& obj)
{
    Document::ImagePlaneMetadata meta;
    meta.sourcePath = obj.value(QStringLiteral("path")).toString().toStdString();
    meta.width = static_cast<float>(obj.value(QStringLiteral("width")).toDouble(meta.width));
    meta.height = static_cast<float>(obj.value(QStringLiteral("height")).toDouble(meta.height));
    return meta;
}

QJsonObject externalReferenceToJson(Document::ObjectId id, const Document::ExternalReferenceMetadata& meta)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("objectId"), static_cast<double>(id));
    obj.insert(QStringLiteral("path"), QString::fromStdString(meta.sourcePath));
    obj.insert(QStringLiteral("displayName"), QString::fromStdString(meta.displayName));
    if (meta.linkedAt.time_since_epoch().count() != 0) {
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            meta.linkedAt.time_since_epoch()).count();
        obj.insert(QStringLiteral("timestamp"), static_cast<double>(ms));
    }
    return obj;
}

Document::ExternalReferenceMetadata externalReferenceFromJson(const QJsonObject& obj)
{
    Document::ExternalReferenceMetadata meta;
    meta.sourcePath = obj.value(QStringLiteral("path")).toString().toStdString();
    meta.displayName = obj.value(QStringLiteral("displayName")).toString().toStdString();
    const double ts = obj.value(QStringLiteral("timestamp")).toDouble(std::numeric_limits<double>::quiet_NaN());
    if (std::isfinite(ts)) {
        auto millis = std::chrono::milliseconds(static_cast<std::int64_t>(ts));
        meta.linkedAt = std::chrono::system_clock::time_point(millis);
    }
    return meta;
}

} // namespace

SceneSerializer::Result SceneSerializer::Result::success()
{
    Result result;
    result.status = Status::Success;
    return result;
}

SceneSerializer::Result SceneSerializer::Result::formatMismatch()
{
    Result result;
    result.status = Status::FormatMismatch;
    return result;
}

SceneSerializer::Result SceneSerializer::Result::failure(const std::string& message)
{
    Result result;
    result.status = Status::Error;
    result.message = message;
    return result;
}

SceneSerializer::Result SceneSerializer::save(const Document& document, const std::string& path)
{
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file)
        return Result::failure("Unable to open scene file for writing");
    return saveToStream(document, file);
}

SceneSerializer::Result SceneSerializer::load(Document& document, const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return Result::failure("Unable to open scene file for reading");
    return loadFromStream(document, file);
}

SceneSerializer::Result SceneSerializer::saveToStream(const Document& document, std::ostream& stream)
{
    std::ostringstream geometryStream(std::ios::binary);
    document.geometryKernel.saveToStream(geometryStream);
    const std::string geometryPayload = geometryStream.str();

    std::unordered_map<const GeometryObject*, std::size_t> geometryLookup;
    const auto& objects = document.geometryKernel.getObjects();
    geometryLookup.reserve(objects.size());
    for (std::size_t i = 0; i < objects.size(); ++i)
        geometryLookup.emplace(objects[i].get(), i);

    QJsonObject root;
    root.insert(QStringLiteral("format"), QStringLiteral("FreeCrafterScene"));
    QJsonObject versionObj;
    versionObj.insert(QStringLiteral("major"), static_cast<int>(kSupportedMajorVersion));
    versionObj.insert(QStringLiteral("minor"), static_cast<int>(kSupportedMinorVersion));
    root.insert(QStringLiteral("version"), versionObj);

    QJsonObject docObj;
    docObj.insert(QStringLiteral("colorByTag"), document.colorByTagEnabled);
    docObj.insert(QStringLiteral("isolationStack"), [&]() {
        QJsonArray arr;
        for (auto id : document.isolationIds)
            arr.append(static_cast<double>(id));
        return arr;
    }());

    QJsonArray planesArray;
    for (const auto& plane : document.planes)
        planesArray.append(sectionPlaneToJson(plane));
    docObj.insert(QStringLiteral("sectionPlanes"), planesArray);
    docObj.insert(QStringLiteral("settings"), sceneSettingsToJson(document.sceneSettings));

    QJsonArray tagArray;
    for (const auto& [id, tag] : document.tagMap) {
        QJsonObject tagObj;
        tagObj.insert(QStringLiteral("id"), static_cast<double>(id));
        tagObj.insert(QStringLiteral("name"), QString::fromStdString(tag.name));
        tagObj.insert(QStringLiteral("color"), colorToJson(tag.color));
        tagObj.insert(QStringLiteral("visible"), tag.visible);
        tagArray.append(tagObj);
    }
    docObj.insert(QStringLiteral("tags"), tagArray);

    docObj.insert(QStringLiteral("objectTree"), objectNodeToJson(*document.rootNode, geometryLookup));

    QJsonArray materialsArray;
    const auto& materials = document.geometryKernel.getMaterials();
    for (const auto& [geom, name] : materials) {
        auto it = geometryLookup.find(geom);
        if (it == geometryLookup.end())
            continue;
        QJsonObject matObj;
        matObj.insert(QStringLiteral("geometry"), static_cast<double>(it->second));
        matObj.insert(QStringLiteral("name"), QString::fromStdString(name));
        materialsArray.append(matObj);
    }
    docObj.insert(QStringLiteral("materials"), materialsArray);

    QJsonArray componentArray;
    for (const auto& [id, definition] : document.componentDefinitions) {
        QJsonObject defObj;
        defObj.insert(QStringLiteral("id"), static_cast<double>(id));
        defObj.insert(QStringLiteral("name"), QString::fromStdString(definition.name));

        std::ostringstream defGeometryStream(std::ios::binary);
        definition.geometry.saveToStream(defGeometryStream);
        defObj.insert(QStringLiteral("geometry"), QString::fromStdString(defGeometryStream.str()));

        std::unordered_map<const GeometryObject*, std::size_t> defLookup;
        const auto& defObjects = definition.geometry.getObjects();
        defLookup.reserve(defObjects.size());
        for (std::size_t i = 0; i < defObjects.size(); ++i)
            defLookup.emplace(defObjects[i].get(), i);

        QJsonArray rootsArray;
        for (const auto& proto : definition.roots)
            rootsArray.append(prototypeToJson(*proto, defLookup));
        defObj.insert(QStringLiteral("prototypes"), rootsArray);
        componentArray.append(defObj);
    }
    docObj.insert(QStringLiteral("components"), componentArray);

    QJsonArray scenesArray;
    for (const auto& [id, scene] : document.sceneMap) {
        QJsonObject sceneObj;
        sceneObj.insert(QStringLiteral("id"), static_cast<double>(id));
        sceneObj.insert(QStringLiteral("name"), QString::fromStdString(scene.name));
        sceneObj.insert(QStringLiteral("camera"), cameraStateToJson(scene.camera));
        sceneObj.insert(QStringLiteral("settings"), sceneSettingsToJson(scene.settings));
        sceneObj.insert(QStringLiteral("colorByTag"), scene.colorByTag);
        QJsonObject visibility;
        for (const auto& [tagId, visible] : scene.tagVisibility)
            visibility.insert(QString::number(tagId), visible);
        sceneObj.insert(QStringLiteral("tagVisibility"), visibility);
        scenesArray.append(sceneObj);
    }
    docObj.insert(QStringLiteral("scenes"), scenesArray);

    QJsonArray importsArray;
    for (const auto& [objectId, meta] : document.importedProvenance)
        importsArray.append(importMetadataToJson(objectId, meta));
    docObj.insert(QStringLiteral("imports"), importsArray);

    QJsonArray imagePlaneArray;
    for (const auto& [objectId, meta] : document.imagePlaneMetadata)
        imagePlaneArray.append(imagePlaneToJson(objectId, meta));
    docObj.insert(QStringLiteral("imagePlanes"), imagePlaneArray);

    QJsonArray externalArray;
    for (const auto& [objectId, meta] : document.externalReferenceMetadata)
        externalArray.append(externalReferenceToJson(objectId, meta));
    docObj.insert(QStringLiteral("externalReferences"), externalArray);

    docObj.insert(QStringLiteral("nextObjectId"), static_cast<double>(document.nextObjectId));
    docObj.insert(QStringLiteral("nextTagId"), static_cast<double>(document.nextTagId));
    docObj.insert(QStringLiteral("nextDefinitionId"), static_cast<double>(document.nextDefinitionId));
    docObj.insert(QStringLiteral("nextSceneId"), static_cast<double>(document.nextSceneId));

    root.insert(QStringLiteral("document"), docObj);

    QJsonObject geometryDescriptor;
    geometryDescriptor.insert(QStringLiteral("encoding"), QStringLiteral("text/vnd.freecrafter.geometry"));
    geometryDescriptor.insert(QStringLiteral("byteLength"), static_cast<double>(geometryPayload.size()));
    root.insert(QStringLiteral("geometry"), geometryDescriptor);

    const QByteArray jsonData = QJsonDocument(root).toJson(QJsonDocument::Compact);

    Header header{};
    std::memcpy(header.magic, kMagic, sizeof(kMagic));
    header.majorVersion = kSupportedMajorVersion;
    header.minorVersion = kSupportedMinorVersion;
    header.jsonByteLength = static_cast<std::uint32_t>(jsonData.size());
    header.geometryByteLength = static_cast<std::uint32_t>(geometryPayload.size());

    static_assert(sizeof(Header) == 16, "SceneSerializer header must remain packed");

    stream.write(reinterpret_cast<const char*>(&header), sizeof(header));
    if (!stream)
        return Result::failure("Failed writing scene header");
    if (!jsonData.isEmpty()) {
        stream.write(jsonData.constData(), jsonData.size());
        if (!stream)
            return Result::failure("Failed writing scene metadata");
    }
    if (!geometryPayload.empty()) {
        stream.write(geometryPayload.data(), static_cast<std::streamsize>(geometryPayload.size()));
        if (!stream)
            return Result::failure("Failed writing scene geometry payload");
    }
    stream.flush();
    if (!stream)
        return Result::failure("Failed finalising scene file");
    return Result::success();
}

SceneSerializer::Result SceneSerializer::loadFromStream(Document& document, std::istream& stream)
{
    char magic[4];
    stream.read(magic, sizeof(magic));
    if (stream.gcount() != sizeof(magic))
        return Result::formatMismatch();
    if (std::memcmp(magic, kMagic, sizeof(magic)) != 0)
        return Result::formatMismatch();

    Header header{};
    std::memcpy(header.magic, magic, sizeof(magic));
    stream.read(reinterpret_cast<char*>(&header.majorVersion), sizeof(Header) - sizeof(magic));
    if (!stream)
        return Result::failure("Scene file header truncated");

    if (header.majorVersion > kSupportedMajorVersion) {
        return Result::failure("Scene file was written by a newer version of FreeCrafter");
    }

    std::string jsonBuffer;
    jsonBuffer.resize(header.jsonByteLength);
    if (header.jsonByteLength > 0) {
        stream.read(jsonBuffer.data(), header.jsonByteLength);
        if (!stream)
            return Result::failure("Scene metadata truncated");
    }

    std::string geometryBuffer;
    geometryBuffer.resize(header.geometryByteLength);
    if (header.geometryByteLength > 0) {
        stream.read(geometryBuffer.data(), header.geometryByteLength);
        if (!stream)
            return Result::failure("Scene geometry payload truncated");
    }

    QJsonParseError parseError{};
    QJsonDocument docJson = QJsonDocument::fromJson(QByteArray::fromRawData(jsonBuffer.data(), jsonBuffer.size()), &parseError);
    if (parseError.error != QJsonParseError::NoError || docJson.isNull())
        return Result::failure("Scene metadata could not be parsed as JSON");

    const QJsonObject root = docJson.object();
    if (root.value(QStringLiteral("format")) != QStringLiteral("FreeCrafterScene"))
        return Result::failure("Scene file format marker missing");

    const QJsonObject versionObj = root.value(QStringLiteral("version")).toObject();
    const int majorVersion = versionObj.value(QStringLiteral("major")).toInt(kSupportedMajorVersion);
    if (majorVersion > static_cast<int>(kSupportedMajorVersion))
        return Result::failure("Scene file uses an unsupported major version");

    const QJsonObject docObj = root.value(QStringLiteral("document")).toObject();

    document.resetInternal(true);

    if (!geometryBuffer.empty()) {
        std::istringstream geoStream(geometryBuffer);
        document.geometryKernel.loadFromStream(geoStream, std::string());
    }

    document.colorByTagEnabled = docObj.value(QStringLiteral("colorByTag")).toBool(false);

    const QJsonArray isolationArray = docObj.value(QStringLiteral("isolationStack")).toArray();
    document.isolationIds.clear();
    document.isolationIds.reserve(isolationArray.size());
    for (const auto& value : isolationArray) {
        const double raw = value.toDouble(0.0);
        if (raw >= 0.0 && std::isfinite(raw))
            document.isolationIds.push_back(static_cast<Document::ObjectId>(raw));
    }

    document.planes.clear();
    const QJsonArray planesArray = docObj.value(QStringLiteral("sectionPlanes")).toArray();
    document.planes.reserve(planesArray.size());
    for (const auto& entry : planesArray)
        document.planes.push_back(sectionPlaneFromJson(entry.toObject()));

    document.sceneSettings.reset();
    applySceneSettings(docObj.value(QStringLiteral("settings")).toObject(), document.sceneSettings);

    document.tagMap.clear();
    std::uint64_t maxTagId = 0;
    const QJsonArray tagArray = docObj.value(QStringLiteral("tags")).toArray();
    for (const auto& entry : tagArray) {
        const QJsonObject tagObj = entry.toObject();
        Document::Tag tag;
        tag.id = static_cast<Document::TagId>(tagObj.value(QStringLiteral("id")).toDouble(0.0));
        tag.name = tagObj.value(QStringLiteral("name")).toString().toStdString();
        tag.color = colorFromJson(tagObj.value(QStringLiteral("color")).toObject());
        tag.visible = tagObj.value(QStringLiteral("visible")).toBool(true);
        maxTagId = std::max<std::uint64_t>(maxTagId, tag.id);
        document.tagMap.emplace(tag.id, tag);
    }
    document.nextTagId = static_cast<Document::TagId>(docObj.value(QStringLiteral("nextTagId")).toDouble(maxTagId + 1));
    if (document.nextTagId <= maxTagId)
        document.nextTagId = maxTagId + 1;

    std::vector<GeometryObject*> geometryObjects;
    const auto& objects = document.geometryKernel.getObjects();
    geometryObjects.reserve(objects.size());
    for (const auto& up : objects)
        geometryObjects.push_back(up.get());

    document.rootNode = std::make_unique<Document::ObjectNode>();
    document.rootNode->id = 0;
    document.rootNode->kind = Document::NodeKind::Root;
    document.rootNode->name = "Root";
    document.rootNode->visible = true;
    document.rootNode->expanded = true;
    document.rootNode->parent = nullptr;
    document.rootNode->geometry = nullptr;
    document.rootNode->tags.clear();
    document.registerNode(document.rootNode.get());

    std::uint64_t maxObjectId = 0;
    const QJsonObject tree = docObj.value(QStringLiteral("objectTree")).toObject();
    document.rootNode->children.clear();
    const QJsonArray childrenArray = tree.value(QStringLiteral("children")).toArray();
    document.rootNode->children.reserve(childrenArray.size());
    for (const auto& entry : childrenArray) {
        auto child = objectNodeFromJson(entry.toObject(), geometryObjects, maxObjectId);
        child->parent = document.rootNode.get();
        document.rootNode->children.push_back(std::move(child));
    }

    document.nextObjectId = static_cast<Document::ObjectId>(docObj.value(QStringLiteral("nextObjectId")).toDouble(maxObjectId + 1));
    if (document.nextObjectId <= maxObjectId)
        document.nextObjectId = maxObjectId + 1;

    document.componentDefinitions.clear();
    std::uint64_t maxDefinitionId = 0;
    const QJsonArray componentArray = docObj.value(QStringLiteral("components")).toArray();
    for (const auto& entry : componentArray) {
        const QJsonObject defObj = entry.toObject();
        Document::ComponentDefinition definition;
        definition.id = static_cast<Document::ComponentDefinitionId>(
            defObj.value(QStringLiteral("id")).toDouble(0.0));
        definition.name = defObj.value(QStringLiteral("name")).toString().toStdString();
        const std::string geometryText = defObj.value(QStringLiteral("geometry")).toString().toStdString();
        if (!geometryText.empty()) {
            std::istringstream defStream(geometryText);
            definition.geometry.loadFromStream(defStream, std::string());
        }
        std::vector<GeometryObject*> defGeometry;
        const auto& defObjects = definition.geometry.getObjects();
        defGeometry.reserve(defObjects.size());
        for (const auto& up : defObjects)
            defGeometry.push_back(up.get());
        const QJsonArray roots = defObj.value(QStringLiteral("prototypes")).toArray();
        for (const auto& protoValue : roots)
            definition.roots.push_back(prototypeFromJson(protoValue.toObject(), defGeometry));
        maxDefinitionId = std::max<std::uint64_t>(maxDefinitionId, definition.id);
        document.componentDefinitions.emplace(definition.id, std::move(definition));
    }
    document.nextDefinitionId = static_cast<Document::ComponentDefinitionId>(
        docObj.value(QStringLiteral("nextDefinitionId")).toDouble(maxDefinitionId + 1));
    if (document.nextDefinitionId <= maxDefinitionId)
        document.nextDefinitionId = maxDefinitionId + 1;

    document.sceneMap.clear();
    std::uint64_t maxSceneId = 0;
    const QJsonArray scenesArray = docObj.value(QStringLiteral("scenes")).toArray();
    for (const auto& entry : scenesArray) {
        const QJsonObject sceneObj = entry.toObject();
        Document::SceneState state;
        state.id = static_cast<Document::SceneId>(sceneObj.value(QStringLiteral("id")).toDouble(0.0));
        state.name = sceneObj.value(QStringLiteral("name")).toString().toStdString();
        state.camera = cameraStateFromJson(sceneObj.value(QStringLiteral("camera")).toObject());
        state.settings.reset();
        applySceneSettings(sceneObj.value(QStringLiteral("settings")).toObject(), state.settings);
        state.colorByTag = sceneObj.value(QStringLiteral("colorByTag")).toBool(false);
        const QJsonObject visibility = sceneObj.value(QStringLiteral("tagVisibility")).toObject();
        for (auto it = visibility.begin(); it != visibility.end(); ++it) {
            bool ok = false;
            const double value = it.value().toDouble();
            const std::uint64_t id = it.key().toULongLong(&ok);
            if (ok)
                state.tagVisibility[static_cast<Document::TagId>(id)] = (value != 0.0);
        }
        maxSceneId = std::max<std::uint64_t>(maxSceneId, state.id);
        document.sceneMap.emplace(state.id, std::move(state));
    }
    document.nextSceneId = static_cast<Document::SceneId>(docObj.value(QStringLiteral("nextSceneId")).toDouble(maxSceneId + 1));
    if (document.nextSceneId <= maxSceneId)
        document.nextSceneId = maxSceneId + 1;

    const QJsonArray materialsArray = docObj.value(QStringLiteral("materials")).toArray();
    for (const auto& entry : materialsArray) {
        const QJsonObject matObj = entry.toObject();
        const double index = matObj.value(QStringLiteral("geometry")).toDouble(-1.0);
        if (index < 0.0 || !std::isfinite(index))
            continue;
        const std::size_t idx = static_cast<std::size_t>(index);
        if (idx >= geometryObjects.size())
            continue;
        const std::string name = matObj.value(QStringLiteral("name")).toString().toStdString();
        document.geometryKernel.assignMaterial(geometryObjects[idx], name);
    }

    document.importedProvenance.clear();
    const QJsonArray importsArray = docObj.value(QStringLiteral("imports")).toArray();
    for (const auto& entry : importsArray) {
        const QJsonObject importObj = entry.toObject();
        const double idValue = importObj.value(QStringLiteral("objectId")).toDouble(-1.0);
        if (idValue < 0.0 || !std::isfinite(idValue))
            continue;
        const Document::ObjectId objectId = static_cast<Document::ObjectId>(idValue);
        document.importedProvenance[objectId] = importMetadataFromJson(importObj);
    }

    document.imagePlaneMetadata.clear();
    const QJsonArray imageArray = docObj.value(QStringLiteral("imagePlanes")).toArray();
    for (const auto& entry : imageArray) {
        const QJsonObject imgObj = entry.toObject();
        const double idValue = imgObj.value(QStringLiteral("objectId")).toDouble(-1.0);
        if (idValue < 0.0 || !std::isfinite(idValue))
            continue;
        const Document::ObjectId objectId = static_cast<Document::ObjectId>(idValue);
        document.imagePlaneMetadata[objectId] = imagePlaneFromJson(imgObj);
    }

    document.externalReferenceMetadata.clear();
    const QJsonArray externalArray = docObj.value(QStringLiteral("externalReferences")).toArray();
    for (const auto& entry : externalArray) {
        const QJsonObject extObj = entry.toObject();
        const double idValue = extObj.value(QStringLiteral("objectId")).toDouble(-1.0);
        if (idValue < 0.0 || !std::isfinite(idValue))
            continue;
        const Document::ObjectId objectId = static_cast<Document::ObjectId>(idValue);
        document.externalReferenceMetadata[objectId] = externalReferenceFromJson(extObj);
    }

    document.rebuildIndices();
    document.updateVisibility();
    return Result::success();
}

} // namespace Scene

