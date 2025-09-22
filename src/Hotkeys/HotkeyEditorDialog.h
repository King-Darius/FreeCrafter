#pragma once

#include <QDialog>
#include <QVector>

#include "HotkeyManager.h"

class QTableWidget;
class QPushButton;

class HotkeyEditorDialog : public QDialog
{
    Q_OBJECT
public:
    HotkeyEditorDialog(HotkeyManager* manager, QWidget* parent = nullptr);

private slots:
    void applyChanges();

private:
    void populateFromManager();

    HotkeyManager* manager;
    QTableWidget* table = nullptr;
};
