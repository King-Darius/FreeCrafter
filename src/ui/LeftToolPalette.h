#pragma once

#include <QWidget>
#include <QList>
#include <QString>

class QAction;

struct ToolGroup {
    QString label;
    QList<QAction*> actions;
};

class LeftToolPalette : public QWidget {
    Q_OBJECT
public:
    explicit LeftToolPalette(const QList<ToolGroup>& groups, QWidget* parent = nullptr);
    static int preferredWidth() { return 84; }
};
