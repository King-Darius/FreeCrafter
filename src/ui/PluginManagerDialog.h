#pragma once

#include <QDialog>

class QLabel;
class QListWidget;
class QPushButton;

class PluginManagerDialog : public QDialog {
    Q_OBJECT
public:
    explicit PluginManagerDialog(QWidget* parent = nullptr);

private slots:
    void refreshPlugins();

private:
    void populateFromDirectory(const QString& path);

    QListWidget* pluginList = nullptr;
    QLabel* statusLabel = nullptr;
};

