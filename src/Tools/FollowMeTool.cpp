#include "FollowMeTool.h"

#include "ToolGeometryUtils.h"

#include "../GeometryKernel/Curve.h"
#include "../GeometryKernel/Solid.h"

#include <cmath>

FollowMeTool::FollowMeTool(GeometryKernel* geometry, CameraController* camera)
    : Tool(geometry, camera)
{
}

void FollowMeTool::onPointerDown(const PointerInput&)
{
    lastSelection = gatherSelectedCurves();
    if (lastSelection.size() < 2) {
        reset();
        return;
    }

    Curve* profile = lastSelection.front();
    Curve* path = lastSelection[1];
    applySweep(profile, path);
    reset();
}

Tool::PreviewState FollowMeTool::buildPreview() const
{
    PreviewState state;
    if (lastSelection.size() >= 2 && lastSelection[1]) {
        PreviewPolyline path;
        path.points = lastSelection[1]->getBoundaryLoop();
        path.closed = true;
        state.polylines.push_back(path);
    }
    return state;
}

std::vector<Curve*> FollowMeTool::gatherSelectedCurves() const
{
    std::vector<Curve*> curves;
    if (!geometry)
        return curves;
    for (const auto& object : geometry->getObjects()) {
        if (!object || !object->isSelected())
            continue;
        if (object->getType() != ObjectType::Curve)
            continue;
        curves.push_back(static_cast<Curve*>(object.get()));
    }
    return curves;
}

void FollowMeTool::applySweep(Curve* profile, Curve* path)
{
    if (!geometry || !profile || !path)
        return;

    const auto& pathLoop = path->getBoundaryLoop();
    if (pathLoop.size() < 2)
        return;

    Vector3 profileCentroid = computeCentroid(*profile);

    for (size_t i = 0; i < pathLoop.size(); ++i) {
        const Vector3& start = pathLoop[i];
        const Vector3& end = pathLoop[(i + 1) % pathLoop.size()];
        Vector3 delta = end - start;
        float length = std::sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
        if (length <= 1e-5f)
            continue;

        GeometryObject* created = geometry->extrudeCurve(profile, length);
        if (!created || created->getType() != ObjectType::Solid)
            continue;

        Vector3 translation = start - profileCentroid;
        translateObject(*created, translation);
    }
}

void FollowMeTool::reset()
{
    lastSelection.clear();
    setState(State::Idle);
}
