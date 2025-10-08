#pragma once

#include <QWidget>
#include <QString>

class CollapsibleSection : public QWidget {
    Q_OBJECT
public:
    explicit CollapsibleSection(const QString& title, QWidget* content,
                                bool open = true, QWidget* parent = nullptr);
    bool isOpen() const { return open_; }

signals:
    void toggled(bool open);

private:
    QWidget* content_ = nullptr;
    bool open_ = true;
};
