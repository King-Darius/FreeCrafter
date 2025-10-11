#pragma once

#include "Scene/SceneCommands.h"

#include <QDialog>

class QDialogButtonBox;
class QDoubleSpinBox;
class QLineEdit;
class QPushButton;

class ExternalReferenceDialog : public QDialog {
    Q_OBJECT
public:
    explicit ExternalReferenceDialog(QWidget* parent = nullptr);

    Scene::ExternalReferenceOptions options() const;

private slots:
    void browseForReference();
    void validate();

private:
    QLineEdit* pathEdit = nullptr;
    QLineEdit* nameEdit = nullptr;
    QDoubleSpinBox* widthSpin = nullptr;
    QDoubleSpinBox* depthSpin = nullptr;
    QDoubleSpinBox* heightSpin = nullptr;
    QDialogButtonBox* buttonBox = nullptr;
};

