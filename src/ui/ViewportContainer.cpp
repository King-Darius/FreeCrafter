#include "ViewportContainer.h"

#include "GLViewport.h"
#include "NavigationPreferences.h"
#include "PalettePreferences.h"
#include "Tools/ToolManager.h"

#include <QList>
#include <QSplitter>
#include <QStackedLayout>
#include <QVBoxLayout>

ViewportContainer::ViewportContainer(QWidget* parent)
    : QWidget(parent)
{
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    stackedLayout_ = new QStackedLayout;
    stackedLayout_->setStackingMode(QStackedLayout::StackOne);
    outerLayout->addLayout(stackedLayout_);

    singlePage_ = new QWidget(this);
    singleLayout_ = new QVBoxLayout(singlePage_);
    singleLayout_->setContentsMargins(0, 0, 0, 0);
    singleLayout_->setSpacing(0);

    primaryViewport_ = new GLViewport(singlePage_);
    attachViewport(primaryViewport_);
    singleLayout_->addWidget(primaryViewport_);

    stackedLayout_->addWidget(singlePage_);

    splitPage_ = new QWidget(this);
    auto* splitLayout = new QVBoxLayout(splitPage_);
    splitLayout->setContentsMargins(0, 0, 0, 0);
    splitLayout->setSpacing(0);

    splitter_ = new QSplitter(Qt::Horizontal, splitPage_);
    splitter_->setChildrenCollapsible(false);
    splitLayout->addWidget(splitter_);

    stackedLayout_->addWidget(splitPage_);
    stackedLayout_->setCurrentWidget(singlePage_);
}

GLViewport* ViewportContainer::primaryViewport() const
{
    return primaryViewport_;
}

GLViewport* ViewportContainer::secondaryViewport() const
{
    return secondaryViewport_;
}

QList<GLViewport*> ViewportContainer::viewports() const
{
    QList<GLViewport*> result;
    if (primaryViewport_)
        result.push_back(primaryViewport_);
    if (secondaryViewport_)
        result.push_back(secondaryViewport_);
    return result;
}

void ViewportContainer::setToolManager(ToolManager* manager)
{
    toolManager_ = manager;
    for (GLViewport* viewport : viewports()) {
        if (viewport)
            viewport->setToolManager(toolManager_);
    }
}

void ViewportContainer::setNavigationPreferences(NavigationPreferences* prefs)
{
    navigationPrefs_ = prefs;
    for (GLViewport* viewport : viewports()) {
        if (viewport)
            viewport->setNavigationPreferences(navigationPrefs_);
    }
}

void ViewportContainer::setPalettePreferences(PalettePreferences* prefs)
{
    palettePrefs_ = prefs;
    for (GLViewport* viewport : viewports()) {
        if (viewport)
            viewport->setPalettePreferences(palettePrefs_);
    }
}

void ViewportContainer::setDocument(Scene::Document* document)
{
    document_ = document;
    for (GLViewport* viewport : viewports()) {
        if (viewport)
            viewport->setDocument(document_);
    }
}

void ViewportContainer::setSplitEnabled(bool enabled)
{
    if (splitEnabled_ == enabled)
        return;

    splitEnabled_ = enabled;

    if (splitEnabled_) {
        GLViewport* secondary = ensureSecondaryViewport();
        if (primaryViewport_->parentWidget())
            primaryViewport_->parentWidget()->layout()->removeWidget(primaryViewport_);
        primaryViewport_->setParent(splitter_);
        splitter_->addWidget(primaryViewport_);

        secondary->setParent(splitter_);
        splitter_->addWidget(secondary);
        secondary->show();
        splitter_->show();

        stackedLayout_->setCurrentWidget(splitPage_);
    } else {
        splitter_->hide();
        splitter_->removeWidget(primaryViewport_);
        if (secondaryViewport_) {
            splitter_->removeWidget(secondaryViewport_);
            secondaryViewport_->setParent(this);
            secondaryViewport_->hide();
        }
        primaryViewport_->setParent(singlePage_);
        singleLayout_->addWidget(primaryViewport_);
        stackedLayout_->setCurrentWidget(singlePage_);
    }

    emit splitModeChanged(splitEnabled_);
}

void ViewportContainer::attachViewport(GLViewport* viewport)
{
    if (!viewport)
        return;

    viewport->setDocument(document_);
    viewport->setToolManager(toolManager_);
    viewport->setNavigationPreferences(navigationPrefs_);
    viewport->setPalettePreferences(palettePrefs_);
    connect(viewport, &GLViewport::activated, this, &ViewportContainer::handleViewportActivated, Qt::UniqueConnection);
}

GLViewport* ViewportContainer::ensureSecondaryViewport()
{
    if (!secondaryViewport_) {
        secondaryViewport_ = new GLViewport(this);
        secondaryViewport_->setObjectName(QStringLiteral("SecondaryViewport"));
        attachViewport(secondaryViewport_);
    }
    return secondaryViewport_;
}

void ViewportContainer::handleViewportActivated(GLViewport* viewport)
{
    if (!viewport)
        return;
    emit viewportActivated(viewport);
}
