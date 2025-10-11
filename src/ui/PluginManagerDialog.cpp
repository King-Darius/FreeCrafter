#include "PluginManagerDialog.h"

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

PluginManagerDialog::PluginManagerDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Plugin Manager"));
    setModal(true);

    statusLabel = new QLabel(this);
    statusLabel->setWordWrap(true);

    pluginList = new QListWidget(this);
    pluginList->setSelectionMode(QListWidget::NoSelection);
    pluginList->setMinimumHeight(200);

    auto* refreshButton = new QPushButton(tr("Refresh"), this);
    refreshButton->setToolTip(tr("Rescan plugin directories."));
    connect(refreshButton, &QPushButton::clicked, this, &PluginManagerDialog::refreshPlugins);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox->button(QDialogButtonBox::Close), &QPushButton::clicked, this, &QDialog::accept);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(statusLabel);
    layout->addWidget(pluginList);
    layout->addWidget(refreshButton);
    layout->addWidget(buttonBox);

    refreshPlugins();
}

void PluginManagerDialog::refreshPlugins()
{
    pluginList->clear();

    QStringList searchRoots;
    searchRoots << QCoreApplication::applicationDirPath() + QStringLiteral("/plugins");
    searchRoots << QDir::currentPath() + QStringLiteral("/plugins");
    searchRoots << QDir::currentPath() + QStringLiteral("/dist/plugins");
    searchRoots << QDir::currentPath() + QStringLiteral("/qt/plugins");

    for (const QString& root : searchRoots) {
        populateFromDirectory(root);
    }

    if (pluginList->count() == 0) {
        statusLabel->setText(tr("No plugins found in known directories."));
    } else {
        statusLabel->setText(tr("Loaded %1 plugin entries.").arg(pluginList->count()));
    }
}

void PluginManagerDialog::populateFromDirectory(const QString& path)
{
    QDir dir(path);
    if (!dir.exists())
        return;

    const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo& info : entries) {
        if (info.isDir()) {
            QListWidgetItem* item = new QListWidgetItem(tr("%1/ (directory)").arg(info.fileName()), pluginList);
            item->setToolTip(info.absoluteFilePath());
        } else if (info.suffix().compare(QStringLiteral("json"), Qt::CaseInsensitive) == 0
                   || info.suffix().compare(QStringLiteral("dll"), Qt::CaseInsensitive) == 0
                   || info.suffix().compare(QStringLiteral("so"), Qt::CaseInsensitive) == 0
                   || info.suffix().compare(QStringLiteral("dylib"), Qt::CaseInsensitive) == 0) {
            QListWidgetItem* item = new QListWidgetItem(info.fileName(), pluginList);
            item->setToolTip(info.absoluteFilePath());
        }
    }
}

