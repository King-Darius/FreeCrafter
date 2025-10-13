#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

#include <QHash>
#include <QString>

class Tool;
class CameraController;
class GeometryKernel;

namespace Scene {
class Document;
}

class ToolRegistry {
public:
    struct ToolCreationContext {
        GeometryKernel* geometry = nullptr;
        CameraController* camera = nullptr;
        Scene::Document* document = nullptr;
    };

    using ToolFactory = std::function<std::unique_ptr<Tool>(const ToolCreationContext&)>;

    enum class ToolId {
        SmartSelect,
        Line,
        Rectangle,
        Arc,
        CenterArc,
        TangentArc,
        Circle,
        Polygon,
        RotatedRectangle,
        Freehand,
        Bezier,
        Move,
        Rotate,
        Scale,
        Offset,
        PushPull,
        FollowMe,
        PaintBucket,
        Text,
        Dimension,
        TapeMeasure,
        Protractor,
        Axes,
        Extrude,
        Chamfer,
        Loft,
        Section,
        Orbit,
        Pan,
        Zoom
    };

    struct ToolDescriptor {
        ToolId key;
        const char* idLiteral = nullptr;
        QString id;
        QString iconPath;
        const char* labelKey = nullptr;
        const char* statusTipKey = nullptr;
        const char* hintKey = nullptr;
        ToolFactory factory;
        bool navigation = false;

        ToolDescriptor(ToolId key,
            const char* idLiteral,
            const char* iconLiteral,
            const char* labelKey,
            const char* statusTipKey,
            const char* hintKey,
            ToolFactory factory,
            bool navigation);
    };

    static const ToolRegistry& instance();

    const ToolDescriptor& descriptor(ToolId key) const;
    const ToolDescriptor* findById(const QString& id) const;

    const std::vector<ToolDescriptor>& allTools() const { return entries; }

private:
    ToolRegistry();

    std::vector<ToolDescriptor> entries;
    QHash<QString, std::size_t> indexById;
};
