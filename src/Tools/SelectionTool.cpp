#include "SelectionTool.h"

#include <QtCore/Qt>

#include <algorithm>
#include <cmath>
#include <limits>

#include "../GeometryKernel/Curve.h"
#include "../GeometryKernel/Solid.h"
#include "../GeometryKernel/HalfEdgeMesh.h"

namespace {
bool rayToGround(CameraController* cam, int sx, int sy, int viewportW, int viewportH, Vector3& out)
{
    if (viewportW <= 0 || viewportH <= 0) return false;
    float cx, cy, cz; cam->getCameraPosition(cx, cy, cz);
    float yaw = cam->getYaw();
    float pitch = cam->getPitch();
    float ry = yaw * static_cast<float>(M_PI) / 180.0f;
    float rp = pitch * static_cast<float>(M_PI) / 180.0f;
    Vector3 f(-sinf(ry) * cosf(rp), -sinf(rp), -cosf(ry) * cosf(rp)); f = f.normalized();
    Vector3 up(0, 1, 0); Vector3 r = f.cross(up).normalized(); up = r.cross(f).normalized();
    const float fov = 60.0f;
    float aspect = static_cast<float>(viewportW) / static_cast<float>(viewportH);
    float nx = (2.0f * sx / viewportW) - 1.0f;
    float ny = 1.0f - (2.0f * sy / viewportH);
    float th = tanf((fov * static_cast<float>(M_PI) / 180.0f) / 2.0f);
    Vector3 dir = (f + r * (nx * th * aspect) + up * (ny * th)).normalized();
    Vector3 origin(cx, cy, cz);
    if (std::fabs(dir.y) < 1e-6f) return false;
    float t = -origin.y / dir.y;
    if (t < 0.0f) return false;
    out = origin + dir * t;
    return true;
}
}

GeometryObject* SelectionTool::pickObjectAt(const Vector3& worldPoint)
{
    float bestDistance = std::numeric_limits<float>::max();
    GeometryObject* best = nullptr;

    const float solidBias = 0.85f;

    for (const auto& object : geometry->getObjects()) {
        if (object->getType() == ObjectType::Curve) {
            const Curve* curve = static_cast<const Curve*>(object.get());
            const auto& loop = curve->getBoundaryLoop();
            for (const auto& vertex : loop) {
                float d = (vertex - worldPoint).lengthSquared();
                if (d < bestDistance) {
                    bestDistance = d;
                    best = object.get();
                }
            }
        } else {
            const HalfEdgeMesh& mesh = object->getMesh();
            float localBest = std::numeric_limits<float>::max();
            for (const auto& vertex : mesh.getVertices()) {
                float d = (vertex.position - worldPoint).lengthSquared();
                if (d < localBest) {
                    localBest = d;
                }
            }
            if (localBest < std::numeric_limits<float>::max()) {
                float weighted = localBest * solidBias;
                if (weighted < bestDistance) {
                    bestDistance = weighted;
                    best = object.get();
                }
            }
        }
    }

    const float kSelectionRadius = 0.6f;
    if (!best || bestDistance > kSelectionRadius * kSelectionRadius) {
        return nullptr;
    }
    return best;
}

void SelectionTool::onMouseDown(int x, int y)
{
    Vector3 worldPoint;
    const auto& snap = getInferenceResult();
    if (snap.isValid()) {
        worldPoint = snap.position;
    } else if (!rayToGround(camera, x, y, viewportWidth, viewportHeight, worldPoint)) {
        return;
    }

    if (GeometryObject* candidate = pickObjectAt(worldPoint)) {
        if (lastSelected) {
            lastSelected->setSelected(false);
        }
        candidate->setSelected(true);
        lastSelected = candidate;
    }
}

void SelectionTool::onMouseUp(int, int)
{
}

void SelectionTool::onKeyPress(int key)
{
    if ((key == Qt::Key_Delete || key == Qt::Key_Backspace) && lastSelected) {
        geometry->deleteObject(lastSelected);
        lastSelected = nullptr;
    }
}

