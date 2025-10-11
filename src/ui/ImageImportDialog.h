#pragma once

#include "Scene/SceneCommands.h"

#include <QDialog>

class QDoubleSpinBox;
class QLineEdit;
class QPushButton;
class QDialogButtonBox;

class ImageImportDialog : public QDialog {
    Q_OBJECT
public:
    explicit ImageImportDialog(QWidget* parent = nullptr);

    Scene::ImagePlaneOptions options() const;

private slots:
    void browseForImage();
    void validate();

private:
    QLineEdit* pathEdit = nullptr;
    QDoubleSpinBox* widthSpin = nullptr;
    QDoubleSpinBox* heightSpin = nullptr;
    QPushButton* browseButton = nullptr;
    QDialogButtonBox* buttonBox = nullptr;
};

