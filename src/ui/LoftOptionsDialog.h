#pragma once

#include <QDialog>

#include "Phase6/AdvancedModeling.h"

class QCheckBox;
class QDoubleSpinBox;
class QSpinBox;

class LoftOptionsDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoftOptionsDialog(QWidget* parent = nullptr);

    void setOptions(const Phase6::LoftOptions& options);
    Phase6::LoftOptions options() const;

private:
    QSpinBox* sectionsSpin_ = nullptr;
    QDoubleSpinBox* twistSpin_ = nullptr;
    QSpinBox* smoothingSpin_ = nullptr;
    QCheckBox* closeRailsCheck_ = nullptr;
    QCheckBox* smoothNormalsCheck_ = nullptr;
    QCheckBox* symmetricPairCheck_ = nullptr;
};
