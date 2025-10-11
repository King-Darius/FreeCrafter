#pragma once

#include <QDialog>

#include "Phase6/AdvancedModeling.h"

class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QCheckBox;

class ChamferOptionsDialog : public QDialog {
    Q_OBJECT
public:
    explicit ChamferOptionsDialog(QWidget* parent = nullptr);

    void setOptions(const Phase6::RoundCornerOptions& options);
    Phase6::RoundCornerOptions options() const;

private:
    QDoubleSpinBox* radiusSpin_ = nullptr;
    QSpinBox* segmentsSpin_ = nullptr;
    QComboBox* styleCombo_ = nullptr;
    QCheckBox* hardEdgeCheck_ = nullptr;
};
