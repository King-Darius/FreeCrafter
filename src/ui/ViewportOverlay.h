#pragma once

#include <QWidget>

class ViewportOverlay : public QWidget {
    Q_OBJECT
public:
    explicit ViewportOverlay(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
};
