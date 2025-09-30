#include "SketchTool.h"

#include <cmath>

#include "../Interaction/InferenceEngine.h"

namespace {
bool screenToGround(CameraController* cam, int x, int y, int viewportW, int viewportH, Vector3& out)
{
    if (viewportW <= 0 || viewportH <= 0) return false;
    float cx, cy, cz; cam->getCameraPosition(cx, cy, cz);
    float yaw = cam->getYaw();
    float pitch = cam->getPitch();
    float ry = yaw * static_cast<float>(M_PI) / 180.0f;
    float rp = pitch * static_cast<float>(M_PI) / 180.0f;
    Vector3 f(-sinf(ry) * cosf(rp), -sinf(rp), -cosf(ry) * cosf(rp)); f = f.normalized();
    Vector3 up(0, 1, 0);
    Vector3 r = f.cross(up).normalized();
    up = r.cross(f).normalized();
    const float fov = 60.0f;
    float aspect = static_cast<float>(viewportW) / static_cast<float>(viewportH);
    float nx = (2.0f * x / viewportW) - 1.0f;
    float ny = 1.0f - (2.0f * y / viewportH);
    float th = tanf((fov * static_cast<float>(M_PI) / 180.0f) / 2.0f);
    Vector3 dir = (f + r * (nx * th * aspect) + up * (ny * th)).normalized();
    Vector3 origin(cx, cy, cz);
    if (std::fabs(dir.y) < 1e-6f) return false;
    float t = -origin.y / dir.y;
    if (t < 0.0f) return false;
    out = origin + dir * t;
    return true;
}

void axisSnap(Vector3& point)
{
    const float grid = 0.25f;
    const float eps = 0.08f;
    float gx = std::round(point.x / grid) * grid;
    float gz = std::round(point.z / grid) * grid;
    if (std::fabs(point.x - gx) < eps) point.x = gx;
    if (std::fabs(point.z - gz) < eps) point.z = gz;
}
}

bool SketchTool::resolveFallback(int x, int y, Vector3& out) const
{
    Vector3 ground;
    if (!screenToGround(camera, x, y, viewportWidth, viewportHeight, ground)) {
        return false;
    }
    axisSnap(ground);
    out = Vector3(ground.x, 0.0f, ground.z);
    return true;
}

bool SketchTool::resolvePoint(int x, int y, Vector3& out) const
{
    const auto& snap = getInferenceResult();
    if (snap.isValid()) {
        out = snap.position;
        return true;
    }
    return resolveFallback(x, y, out);
}

void SketchTool::onMouseDown(int x, int y)
{
    lastMouseX = x;
    lastMouseY = y;
    Vector3 point;
    if (!resolvePoint(x, y, point)) {
        previewValid = false;
        return;
    }

    if (!drawing) {
        drawing = true;
        pts.clear();
    }

    pts.push_back(point);
    previewPoint = point;
    previewValid = true;
}

void SketchTool::onMouseMove(int x, int y)
{
    lastMouseX = x;
    lastMouseY = y;
    Vector3 point;
    if (!resolvePoint(x, y, point)) {
        previewValid = false;
        return;
    }
    previewPoint = point;
    previewValid = true;
}

void SketchTool::onMouseUp(int, int)
{
    if (drawing && pts.size() >= 2) {
        geometry->addCurve(pts);
        drawing = false;
        pts.clear();
        previewValid = false;
    }
}

void SketchTool::onInferenceResultChanged(const Interaction::InferenceResult& result)
{
    if (!drawing) {
        if (result.isValid()) {
            previewPoint = result.position;
            previewValid = true;
        } else if (resolveFallback(lastMouseX, lastMouseY, previewPoint)) {
            previewValid = true;
        } else {
            previewValid = false;
        }
    }
}

