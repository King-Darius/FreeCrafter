#include "ImageImportDialog.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
constexpr double kMinDimension = 0.01;
constexpr double kMaxDimension = 50000.0;
}

ImageImportDialog::ImageImportDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Import Image"));
    setModal(true);

    pathEdit = new QLineEdit(this);
    pathEdit->setPlaceholderText(tr("Image file path"));
    pathEdit->setToolTip(tr("Choose an image file to insert as a reference plane."));

    browseButton = new QPushButton(tr("Browse..."), this);
    browseButton->setToolTip(tr("Select an image file from disk."));
    connect(browseButton, &QPushButton::clicked, this, &ImageImportDialog::browseForImage);

    widthSpin = new QDoubleSpinBox(this);
    widthSpin->setRange(kMinDimension, kMaxDimension);
    widthSpin->setDecimals(3);
    widthSpin->setValue(2.0);
    widthSpin->setSuffix(tr(" mm"));
    widthSpin->setToolTip(tr("Width of the inserted image plane."));

    heightSpin = new QDoubleSpinBox(this);
    heightSpin->setRange(kMinDimension, kMaxDimension);
    heightSpin->setDecimals(3);
    heightSpin->setValue(2.0);
    heightSpin->setSuffix(tr(" mm"));
    heightSpin->setToolTip(tr("Height of the inserted image plane."));

    auto* formLayout = new QFormLayout();
    formLayout->addRow(tr("Image"), pathEdit);
    formLayout->addRow(QString(), browseButton);
    formLayout->addRow(tr("Width"), widthSpin);
    formLayout->addRow(tr("Height"), heightSpin);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(formLayout);
    layout->addWidget(buttonBox);

    connect(pathEdit, &QLineEdit::textChanged, this, &ImageImportDialog::validate);
    validate();
}

Scene::ImagePlaneOptions ImageImportDialog::options() const
{
    Scene::ImagePlaneOptions opts;
    opts.filePath = pathEdit->text().toStdString();
    opts.width = static_cast<float>(widthSpin->value());
    opts.height = static_cast<float>(heightSpin->value());
    return opts;
}

void ImageImportDialog::browseForImage()
{
    const QString file = QFileDialog::getOpenFileName(this, tr("Select Image"), QString(), tr("Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff);;All Files (*)"));
    if (!file.isEmpty()) {
        pathEdit->setText(file);
    }
}

void ImageImportDialog::validate()
{
    const bool valid = !pathEdit->text().trimmed().isEmpty();
    if (buttonBox) {
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
    }
}

