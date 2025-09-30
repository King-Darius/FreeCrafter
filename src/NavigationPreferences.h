#pragma once

#include <QObject>
#include <QList>
#include <QString>

#include "NavigationConfig.h"

class NavigationPreferences : public QObject {
    Q_OBJECT
public:
    struct SchemeInfo {
        QString id;
        QString label;
        QString description;
    };

    explicit NavigationPreferences(QObject* parent = nullptr);

    const NavigationConfig& config() const { return activeConfig; }
    QList<SchemeInfo> availableSchemes() const;
    QString activeScheme() const { return activeSchemeId; }
    NavigationConfig schemeConfig(const QString& id) const;

    bool setActiveScheme(const QString& id);
    void setZoomToCursor(bool enabled);
    void setInvertWheel(bool enabled);
    void resetToDefaults();
    void save() const;

signals:
    void configChanged();

private:
    struct SchemeDefinition {
        SchemeInfo info;
        NavigationConfig config;
    };

    QList<SchemeDefinition> schemes;
    NavigationConfig activeConfig;
    QString activeSchemeId;
    QString defaultSchemeId;
    QString configPath;
    int schemaVersion = 1;

    void ensureConfigPath();
    void loadDefaults();
    void load();
    const SchemeDefinition* findScheme(const QString& id) const;
};
