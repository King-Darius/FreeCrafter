#pragma once

#include "Core/Command.h"
#include "GeometryKernel/Vector3.h"
#include "Phase6/AdvancedModeling.h"

#include <memory>
#include <optional>
#include <QString>
#include <string>
#include <vector>

class GeometryObject;

namespace Tools {

class CreateCurveCommand : public Core::Command {
public:
    CreateCurveCommand(std::vector<Vector3> points, const QString& description,
                       std::optional<GeometryKernel::ShapeMetadata> metadata = std::nullopt,
                       std::string name = {});

protected:
    void performRedo() override;
    void performUndo() override;

private:
    std::vector<Vector3> points_;
    std::optional<GeometryKernel::ShapeMetadata> metadata_;
    std::string name_;
    Scene::Document::ObjectId createdId_ = 0;
};

class ExtrudeProfileCommand : public Core::Command {
public:
    ExtrudeProfileCommand(Scene::Document::ObjectId profileId,
                          std::optional<Scene::Document::ObjectId> pathId,
                          Vector3 direction, bool capStart, bool capEnd,
                          const QString& description, std::string name = {});

protected:
    void performRedo() override;
    void performUndo() override;

private:
    Scene::Document::ObjectId profileId_ = 0;
    std::optional<Scene::Document::ObjectId> pathId_;
    Vector3 direction_;
    bool capStart_ = true;
    bool capEnd_ = true;
    std::string name_;
    Scene::Document::ObjectId createdId_ = 0;
};

class TranslateObjectsCommand : public Core::Command {
public:
    TranslateObjectsCommand(std::vector<Scene::Document::ObjectId> ids, Vector3 delta, const QString& description);

protected:
    void performRedo() override;
    void performUndo() override;

private:
    std::vector<Scene::Document::ObjectId> ids_;
    Vector3 delta_;
};

class RotateObjectsCommand : public Core::Command {
public:
    RotateObjectsCommand(std::vector<Scene::Document::ObjectId> ids, Vector3 pivot, Vector3 axis, float angleRadians,
                         const QString& description);

protected:
    void performRedo() override;
    void performUndo() override;

private:
    std::vector<Scene::Document::ObjectId> ids_;
    Vector3 pivot_;
    Vector3 axis_;
    float angleRadians_ = 0.0f;
};

class ScaleObjectsCommand : public Core::Command {
public:
    ScaleObjectsCommand(std::vector<Scene::Document::ObjectId> ids, Vector3 pivot, Vector3 factors,
                        const QString& description);

protected:
    void performRedo() override;
    void performUndo() override;

private:
    std::vector<Scene::Document::ObjectId> ids_;
    Vector3 pivot_;
    Vector3 factors_;
};

class DeleteObjectsCommand : public Core::Command {
public:
    DeleteObjectsCommand(std::vector<Scene::Document::ObjectId> ids, const QString& description);

protected:
    void initialize() override;
    void performRedo() override;
    void performUndo() override;

private:
    struct Entry {
        Scene::Document::ObjectId originalId = 0;
        Scene::Document::ObjectId currentId = 0;
        std::unique_ptr<GeometryObject> prototype;
        std::optional<std::string> material;
        std::optional<GeometryKernel::ShapeMetadata> metadata;
        std::string name;
        std::vector<Scene::Document::TagId> tags;
        Scene::Document::ObjectId parentId = 0;
        std::size_t childIndex = 0;
    };

    std::vector<Scene::Document::ObjectId> requestedIds_;
    std::vector<Entry> entries_;
};

class ApplyChamferCommand : public Core::Command {
public:
    ApplyChamferCommand(Scene::Document::ObjectId curveId, Phase6::RoundCornerOptions options, const QString& description);

protected:
    void initialize() override;
    void performRedo() override;
    void performUndo() override;

private:
    Scene::Document::ObjectId curveId_ = 0;
    Phase6::RoundCornerOptions options_;
    std::vector<Vector3> originalLoop_;
    std::vector<bool> originalHardness_;
    bool captured_ = false;
};

class CreateLoftCommand : public Core::Command {
public:
    CreateLoftCommand(Scene::Document::ObjectId startId, Scene::Document::ObjectId endId,
                      Phase6::LoftOptions options, const QString& description, std::string name = {});

protected:
    void performRedo() override;
    void performUndo() override;

private:
    Scene::Document::ObjectId startId_ = 0;
    Scene::Document::ObjectId endId_ = 0;
    Phase6::LoftOptions options_;
    std::string name_;
    Scene::Document::ObjectId createdId_ = 0;
};

} // namespace Tools

