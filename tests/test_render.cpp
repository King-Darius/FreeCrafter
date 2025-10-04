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

#include <vector>
#include <cmath>

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

    GLViewport viewport;
    viewport.resize(400, 300);
    viewport.show();

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
    const bool skipCoverage = qEnvironmentVariableIsSet("FREECRAFTER_RENDER_SKIP_COVERAGE");
    if (!monochrome.valid || monochrome.nonBackground <= 0) {
        return 5;
    }

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

    return 0;
}

