#include "SceneCommands.h"

#include "Scene/Document.h"

#include <QFileInfo>
#include <QObject>
#include <QString>

#include <algorithm>
#include <chrono>

namespace Scene {
namespace {
std::string fallbackName(const PrimitiveOptions& options)
{
    if (!options.name.empty())
        return options.name;
    switch (options.type) {
    case PrimitiveType::Box:
        return "Box";
    case PrimitiveType::Plane:
        return "Plane";
    case PrimitiveType::Cylinder:
        return "Cylinder";
    case PrimitiveType::Sphere:
        return "Sphere";
    }
    return "Primitive";
}

std::string stemFromPath(const std::string& path)
{
    QFileInfo info(QString::fromStdString(path));
    if (!info.fileName().isEmpty())
        return info.fileName().toStdString();
    return path;
}

void applyGuideState(Document& document, const GeometryKernel::GuideState& state)
{
    GeometryKernel& kernel = document.geometry();
    kernel.clearGuides();
    for (const auto& line : state.lines) {
        kernel.addGuideLine(line.start, line.end);
    }
    for (const auto& point : state.points) {
        kernel.addGuidePoint(point.position);
    }
    for (const auto& angle : state.angles) {
        kernel.addGuideAngle(angle.origin, angle.startDirection, angle.endDirection);
    }
}

}

AddPrimitiveCommand::AddPrimitiveCommand(Document& document, PrimitiveOptions opts, const QString& text)
    : QUndoCommand(text.isEmpty() ? QObject::tr("Insert %1").arg(QString::fromStdString(fallbackName(opts))) : text)
    , document(document)
    , options(std::move(opts))
{
}

void AddPrimitiveCommand::redo()
{
    std::string error;
    auto object = buildPrimitiveGeometry(options, &error);
    if (!object) {
        lastError = error;
        setObsolete(true);
        return;
    }

    GeometryObject* added = document.geometry().addObject(std::move(object));
    if (!added) {
        lastError = "Failed to add geometry to document";
        setObsolete(true);
        return;
    }

    currentObjectId = document.ensureObjectForGeometry(added, fallbackName(options));
    lastError.clear();
}

void AddPrimitiveCommand::undo()
{
    if (currentObjectId != 0) {
        document.removeObject(currentObjectId);
        currentObjectId = 0;
    }
}

SetGuidesCommand::SetGuidesCommand(Document& document, const GeometryKernel::GuideState& nextState, const QString& text)
    : QUndoCommand(text.isEmpty() ? QObject::tr("Update Guides") : text)
    , document(document)
    , newState(nextState)
{
}

void SetGuidesCommand::redo()
{
    if (!initialized) {
        previousState = document.geometry().getGuides();
        initialized = true;
    }
    applyGuideState(document, newState);
}

void SetGuidesCommand::undo()
{
    applyGuideState(document, previousState);
}

void SetGuidesCommand::applyState(Document& document, const GeometryKernel::GuideState& state)
{
    applyGuideState(document, state);
}

AddImagePlaneCommand::AddImagePlaneCommand(Document& document, ImagePlaneOptions opts, const QString& text)
    : QUndoCommand(text.isEmpty() ? QObject::tr("Insert Image") : text)
    , document(document)
    , options(std::move(opts))
{
}

void AddImagePlaneCommand::redo()
{
    std::string error;
    auto object = buildPlaneGeometry(options.width, options.height, 0.01f, options.center, &error);
    if (!object) {
        lastError = error;
        setObsolete(true);
        return;
    }

    GeometryObject* added = document.geometry().addObject(std::move(object));
    if (!added) {
        lastError = "Failed to add image plane";
        setObsolete(true);
        return;
    }

    PrimitiveOptions naming;
    naming.type = PrimitiveType::Plane;
    naming.name = stemFromPath(options.filePath);
    currentObjectId = document.ensureObjectForGeometry(added, naming.name);

    Document::ImagePlaneMetadata metadata;
    metadata.sourcePath = options.filePath;
    metadata.width = options.width;
    metadata.height = options.height;
    document.setImagePlaneMetadata(currentObjectId, metadata);
    lastError.clear();
}

void AddImagePlaneCommand::undo()
{
    if (currentObjectId != 0) {
        document.clearImagePlaneMetadata(currentObjectId);
        document.removeObject(currentObjectId);
        currentObjectId = 0;
    }
}

LinkExternalReferenceCommand::LinkExternalReferenceCommand(Document& document, ExternalReferenceOptions opts, const QString& text)
    : QUndoCommand(text.isEmpty() ? QObject::tr("Link External Reference") : text)
    , document(document)
    , options(std::move(opts))
{
}

void LinkExternalReferenceCommand::redo()
{
    PrimitiveOptions primitive;
    primitive.type = PrimitiveType::Box;
    primitive.width = std::max(0.001f, options.width);
    primitive.depth = std::max(0.001f, options.depth);
    primitive.height = std::max(0.001f, options.height);
    primitive.center = options.center;
    primitive.name = options.displayName.empty() ? stemFromPath(options.filePath) : options.displayName;

    std::string error;
    auto object = buildPrimitiveGeometry(primitive, &error);
    if (!object) {
        lastError = error;
        setObsolete(true);
        return;
    }

    GeometryObject* added = document.geometry().addObject(std::move(object));
    if (!added) {
        lastError = "Failed to add reference placeholder";
        setObsolete(true);
        return;
    }

    currentObjectId = document.ensureObjectForGeometry(added, primitive.name);
    Document::ExternalReferenceMetadata metadata;
    metadata.sourcePath = options.filePath;
    metadata.displayName = primitive.name;
    metadata.linkedAt = std::chrono::system_clock::now();
    document.setExternalReferenceMetadata(currentObjectId, metadata);
    lastError.clear();
}

void LinkExternalReferenceCommand::undo()
{
    if (currentObjectId != 0) {
        document.clearExternalReferenceMetadata(currentObjectId);
        document.removeObject(currentObjectId);
        currentObjectId = 0;
    }
}

} // namespace Scene

