#pragma once

#include <QtCore/Qt>

#include <optional>
#include <string>
#include <vector>

struct NavigationConfig {
    struct DragBinding {
        Qt::MouseButton button = Qt::NoButton;
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        std::string toolName;
        bool temporary = true;

        bool operator==(const DragBinding& other) const
        {
            return button == other.button && modifiers == other.modifiers && toolName == other.toolName
                && temporary == other.temporary;
        }

        bool operator!=(const DragBinding& other) const { return !(*this == other); }
    };

    std::vector<DragBinding> dragBindings;
    std::string wheelToolName = "ZoomTool";
    bool zoomToCursor = true;
    bool invertWheel = false;
    float wheelStep = 1.0f;

    std::optional<DragBinding> matchDrag(Qt::MouseButton button, Qt::KeyboardModifiers modifiers) const;
};

inline std::optional<NavigationConfig::DragBinding> NavigationConfig::matchDrag(Qt::MouseButton button,
                                                                                Qt::KeyboardModifiers modifiers) const
{
    const Qt::KeyboardModifiers relevant = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;
    auto normalize = [relevant](Qt::KeyboardModifiers mods) {
        Qt::KeyboardModifiers normalized = mods & relevant;
        if (normalized.testFlag(Qt::MetaModifier))
            normalized |= Qt::ControlModifier;
        return normalized;
    };

    const Qt::KeyboardModifiers sanitized = normalize(modifiers);

    for (const auto& binding : dragBindings) {
        if (binding.button != button)
            continue;
        if (normalize(binding.modifiers) == sanitized)
            return binding;
    }
    return std::nullopt;
}
