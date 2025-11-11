#pragma once

#include "Core/Command.h"
#include "GeometryKernel/GeometryKernel.h"

#include <QString>
#include <string>
#include <unordered_map>
#include <vector>

namespace Scene {

class RenameObjectCommand : public Core::Command {
public:
    RenameObjectCommand(Document::ObjectId id, const QString& name);

protected:
    void initialize() override;
    void performRedo() override;
    void performUndo() override;

private:
    Document::ObjectId objectId = 0;
    QString newName;
    std::string previousName;
    bool captured = false;
};

class RenameObjectsCommand : public Core::Command {
public:
    RenameObjectsCommand(const std::vector<Document::ObjectId>& ids, const QString& name);

protected:
    void initialize() override;
    void performRedo() override;
    void performUndo() override;

private:
    std::vector<Document::ObjectId> objectIds;
    QString newName;
    std::vector<std::string> previousNames;
};

class SetObjectVisibilityCommand : public Core::Command {
public:
    SetObjectVisibilityCommand(Document::ObjectId id, bool visible);

protected:
    void initialize() override;
    void performRedo() override;
    void performUndo() override;

private:
    Document::ObjectId objectId = 0;
    bool visible = true;
    bool previous = true;
    bool captured = false;
};

class SetObjectsVisibilityCommand : public Core::Command {
public:
    SetObjectsVisibilityCommand(const std::vector<Document::ObjectId>& ids, bool visible);

protected:
    void initialize() override;
    void performRedo() override;
    void performUndo() override;

private:
    std::vector<Document::ObjectId> objectIds;
    bool newValue = true;
    std::vector<bool> previousValues;
};

class AssignMaterialCommand : public Core::Command {
public:
    AssignMaterialCommand(const std::vector<Document::ObjectId>& ids, const QString& materialName);

protected:
    void initialize() override;
    void performRedo() override;
    void performUndo() override;

private:
    std::vector<Document::ObjectId> objectIds;
    QString material;
    std::unordered_map<Document::ObjectId, std::string> previous;
};

class CreateTagCommand : public Core::Command {
public:
    CreateTagCommand(const QString& name, const SceneSettings::Color& color);

protected:
    void initialize() override;
    void performRedo() override;
    void performUndo() override;

private:
    QString name;
    SceneSettings::Color color;
    Document::Tag savedTag;
    std::vector<Document::ObjectId> savedAssignments;
    bool created = false;
};

class RenameTagCommand : public Core::Command {
public:
    RenameTagCommand(Document::TagId id, const QString& name);

protected:
    void initialize() override;
    void performRedo() override;
    void performUndo() override;

private:
    Document::TagId tagId = 0;
    QString newName;
    std::string previousName;
    bool captured = false;
};

class SetTagVisibilityCommand : public Core::Command {
public:
    SetTagVisibilityCommand(Document::TagId id, bool visible);

protected:
    void initialize() override;
    void performRedo() override;
    void performUndo() override;

private:
    Document::TagId tagId = 0;
    bool newValue = true;
    bool previousValue = true;
    bool captured = false;
};

class SetTagColorCommand : public Core::Command {
public:
    SetTagColorCommand(Document::TagId id, const SceneSettings::Color& color);

protected:
    void initialize() override;
    void performRedo() override;
    void performUndo() override;

private:
    Document::TagId tagId = 0;
    SceneSettings::Color newColor{};
    SceneSettings::Color previousColor{};
    bool captured = false;
};

class RebuildCurveFromMetadataCommand : public Core::Command {
public:
    RebuildCurveFromMetadataCommand(Document::ObjectId id, const GeometryKernel::ShapeMetadata& metadata);

    bool wasApplied() const { return applied; }

protected:
    void initialize() override;
    void performRedo() override;
    void performUndo() override;

private:
    Document::ObjectId objectId = 0;
    GeometryKernel::ShapeMetadata newMetadata{};
    GeometryKernel::ShapeMetadata previousMetadata{};
    bool captured = false;
    bool applied = false;
};

} // namespace Scene

