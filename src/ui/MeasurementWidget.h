#pragma once

#include <QWidget>

class QComboBox;
class QLineEdit;

class MeasurementWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MeasurementWidget(QWidget* parent = nullptr);

    QString text() const;
    void setHint(const QString& hint);
    QString unitSystem() const;
    void clear();

signals:
    void measurementCommitted(const QString& value, const QString& unitSystem);

private slots:
    void handleReturnPressed();

private:
    QLineEdit* input;
    QComboBox* unitSelector;
};
