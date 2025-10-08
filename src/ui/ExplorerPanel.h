#pragma once

#include <QWidget>

class QLabel;

class ExplorerPanel : public QWidget {
    Q_OBJECT
public:
    explicit ExplorerPanel(QWidget* parent = nullptr);
};
