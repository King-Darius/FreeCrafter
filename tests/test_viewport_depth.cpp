#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QImage>
#include <QColor>
#include <QByteArray>

#include "GLViewport.h"
#include "Tools/ToolGeometryUtils.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace {

bool intersectRayAabb(const QVector3D& origin,
                      const QVector3D& direction,
                      const QVector3D& boxMin,
                      const QVector3D& boxMax,
                      float& enter,
                      float& exit)
{
    float tmin = 0.0f;
    float tmax = std::numeric_limits<float>::max();

    for (int axis = 0; axis < 3; ++axis) {
        const float o = origin[axis];
        const float d = direction[axis];
        const float minVal = boxMin[axis];
        const float maxVal = boxMax[axis];
        if (std::fabs(d) < 1e-6f) {
            if (o < minVal || o > maxVal)
                return false;
            continue;
        }
        const float invD = 1.0f / d;
        float t1 = (minVal - o) * invD;
        float t2 = (maxVal - o) * invD;
        if (t1 > t2)
            std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
        if (tmin > tmax)
            return false;
    }

    enter = tmin;
    exit = tmax;
    return true;
}

BoundingBox computeDocumentBounds(const GeometryKernel& geometry)
{
    BoundingBox combined;
    combined.valid = false;

    const auto& objects = geometry.getObjects();
    for (const auto& object : objects) {
        if (!object)
            continue;
        BoundingBox box = computeBoundingBox(*object);
        if (!box.valid)
            continue;
        if (!combined.valid) {
            combined = box;
            continue;
        }
        combined.min.x = std::min(combined.min.x, box.min.x);
        combined.min.y = std::min(combined.min.y, box.min.y);
        combined.min.z = std::min(combined.min.z, box.min.z);
        combined.max.x = std::max(combined.max.x, box.max.x);
        combined.max.y = std::max(combined.max.y, box.max.y);
        combined.max.z = std::max(combined.max.z, box.max.z);
    }

    return combined;
}

bool hasCoverageInRowRange(const QImage& image, int minRow, int maxRow)
{
    const int background = static_cast<int>(0.95f * 255.0f);
    const int threshold = 8;
    minRow = std::max(minRow, 0);
    maxRow = std::min(maxRow, image.height() - 1);
    if (minRow > maxRow)
        return false;
    for (int y = minRow; y <= maxRow; ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            if (std::abs(color.red() - background) > threshold ||
                std::abs(color.green() - background) > threshold ||
                std::abs(color.blue() - background) > threshold) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

int main(int argc, char** argv)
{
    if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));

    QApplication app(argc, argv);

    GLViewport viewport;
    viewport.resize(640, 480);
    viewport.show();

    CameraController* camera = viewport.getCamera();
    camera->setYawPitch(0.0f, 0.0f);
    camera->setTarget(0.0f, 0.0f, 0.0f);
    camera->setDistance(2.0f);

    GeometryKernel* geometry = viewport.getGeometry();
    std::vector<Vector3> tallBase {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.5f, 0.5f, 0.0f},
        {-0.5f, 0.5f, 0.0f},
        {-0.5f, -0.5f, 0.0f}
    };
    GeometryObject* tallCurve = geometry->addCurve(tallBase);
    if (!tallCurve)
        return 1;
    GeometryObject* tallSolid = geometry->extrudeCurveAlongVector(tallCurve, Vector3(0.0f, 0.0f, -200000.0f));
    if (!tallSolid)
        return 2;

    std::vector<Vector3> closeBase {
        {-0.1f, -0.1f, 0.1f},
        {0.1f, -0.1f, 0.1f},
        {0.1f, 0.1f, 0.1f},
        {-0.1f, 0.1f, 0.1f},
        {-0.1f, -0.1f, 0.1f}
    };
    GeometryObject* closeCurve = geometry->addCurve(closeBase);
    if (!closeCurve)
        return 3;
    GeometryObject* closeSolid = geometry->extrudeCurveAlongVector(closeCurve, Vector3(0.0f, 0.0f, -0.25f));
    if (!closeSolid)
        return 4;

    const float aspect = viewport.width() > 0 && viewport.height() > 0
        ? float(viewport.width()) / float(viewport.height())
        : 1.0f;
    const auto depthRange = viewport.depthRangeForAspect(aspect);
    const float znear = depthRange.first;
    const float zfar = depthRange.second;

    if (!(znear > 0.0f && zfar > znear))
        return 5;

    const float minFractionNear = std::max(camera->getDistance() * 0.001f, 1e-4f);

    BoundingBox bounds = computeDocumentBounds(*geometry);
    if (!bounds.valid)
        return 6;

    float cx, cy, cz;
    camera->getCameraPosition(cx, cy, cz);
    QVector3D cameraPos(cx, cy, cz);
    float tx, ty, tz;
    camera->getTarget(tx, ty, tz);
    QVector3D targetPos(tx, ty, tz);
    QVector3D viewDir = targetPos - cameraPos;
    const float viewLen = viewDir.length();
    if (viewLen > 1e-5f)
        viewDir /= viewLen;
    else
        viewDir = QVector3D(0.0f, 0.0f, -1.0f);

    std::array<QVector3D, 8> corners = {
        QVector3D(bounds.min.x, bounds.min.y, bounds.min.z),
        QVector3D(bounds.max.x, bounds.min.y, bounds.min.z),
        QVector3D(bounds.min.x, bounds.max.y, bounds.min.z),
        QVector3D(bounds.max.x, bounds.max.y, bounds.min.z),
        QVector3D(bounds.min.x, bounds.min.y, bounds.max.z),
        QVector3D(bounds.max.x, bounds.min.y, bounds.max.z),
        QVector3D(bounds.min.x, bounds.max.y, bounds.max.z),
        QVector3D(bounds.max.x, bounds.max.y, bounds.max.z)
    };

    float minProj = std::numeric_limits<float>::max();
    float maxProj = -std::numeric_limits<float>::max();
    for (const QVector3D& corner : corners) {
        const float proj = QVector3D::dotProduct(corner - cameraPos, viewDir);
        minProj = std::min(minProj, proj);
        maxProj = std::max(maxProj, proj);
    }

    const float frontDistance = std::max(minProj, 0.0f);
    if (frontDistance > minFractionNear + 1e-3f && znear + 1e-3f < minFractionNear)
        return 7;
    if (znear > frontDistance + 0.5f)
        return 8;
    if (zfar + 5.0f < maxProj)
        return 9;

    QPoint centerPoint(viewport.width() / 2, viewport.height() / 2);
    QVector3D rayOrigin;
    QVector3D rayDirection;
    if (!viewport.computePickRay(centerPoint, rayOrigin, rayDirection))
        return 10;

    const float originDistance = (rayOrigin - cameraPos).length();
    if (std::fabs(originDistance - znear) > 0.5f)
        return 11;

    float enter = 0.0f;
    float exit = 0.0f;
    if (!intersectRayAabb(rayOrigin, rayDirection,
                          QVector3D(bounds.min.x, bounds.min.y, bounds.min.z),
                          QVector3D(bounds.max.x, bounds.max.y, bounds.max.z),
                          enter, exit))
        return 12;

    if (originDistance + exit > zfar + 50.0f)
        return 13;

    const float ratio = zfar / std::max(znear, 1e-4f);
    if (ratio > 2.1e5f)
        return 16;

    viewport.update();
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 400) {
        app.processEvents(QEventLoop::AllEvents, 16);
    }

    QImage frame = viewport.grabFramebuffer();
    if (frame.isNull())
        return 14;

    bool topCoverage = hasCoverageInRowRange(frame, 0, frame.height() / 4);
    bool bottomCoverage = hasCoverageInRowRange(frame, frame.height() * 3 / 4, frame.height() - 1);
    if (!topCoverage || !bottomCoverage)
        return 15;

    return 0;
}
