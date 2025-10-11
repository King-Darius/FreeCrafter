#include "ViewportOverlay.h"

#include <QPainter>
#include <QPaintEvent>
#include <QLineF>
#include <QPolygonF>
#include <QRadialGradient>
#include <QtMath>

ViewportOverlay::ViewportOverlay(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setFocusPolicy(Qt::NoFocus);
    setFixedSize(100, 100);
}

void ViewportOverlay::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QPoint center(width() / 2, height() / 2);
    const int radius = qMin(width(), height()) / 2 - 4;

    QRadialGradient gradient(center, radius);
    gradient.setColorAt(0.0, QColor(70, 78, 92));
    gradient.setColorAt(1.0, QColor(40, 45, 54));

    painter.setBrush(gradient);
    painter.setPen(QColor(106, 169, 255));
    painter.drawEllipse(center, radius, radius);

    const int axisLength = radius - 10;

    auto drawAxis = [&](const QPoint& end, const QColor& color, const QString& label) {
        painter.setPen(QPen(color, 2));
        painter.drawLine(center, end);

        QLineF line(center, end);
        const double angle = qDegreesToRadians(line.angle());
        QPointF h1 = end - QPointF(8 * std::cos(angle - 0.4), -8 * std::sin(angle - 0.4));
        QPointF h2 = end - QPointF(8 * std::cos(angle + 0.4), -8 * std::sin(angle + 0.4));
        QPolygonF tip;
        tip << end << h1 << h2;
        painter.setBrush(color);
        painter.drawPolygon(tip);

        painter.setPen(QColor(220, 225, 230));
        painter.drawText(end + QPoint(4, 4), label);
    };

    drawAxis(center + QPoint(axisLength, 0), QColor(230, 80, 80), QStringLiteral("X"));
    drawAxis(center + QPoint(0, axisLength), QColor(80, 200, 80), QStringLiteral("Y"));
    drawAxis(center + QPoint(-axisLength * 0.7, -axisLength * 0.7), QColor(80, 150, 255), QStringLiteral("Z"));
}
