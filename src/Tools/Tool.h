#pragma once
#include "../GeometryKernel/GeometryKernel.h"
#include "../GeometryKernel/Vector3.h"
#include "../CameraController.h"
#include "../Interaction/InferenceEngine.h"

#include <algorithm>
#include <vector>

class Tool {
public:
    struct ModifierState {
        bool shift = false;
        bool ctrl = false;
        bool alt = false;

        bool operator==(const ModifierState& other) const
        {
            return shift == other.shift && ctrl == other.ctrl && alt == other.alt;
        }

        bool operator!=(const ModifierState& other) const { return !(*this == other); }
    };

    struct PointerInput {
        int x = 0;
        int y = 0;
        ModifierState modifiers;
        float devicePixelRatio = 1.0f;
    };

    enum class State {
        Idle,
        Armed,
        Active
    };

    struct PreviewPolyline {
        std::vector<Vector3> points;
        bool closed = false;
    };

    struct PreviewGhost {
        const GeometryObject* object = nullptr;
        Vector3 translation{ 0.0f, 0.0f, 0.0f };
        Vector3 scale{ 1.0f, 1.0f, 1.0f };
        Vector3 rotationAxis{ 0.0f, 1.0f, 0.0f };
        float rotationAngle = 0.0f;
        Vector3 pivot{ 0.0f, 0.0f, 0.0f };
        bool usePivot = false;
    };

    struct PreviewState {
        std::vector<PreviewPolyline> polylines;
        std::vector<PreviewGhost> ghosts;
    };

    enum class MeasurementKind {
        None,
        Distance,
        Angle,
        Scale
    };

    enum class OverrideResult {
        Ignored,
        PreviewUpdated,
        Commit
    };

    Tool(GeometryKernel* g, CameraController* c)
        : geometry(g)
        , camera(c)
    {
    }

    virtual ~Tool() = default;

    void setViewportSize(int w, int h)
    {
        viewportWidth = std::max(1, w);
        viewportHeight = std::max(1, h);
    }

    const Interaction::InferenceResult& getInferenceResult() const { return currentInference; }
    void setInferenceResult(const Interaction::InferenceResult& result)
    {
        currentInference = result;
        onInferenceResultChanged(currentInference);
    }

    State getState() const { return state; }

    void handleMouseDown(const PointerInput& input);
    void handleMouseMove(const PointerInput& input);
    void handleMouseUp(const PointerInput& input);
    void handleHover(const PointerInput& input);
    void handleKeyPress(int key);
    void handleKeyRelease(int key);
    void commit();
    void cancel();
    void setModifiers(const ModifierState& modifiers);

    PreviewState getPreviewState() const { return buildPreview(); }

    virtual const char* getName() const = 0;
    virtual MeasurementKind getMeasurementKind() const { return MeasurementKind::None; }
    virtual OverrideResult applyMeasurementOverride(double) { return OverrideResult::Ignored; }
    virtual bool isNavigationTool() const { return false; }

protected:
    virtual void onPointerDown(const PointerInput&) {}
    virtual void onPointerMove(const PointerInput&) {}
    virtual void onPointerUp(const PointerInput&) {}
    virtual void onPointerHover(const PointerInput&) {}
    virtual void onKeyDown(int) {}
    virtual void onKeyUp(int) {}
    virtual void onCommit() {}
    virtual void onCancel() {}
    virtual void onModifiersChanged(const ModifierState&) {}
    virtual void onStateChanged(State, State) {}
    virtual void onInferenceResultChanged(const Interaction::InferenceResult&) {}
    virtual PreviewState buildPreview() const { return {}; }

    void setState(State newState);
    const ModifierState& getModifiers() const { return modifiers; }

    int viewportWidth = 1;
    int viewportHeight = 1;
    GeometryKernel* geometry;
    CameraController* camera;

private:
    void updateModifiers(const ModifierState& modifiers);

    Interaction::InferenceResult currentInference;
    ModifierState modifiers;
    State state = State::Idle;
};

class PointerDragTool : public Tool {
public:
    using Tool::Tool;

protected:
    void onPointerDown(const PointerInput& input) override;
    void onPointerMove(const PointerInput& input) override;
    void onPointerUp(const PointerInput& input) override;
    void onCancel() override;

    bool hasActiveDrag() const { return dragState.isActive(); }
    float getPixelScale() const { return dragState.getPixelScale(); }

    virtual void onDragStart(const PointerInput&) {}
    virtual void onDragUpdate(const PointerInput&, float, float) {}
    virtual void onDragEnd(const PointerInput&) {}
    virtual void onDragCanceled() {}

private:
    class DragState {
    public:
        void begin(const PointerInput& input);
        bool update(const PointerInput& input, float& dx, float& dy);
        bool finish(const PointerInput& input);
        bool cancel();
        bool isActive() const { return dragging; }
        float getPixelScale() const { return pixelScale; }

    private:
        bool dragging = false;
        int lastX = 0;
        int lastY = 0;
        float pixelScale = 1.0f;
    };

    DragState dragState;
};
