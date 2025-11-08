#pragma once

#include <QWidget>

class GLViewport;
class QStackedLayout;
class QSplitter;
class QVBoxLayout;
class NavigationPreferences;
class PalettePreferences;
class ToolManager;

namespace Scene {
class Document;
}

class ViewportContainer : public QWidget {
    Q_OBJECT
public:
    explicit ViewportContainer(QWidget* parent = nullptr);

    GLViewport* primaryViewport() const;
    GLViewport* secondaryViewport() const;
    QList<GLViewport*> viewports() const;

    void setToolManager(ToolManager* manager);
    void setNavigationPreferences(NavigationPreferences* prefs);
    void setPalettePreferences(PalettePreferences* prefs);
    void setDocument(Scene::Document* document);

    void setSplitEnabled(bool enabled);
    bool isSplitEnabled() const { return splitEnabled_; }

signals:
    void viewportActivated(GLViewport* viewport);
    void splitModeChanged(bool enabled);

private:
    void attachViewport(GLViewport* viewport);
    GLViewport* ensureSecondaryViewport();
    void handleViewportActivated(GLViewport* viewport);

    QStackedLayout* stackedLayout_ = nullptr;
    QWidget* singlePage_ = nullptr;
    QWidget* splitPage_ = nullptr;
    QVBoxLayout* singleLayout_ = nullptr;
    QSplitter* splitter_ = nullptr;

    GLViewport* primaryViewport_ = nullptr;
    GLViewport* secondaryViewport_ = nullptr;

    ToolManager* toolManager_ = nullptr;
    NavigationPreferences* navigationPrefs_ = nullptr;
    PalettePreferences* palettePrefs_ = nullptr;
    Scene::Document* document_ = nullptr;

    bool splitEnabled_ = false;
};
