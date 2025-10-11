#pragma once

#include "GeometryKernel/GeometryKernel.h"

#include <QDialog>

class QLabel;
class QCheckBox;
class QPushButton;

class GuideManagerDialog : public QDialog {
    Q_OBJECT
public:
    explicit GuideManagerDialog(const GeometryKernel::GuideState& currentState, QWidget* parent = nullptr);

    GeometryKernel::GuideState selectedState() const;

private slots:
    void handleClear();

private:
    static GeometryKernel::GuideState makeAxisState(bool x, bool y, bool z);

    GeometryKernel::GuideState initialState;
    QCheckBox* xAxisCheck = nullptr;
    QCheckBox* yAxisCheck = nullptr;
    QCheckBox* zAxisCheck = nullptr;
    QPushButton* clearButton = nullptr;
    QLabel* countLabel = nullptr;
    bool cleared = false;
};

