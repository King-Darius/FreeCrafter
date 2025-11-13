#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QImage>
#include <QColor>
#include <QByteArray>
#include <QProcessEnvironment>

#include "GLViewport.h"
#include "Renderer.h"
#include "GeometryKernel/GeometryKernel.h"
#include "GeometryKernel/Vector3.h"
#include "Scene/SectionPlane.h"
#include "test_support.h"

#include <vector>
#include <cmath>
#include <algorithm>

namespace {

struct CaptureMetrics {
    int totalPixels = 0;
    int nonBackground = 0;
    double averageIntensity = 0.0;
    double nonBackgroundIntensity = 0.0;
    bool valid = false;
};

CaptureMetrics captureMetrics(GLViewport& viewport, QApplication& app)
{
    CaptureMetrics metrics;
    viewport.update();
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 500) {
        app.processEvents(QEventLoop::AllEvents, 16);
    }
    QImage image = viewport.grabFramebuffer();
    if (image.isNull()) {
        return metrics;
    }
    metrics.totalPixels = image.width() * image.height();
    if (metrics.totalPixels <= 0) {
        return metrics;
    }

    const int background = static_cast<int>(0.95f * 255.0f);
    const int threshold = 8;
    double sumIntensity = 0.0;
    double sumNonBackground = 0.0;
    int nonBackgroundCount = 0;

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            double intensity = (color.red() + color.green() + color.blue()) / (3.0 * 255.0);
            sumIntensity += intensity;
            if (std::abs(color.red() - background) > threshold ||
                std::abs(color.green() - background) > threshold ||
                std::abs(color.blue() - background) > threshold) {
                ++nonBackgroundCount;
                sumNonBackground += intensity;
            }
        }
    }

    metrics.nonBackground = nonBackgroundCount;
    metrics.averageIntensity = sumIntensity / metrics.totalPixels;
    metrics.nonBackgroundIntensity = nonBackgroundCount > 0 ? (sumNonBackground / nonBackgroundCount) : 0.0;
    metrics.valid = true;
    return metrics;
}

} // namespace

int main(int argc, char** argv)
{
    if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }
    QApplication app(argc, argv);

    if (!TestSupport::ensureOpenGL("render_regression"))
        return 0;

    GLViewport viewport;
    viewport.resize(400, 300);
    viewport.show();

    CameraController* camera = viewport.getCamera();
    if (!camera) {
        return 24;
    }

    camera->setTarget(180.0f, -240.0f, 80.0f);
    camera->setYawPitch(135.0f, -20.0f);
    camera->setDistance(800.0f);
    if (viewport.frameSceneToGeometry()) {
        return 25;
    }
    float tx = 0.0f;
    float ty = 0.0f;
    float tz = 0.0f;
    camera->getTarget(tx, ty, tz);
    if (std::fabs(tx) > 1e-3f || std::fabs(ty) > 1e-3f || std::fabs(tz) > 1e-3f) {
        return 26;
    }
    if (std::fabs(camera->getYaw() - 45.0f) > 1e-3f || std::fabs(camera->getPitch() - 35.0f) > 1e-3f) {
        return 27;
    }
    if (std::fabs(camera->getDistance() - 20.0f) > 1e-2f) {
        return 28;
    }

    GeometryKernel* geometry = viewport.getGeometry();
    std::vector<Vector3> base{
        {-0.5f, 0.0f, -0.5f},
        {0.5f, 0.0f, -0.5f},
        {0.5f, 0.0f, 0.5f},
        {-0.5f, 0.0f, 0.5f},
        {-0.5f, 0.0f, -0.5f}
    };
    GeometryObject* curveObj = geometry->addCurve(base);
    geometry->extrudeCurve(curveObj, 0.6f);

    const bool skipCoverage = qEnvironmentVariableIsSet("FREECRAFTER_RENDER_SKIP_COVERAGE");

    CaptureMetrics initialView = captureMetrics(viewport, app);
    if (!initialView.valid) {
        return 22;
    }

    if (!skipCoverage) {
        if (static_cast<double>(initialView.nonBackground) / std::max(1, initialView.totalPixels) <= 0.02) {
            return 23;
        }
    }

    viewport.setAutoFrameOnGeometryChange(false);

    camera->setTarget(150.0f, 150.0f, 150.0f);
    camera->setDistance(400.0f);
    CaptureMetrics lostFrame = captureMetrics(viewport, app);
    if (!lostFrame.valid) {
        return 17;
    }

    viewport.zoomExtents();
    CaptureMetrics framed = captureMetrics(viewport, app);
    if (!framed.valid) {
        return 18;
    }

    if (!skipCoverage) {
        if (static_cast<double>(lostFrame.nonBackground) / std::max(1, lostFrame.totalPixels) > 0.02) {
            return 19;
        }
        if (framed.nonBackground <= lostFrame.nonBackground + 1200) {
            return 20;
        }
        if (static_cast<double>(framed.nonBackground) / std::max(1, framed.totalPixels) <= 0.02) {
            return 21;
        }
    }

    viewport.setRenderStyle(Renderer::RenderStyle::Wireframe);
    CaptureMetrics wire = captureMetrics(viewport, app);
    if (!wire.valid || wire.nonBackground <= 0) {
        return 1;
    }

    viewport.setRenderStyle(Renderer::RenderStyle::Shaded);
    CaptureMetrics shaded = captureMetrics(viewport, app);
    if (!shaded.valid) {
        return 2;
    }

    viewport.setRenderStyle(Renderer::RenderStyle::ShadedWithEdges);
    CaptureMetrics shadedEdges = captureMetrics(viewport, app);
    if (!shadedEdges.valid) {
        return 3;
    }

    viewport.setRenderStyle(Renderer::RenderStyle::HiddenLine);
    CaptureMetrics hiddenLine = captureMetrics(viewport, app);
    if (!hiddenLine.valid || hiddenLine.nonBackground <= 0) {
        return 4;
    }

    viewport.setRenderStyle(Renderer::RenderStyle::Monochrome);
    CaptureMetrics monochrome = captureMetrics(viewport, app);
    if (!monochrome.valid || monochrome.nonBackground <= 0) {
        return 5;
    }

    std::vector<Vector3> remoteBase{
        {6.0f, 0.0f, 6.0f},
        {8.0f, 0.0f, 6.0f},
        {8.0f, 0.0f, 8.0f},
        {6.0f, 0.0f, 8.0f},
        {6.0f, 0.0f, 6.0f}
    };
    GeometryObject* remoteCurve = geometry->addCurve(remoteBase);
    geometry->extrudeCurve(remoteCurve, 0.4f);
    if (!remoteCurve) {
        return 32;
    }
    if (curveObj)
        curveObj->setSelected(false);
    remoteCurve->setSelected(true);
    if (!viewport.focusSelection(false)) {
        return 33;
    }
    float fx = 0.0f;
    float fy = 0.0f;
    float fz = 0.0f;
    camera->getTarget(fx, fy, fz);
    if (std::fabs(fx - 7.0f) > 0.35f || std::fabs(fz - 7.0f) > 0.35f) {
        return 34;
    }
    if (camera->getDistance() < 2.5f || camera->getDistance() > 25.0f) {
        return 35;
    }
    remoteCurve->setSelected(false);

    const double wireCoverage = static_cast<double>(wire.nonBackground) / wire.totalPixels;
    const double shadedCoverage = static_cast<double>(shaded.nonBackground) / shaded.totalPixels;
    const double edgeCoverage = static_cast<double>(shadedEdges.nonBackground) / shadedEdges.totalPixels;

    if (!skipCoverage) {
        if (shadedCoverage <= wireCoverage * 3.0) {
            return 6;
        }
        if (edgeCoverage < shadedCoverage) {
            return 7;
        }
        if ((shadedEdges.nonBackground - shaded.nonBackground) < 150) {
            return 8;
        }
        if (shaded.nonBackgroundIntensity >= 0.85) {
            return 9;
        }
        if (wire.nonBackgroundIntensity >= 0.5) {
            return 10;
        }
        if (hiddenLine.nonBackground <= wire.nonBackground) {
            return 11;
        }
        if (std::fabs(monochrome.nonBackgroundIntensity - shaded.nonBackgroundIntensity) < 0.01) {
            return 12;
        }
    }

    std::vector<Vector3> hiddenLoop{
        {-0.8f, 0.0f, -0.8f},
        {0.8f, 0.0f, -0.8f},
        {0.8f, 0.0f, 0.8f},
        {-0.8f, 0.0f, 0.8f},
        {-0.8f, 0.0f, -0.8f}
    };
    GeometryObject* hiddenCurve = geometry->addCurve(hiddenLoop);
    if (hiddenCurve) {
        hiddenCurve->setHidden(true);
    }

    viewport.setRenderStyle(Renderer::RenderStyle::ShadedWithEdges);
    viewport.setShowHiddenGeometry(false);
    CaptureMetrics hiddenOff = captureMetrics(viewport, app);
    viewport.setShowHiddenGeometry(true);
    CaptureMetrics hiddenOn = captureMetrics(viewport, app);
    if (!skipCoverage) {
        if (!hiddenOn.valid || hiddenOn.nonBackground <= hiddenOff.nonBackground + 25) {
            return 13;
        }
    } else if (!hiddenOn.valid) {
        return 13;
    }

    Scene::Document* document = viewport.getDocument();
    Scene::SectionPlane plane;
    plane.setFromOriginAndNormal(Vector3(0.0f, 0.0f, 0.25f), Vector3(0.0f, 0.0f, 1.0f));
    plane.setActive(true);
    plane.setVisible(true);
    document->sectionPlanes().push_back(plane);

    document->settings().setSectionPlanesVisible(true);
    CaptureMetrics clipped = captureMetrics(viewport, app);
    if (!clipped.valid) {
        return 14;
    }

    document->settings().setSectionPlanesVisible(false);
    CaptureMetrics unclipped = captureMetrics(viewport, app);
    if (!unclipped.valid) {
        return 15;
    }

    if (!skipCoverage) {
        if (static_cast<double>(unclipped.nonBackground)
            <= static_cast<double>(clipped.nonBackground) * 1.1) {
            return 16;
        }
    }

    document->settings().setSectionPlanesVisible(true);

    {
        GLViewport hiddenViewport;
        hiddenViewport.resize(200, 150);
        hiddenViewport.show();
        if (!hiddenViewport.getCamera()) {
            return 29;
        }
        GeometryKernel* hiddenGeometry = hiddenViewport.getGeometry();
        std::vector<Vector3> hiddenBase{
            {-0.4f, 0.0f, -0.4f},
            {0.4f, 0.0f, -0.4f},
            {0.4f, 0.0f, 0.4f},
            {-0.4f, 0.0f, 0.4f},
            {-0.4f, 0.0f, -0.4f}
        };
        GeometryObject* hiddenSolid = hiddenGeometry->addCurve(hiddenBase);
        hiddenGeometry->extrudeCurve(hiddenSolid, 0.3f);
        if (hiddenSolid) {
            hiddenSolid->setHidden(true);
        }
        hiddenViewport.setShowHiddenGeometry(false);
        if (hiddenViewport.zoomExtents()) {
            return 30;
        }
        hiddenViewport.setShowHiddenGeometry(true);
        if (!hiddenViewport.zoomExtents()) {
            return 31;
        }
    }

    return 0;
}

