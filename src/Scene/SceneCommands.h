#pragma once

#include "PrimitiveBuilder.h"
#include "Scene/Document.h"

#include <QUndoCommand>
#include <QString>

#include <string>

namespace Scene {

class AddPrimitiveCommand : public QUndoCommand {
public:
    AddPrimitiveCommand(Document& document, PrimitiveOptions options, const QString& text = QString());

    ObjectId objectId() const { return currentObjectId; }
    const std::string& error() const { return lastError; }

    void redo() override;
    void undo() override;

private:
    Document& document;
    PrimitiveOptions options;
    ObjectId currentObjectId = 0;
    std::string lastError;
};

struct GuideStateChange {
    GeometryKernel::GuideState state;
};

class SetGuidesCommand : public QUndoCommand {
public:
    SetGuidesCommand(Document& document, const GeometryKernel::GuideState& nextState, const QString& text = QString());

    void redo() override;
    void undo() override;

private:
    static void applyState(Document& document, const GeometryKernel::GuideState& state);

    Document& document;
    GeometryKernel::GuideState newState;
    GeometryKernel::GuideState previousState;
    bool initialized = false;
};

struct ImagePlaneOptions {
    std::string filePath;
    float width = 1.0f;
    float height = 1.0f;
    Vector3 center{ 0.0f, 0.0f, 0.0f };
};

class AddImagePlaneCommand : public QUndoCommand {
public:
    AddImagePlaneCommand(Document& document, ImagePlaneOptions options, const QString& text = QString());

    ObjectId objectId() const { return currentObjectId; }
    const std::string& error() const { return lastError; }

    void redo() override;
    void undo() override;

private:
    Document& document;
    ImagePlaneOptions options;
    ObjectId currentObjectId = 0;
    std::string lastError;
};

struct ExternalReferenceOptions {
    std::string filePath;
    std::string displayName;
    float width = 1.0f;
    float depth = 1.0f;
    float height = 1.0f;
    Vector3 center{ 0.0f, 0.0f, 0.0f };
};

class LinkExternalReferenceCommand : public QUndoCommand {
public:
    LinkExternalReferenceCommand(Document& document, ExternalReferenceOptions options, const QString& text = QString());

    ObjectId objectId() const { return currentObjectId; }
    const std::string& error() const { return lastError; }

    void redo() override;
    void undo() override;

private:
    Document& document;
    ExternalReferenceOptions options;
    ObjectId currentObjectId = 0;
    std::string lastError;
};

} // namespace Scene

