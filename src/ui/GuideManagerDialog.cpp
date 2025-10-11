#include "GuideManagerDialog.h"

#include "GeometryKernel/Vector3.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <cmath>

namespace {
bool nearlyAxisLine(const GeometryKernel::GuideState::GuideLine& line, const Vector3& axis)
{
    Vector3 dir = line.end - line.start;
    if (dir.lengthSquared() <= 1e-6f)
        return false;
    dir = dir.normalized();
    return std::abs(std::abs(dir.dot(axis)) - 1.0f) <= 1e-3f;
}
}

GuideManagerDialog::GuideManagerDialog(const GeometryKernel::GuideState& currentState, QWidget* parent)
    : QDialog(parent)
    , initialState(currentState)
{
    setWindowTitle(tr("Guide Manager"));
    setModal(true);

    int lineCount = static_cast<int>(currentState.lines.size());
    int pointCount = static_cast<int>(currentState.points.size());
    int angleCount = static_cast<int>(currentState.angles.size());

    xAxisCheck = new QCheckBox(tr("World X guide"), this);
    yAxisCheck = new QCheckBox(tr("World Y guide"), this);
    zAxisCheck = new QCheckBox(tr("World Z guide"), this);

    for (const auto& line : currentState.lines) {
        if (nearlyAxisLine(line, Vector3(1.0f, 0.0f, 0.0f)))
            xAxisCheck->setChecked(true);
        if (nearlyAxisLine(line, Vector3(0.0f, 1.0f, 0.0f)))
            yAxisCheck->setChecked(true);
        if (nearlyAxisLine(line, Vector3(0.0f, 0.0f, 1.0f)))
            zAxisCheck->setChecked(true);
    }

    xAxisCheck->setToolTip(tr("Add a guide line along the global X axis."));
    yAxisCheck->setToolTip(tr("Add a guide line along the global Y axis."));
    zAxisCheck->setToolTip(tr("Add a guide line along the global Z axis."));

    countLabel = new QLabel(tr("Existing guides: %1 lines, %2 points, %3 angles")
                                .arg(lineCount)
                                .arg(pointCount)
                                .arg(angleCount),
                            this);
    countLabel->setWordWrap(true);

    clearButton = new QPushButton(tr("Clear All Guides"), this);
    clearButton->setToolTip(tr("Remove every guide from the document."));
    connect(clearButton, &QPushButton::clicked, this, &GuideManagerDialog::handleClear);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(countLabel);
    layout->addWidget(xAxisCheck);
    layout->addWidget(yAxisCheck);
    layout->addWidget(zAxisCheck);
    layout->addWidget(clearButton);
    layout->addWidget(buttonBox);
}

GeometryKernel::GuideState GuideManagerDialog::selectedState() const
{
    if (cleared)
        return {};
    return makeAxisState(xAxisCheck->isChecked(), yAxisCheck->isChecked(), zAxisCheck->isChecked());
}

void GuideManagerDialog::handleClear()
{
    cleared = true;
    xAxisCheck->setChecked(false);
    yAxisCheck->setChecked(false);
    zAxisCheck->setChecked(false);
}

GeometryKernel::GuideState GuideManagerDialog::makeAxisState(bool x, bool y, bool z)
{
    GeometryKernel::GuideState state;
    const float extent = 1000.0f;
    if (x) {
        state.lines.push_back({ Vector3(-extent, 0.0f, 0.0f), Vector3(extent, 0.0f, 0.0f) });
    }
    if (y) {
        state.lines.push_back({ Vector3(0.0f, -extent, 0.0f), Vector3(0.0f, extent, 0.0f) });
    }
    if (z) {
        state.lines.push_back({ Vector3(0.0f, 0.0f, -extent), Vector3(0.0f, 0.0f, extent) });
    }
    return state;
}

