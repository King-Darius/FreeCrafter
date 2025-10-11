#include "ExternalReferenceDialog.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
constexpr double kMinDimension = 0.01;
constexpr double kMaxDimension = 50000.0;
}

ExternalReferenceDialog::ExternalReferenceDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Link External Reference"));
    setModal(true);

    pathEdit = new QLineEdit(this);
    pathEdit->setPlaceholderText(tr("Model or project file"));
    pathEdit->setToolTip(tr("Choose an external file to reference."));

    auto* browseButton = new QPushButton(tr("Browse..."), this);
    browseButton->setToolTip(tr("Select an external reference file."));
    connect(browseButton, &QPushButton::clicked, this, &ExternalReferenceDialog::browseForReference);

    nameEdit = new QLineEdit(this);
    nameEdit->setPlaceholderText(tr("Display name"));
    nameEdit->setToolTip(tr("Name used in the scene for this reference."));

    widthSpin = new QDoubleSpinBox(this);
    widthSpin->setRange(kMinDimension, kMaxDimension);
    widthSpin->setDecimals(3);
    widthSpin->setValue(4.0);
    widthSpin->setSuffix(tr(" mm"));
    widthSpin->setToolTip(tr("Placeholder width for the reference bounding box."));

    depthSpin = new QDoubleSpinBox(this);
    depthSpin->setRange(kMinDimension, kMaxDimension);
    depthSpin->setDecimals(3);
    depthSpin->setValue(4.0);
    depthSpin->setSuffix(tr(" mm"));
    depthSpin->setToolTip(tr("Placeholder depth for the reference bounding box."));

    heightSpin = new QDoubleSpinBox(this);
    heightSpin->setRange(kMinDimension, kMaxDimension);
    heightSpin->setDecimals(3);
    heightSpin->setValue(3.0);
    heightSpin->setSuffix(tr(" mm"));
    heightSpin->setToolTip(tr("Placeholder height for the reference bounding box."));

    auto* formLayout = new QFormLayout();
    formLayout->addRow(tr("Reference"), pathEdit);
    formLayout->addRow(QString(), browseButton);
    formLayout->addRow(tr("Display Name"), nameEdit);
    formLayout->addRow(tr("Width"), widthSpin);
    formLayout->addRow(tr("Depth"), depthSpin);
    formLayout->addRow(tr("Height"), heightSpin);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(formLayout);
    layout->addWidget(buttonBox);

    connect(pathEdit, &QLineEdit::textChanged, this, &ExternalReferenceDialog::validate);
    validate();
}

Scene::ExternalReferenceOptions ExternalReferenceDialog::options() const
{
    Scene::ExternalReferenceOptions opts;
    opts.filePath = pathEdit->text().toStdString();
    opts.displayName = nameEdit->text().trimmed().toStdString();
    opts.width = static_cast<float>(widthSpin->value());
    opts.depth = static_cast<float>(depthSpin->value());
    opts.height = static_cast<float>(heightSpin->value());
    return opts;
}

void ExternalReferenceDialog::browseForReference()
{
    const QString file = QFileDialog::getOpenFileName(this, tr("Select External Reference"), QString(), tr("Scene Files (*.obj *.stl *.gltf *.fbx *.dxf *.dwg);;All Files (*)"));
    if (!file.isEmpty()) {
        pathEdit->setText(file);
        if (nameEdit->text().trimmed().isEmpty()) {
            nameEdit->setText(QFileInfo(file).completeBaseName());
        }
    }
}

void ExternalReferenceDialog::validate()
{
    const bool valid = !pathEdit->text().trimmed().isEmpty();
    if (buttonBox) {
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
    }
}

