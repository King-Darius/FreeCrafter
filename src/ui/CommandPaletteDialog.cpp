#include "CommandPaletteDialog.h"

#include "HotkeyManager.h"
#include "Tools/ToolManager.h"

#include <QAction>
#include <QAbstractItemView>
#include <QEvent>
#include <QHash>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListView>
#include <QRegularExpression>
#include <QStringList>
#include <QSignalBlocker>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include <algorithm>
#include <limits>
#include <optional>

namespace {
std::optional<int> fuzzyScore(const QString& pattern, const QString& text)
{
    const QString trimmed = pattern.trimmed().toLower();
    if (trimmed.isEmpty())
        return 0;

    const QString haystack = text.toLower();
    const QStringList tokens = trimmed.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    if (tokens.isEmpty())
        return 0;

    int totalScore = 0;
    for (const QString& token : tokens) {
        int position = 0;
        int lastMatch = -1;
        int tokenScore = 0;
        for (const QChar ch : token) {
            const int index = haystack.indexOf(ch, position);
            if (index < 0)
                return std::nullopt;
            tokenScore += 10;
            if (lastMatch >= 0) {
                const int gap = index - lastMatch;
                if (gap == 1)
                    tokenScore += 5;
                else
                    tokenScore -= gap;
            } else {
                tokenScore -= index;
            }
            lastMatch = index;
            position = index + 1;
        }
        if (haystack.startsWith(token))
            tokenScore += 10;
        else if (haystack.contains(token))
            tokenScore += 5;
        totalScore += tokenScore;
    }

    return totalScore;
}

struct Candidate {
    int entryIndex = -1;
    int score = 0;
    int recentIndex = std::numeric_limits<int>::max();
    QString sortKey;
};
} // namespace

CommandPaletteDialog::CommandPaletteDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Command Palette"));
    setObjectName(QStringLiteral("CommandPaletteDialog"));
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setWindowFlag(Qt::Tool, true);
    setModal(false);
    resize(480, 400);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    searchEdit = new QLineEdit(this);
    searchEdit->setObjectName(QStringLiteral("CommandPaletteSearch"));
    searchEdit->setPlaceholderText(tr("Search commands…"));
    searchEdit->installEventFilter(this);
    connect(searchEdit, &QLineEdit::textChanged, this, &CommandPaletteDialog::handleFilterTextChanged);
    connect(searchEdit, &QLineEdit::returnPressed, this, &CommandPaletteDialog::activateCurrent);
    layout->addWidget(searchEdit);

    listView = new QListView(this);
    listView->setObjectName(QStringLiteral("CommandPaletteResults"));
    listView->setSelectionBehavior(QAbstractItemView::SelectRows);
    listView->setSelectionMode(QAbstractItemView::SingleSelection);
    listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    listView->setUniformItemSizes(true);
    listView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    connect(listView, &QListView::activated, this, &CommandPaletteDialog::handleItemActivated);
    connect(listView, &QListView::doubleClicked, this, &CommandPaletteDialog::handleItemActivated);
    layout->addWidget(listView, 1);

    model = new QStandardItemModel(this);
    listView->setModel(model);
}

void CommandPaletteDialog::populate(const HotkeyManager& hotkeys,
    ToolManager* toolManager,
    const QStringList& recentCommands,
    const std::function<QString(const QString&)>& toolHintResolver)
{
    QHash<QString, int> recentOrder;
    for (int i = 0; i < recentCommands.size(); ++i)
        recentOrder.insert(recentCommands.at(i), i);

    const auto commandInfos = hotkeys.commands();
    entries.clear();
    entries.reserve(commandInfos.size());
    const auto shortcuts = hotkeys.currentMap();

    for (const auto& info : commandInfos) {
        QAction* action = info.action;
        if (!action)
            continue;

        const QString descriptorId = action->objectName();
        if (toolManager && !descriptorId.isEmpty() && info.id.startsWith(QStringLiteral("tools."))) {
            if (!toolManager->hasTool(descriptorId))
                continue;
        }

        Entry entry;
        entry.id = info.id;
        entry.title = info.label.isEmpty() ? action->text() : info.label;
        entry.icon = action->icon();
        entry.shortcut = shortcuts.value(info.id).toString(QKeySequence::NativeText);

        QString detail = action->statusTip();
        if (detail.isEmpty())
            detail = action->toolTip();
        if (detail.isEmpty())
            detail = action->whatsThis();
        if (detail.isEmpty() && toolHintResolver && !descriptorId.isEmpty())
            detail = toolHintResolver(descriptorId);
        entry.detail = detail;

        entry.callback = [action]() {
            if (action->isEnabled())
                action->trigger();
        };

        InternalEntry internal;
        internal.entry = std::move(entry);
        internal.recentIndex = recentOrder.value(internal.entry.id, std::numeric_limits<int>::max());
        entries.push_back(std::move(internal));
    }

    if (searchEdit) {
        QSignalBlocker blocker(searchEdit);
        searchEdit->clear();
    }
    currentFilter.clear();

    rebuildModel();
}

void CommandPaletteDialog::focusInput()
{
    if (!searchEdit)
        return;

    searchEdit->setFocus(Qt::ShortcutFocusReason);
    searchEdit->selectAll();
}

void CommandPaletteDialog::activateCurrent()
{
    if (!listView)
        return;

    QModelIndex index = listView->currentIndex();
    if (!index.isValid() && model && model->rowCount() > 0)
        index = model->index(0, 0);

    activateIndex(index);
}

bool CommandPaletteDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == searchEdit && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Down || keyEvent->key() == Qt::Key_Up) {
            if (!model || model->rowCount() == 0)
                return false;

            const int delta = keyEvent->key() == Qt::Key_Down ? 1 : -1;
            int row = listView->currentIndex().row();
            if (row < 0)
                row = keyEvent->key() == Qt::Key_Down ? 0 : model->rowCount() - 1;
            else
                row = std::clamp(row + delta, 0, model->rowCount() - 1);

            const QModelIndex nextIndex = model->index(row, 0);
            listView->setCurrentIndex(nextIndex);
            return true;
        }
    }

    return QDialog::eventFilter(watched, event);
}

void CommandPaletteDialog::handleFilterTextChanged(const QString& text)
{
    currentFilter = text;
    rebuildModel();
}

void CommandPaletteDialog::handleItemActivated(const QModelIndex& index)
{
    activateIndex(index);
}

void CommandPaletteDialog::rebuildModel()
{
    if (!model)
        return;

    QVector<Candidate> candidates;
    candidates.reserve(entries.size());

    const QString effectiveFilter = currentFilter.trimmed();
    const bool hasFilter = !effectiveFilter.isEmpty();

    for (int i = 0; i < entries.size(); ++i) {
        const InternalEntry& internal = entries.at(i);
        const QString combined = internal.entry.title + QLatin1Char(' ') + internal.entry.detail;

        int score = 0;
        if (internal.recentIndex != std::numeric_limits<int>::max())
            score += 100000 - internal.recentIndex * 100;

        if (hasFilter) {
            const auto fuzzy = fuzzyScore(effectiveFilter, combined);
            if (!fuzzy.has_value())
                continue;
            score += *fuzzy;
        }

        Candidate candidate;
        candidate.entryIndex = i;
        candidate.score = score;
        candidate.recentIndex = internal.recentIndex;
        candidate.sortKey = internal.entry.title.toLower();
        candidates.push_back(std::move(candidate));
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        if (a.score != b.score)
            return a.score > b.score;
        if (a.recentIndex != b.recentIndex)
            return a.recentIndex < b.recentIndex;
        return a.sortKey < b.sortKey;
    });

    QSignalBlocker blocker(listView);
    model->clear();

    for (const Candidate& candidate : candidates) {
        const Entry& entry = entries.at(candidate.entryIndex).entry;
        QString display = entry.title;
        if (!entry.shortcut.isEmpty())
            display += QStringLiteral("\t%1").arg(entry.shortcut);

        auto* item = new QStandardItem(entry.icon, display);
        if (!entry.detail.isEmpty())
            item->setData(entry.detail, Qt::ToolTipRole);
        item->setData(candidate.entryIndex, Qt::UserRole);
        model->appendRow(item);
    }

    if (model->rowCount() > 0) {
        const QModelIndex first = model->index(0, 0);
        listView->setCurrentIndex(first);
    } else {
        listView->clearSelection();
    }
}

void CommandPaletteDialog::activateIndex(const QModelIndex& index)
{
    if (!index.isValid())
        return;

    const int entryIndex = index.data(Qt::UserRole).toInt();
    if (entryIndex < 0 || entryIndex >= entries.size())
        return;

    const Entry& entry = entries.at(entryIndex).entry;
    if (entry.callback)
        entry.callback();

    emit commandExecuted(entry.id, entry.title, entry.detail);

    accept();
}
