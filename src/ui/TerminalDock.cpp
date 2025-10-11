#include "ui/TerminalDock.h"

#include <QFont>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextCursor>
#include <QTextOption>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
constexpr int kDefaultDockPadding = 8;
constexpr int kControlSpacing = 6;
}

TerminalDock::TerminalDock(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(kDefaultDockPadding, kDefaultDockPadding, kDefaultDockPadding, kDefaultDockPadding);
    layout->setSpacing(kControlSpacing);

    outputView = new QPlainTextEdit(this);
    outputView->setObjectName(QStringLiteral("terminalOutputView"));
    outputView->setReadOnly(true);
    outputView->setWordWrapMode(QTextOption::NoWrap);

    QFont terminalFont = outputView->font();
    terminalFont.setStyleHint(QFont::Monospace);
    terminalFont.setFamily(QStringLiteral("Monospace"));
    outputView->setFont(terminalFont);

    layout->addWidget(outputView, 1);

    auto* controlsLayout = new QHBoxLayout();
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(kControlSpacing);

    commandEdit = new QLineEdit(this);
    commandEdit->setObjectName(QStringLiteral("terminalCommandEdit"));
    commandEdit->setPlaceholderText(tr("Enter command and press Enter"));
    controlsLayout->addWidget(commandEdit, 1);

    runButton = new QPushButton(tr("Run"), this);
    runButton->setObjectName(QStringLiteral("terminalRunButton"));
    controlsLayout->addWidget(runButton);

    clearButton = new QToolButton(this);
    clearButton->setObjectName(QStringLiteral("terminalClearButton"));
    clearButton->setText(tr("Clear"));
    clearButton->setToolTip(tr("Clear terminal output"));
    controlsLayout->addWidget(clearButton);

    layout->addLayout(controlsLayout);

    connect(runButton, &QPushButton::clicked, this, &TerminalDock::handleSubmit);
    connect(commandEdit, &QLineEdit::returnPressed, this, &TerminalDock::handleSubmit);
    connect(clearButton, &QToolButton::clicked, outputView, &QPlainTextEdit::clear);
}

void TerminalDock::appendMessage(const QString& message)
{
    appendLine(message, MessageType::Info);
}

void TerminalDock::appendError(const QString& message)
{
    appendLine(message, MessageType::Error);
}

void TerminalDock::runCommand(const QString& command)
{
    const QString trimmed = command.trimmed();
    if (trimmed.isEmpty())
        return;

    if (process) {
        appendLine(tr("A command is already running. Please wait for it to finish."), MessageType::Info);
        return;
    }

    currentCommand = trimmed;
    appendPrompt(currentCommand);

    process = new QProcess(this);
    process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(process, &QProcess::readyReadStandardOutput, this, &TerminalDock::handleReadyReadStandardOutput);
    connect(process, &QProcess::readyReadStandardError, this, &TerminalDock::handleReadyReadStandardError);
    connect(process,
            &QProcess::errorOccurred,
            this,
            &TerminalDock::handleProcessError);
    connect(process,
            &QProcess::finished,
            this,
            &TerminalDock::handleProcessFinished);

    setCommandInProgress(true);
    emit commandStarted(currentCommand);

    process->startCommand(currentCommand);
}

void TerminalDock::focusCommandInput()
{
    if (commandEdit)
        commandEdit->setFocus();
}

void TerminalDock::handleSubmit()
{
    if (!commandEdit)
        return;

    const QString submitted = commandEdit->text();
    commandEdit->clear();
    runCommand(submitted);
}

void TerminalDock::handleReadyReadStandardOutput()
{
    if (!process)
        return;

    const QString output = QString::fromLocal8Bit(process->readAllStandardOutput());
    appendText(output);
}

void TerminalDock::handleReadyReadStandardError()
{
    if (!process)
        return;

    const QString output = QString::fromLocal8Bit(process->readAllStandardError());
    appendText(output);
}

void TerminalDock::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (!process)
        return;

    handleReadyReadStandardOutput();
    handleReadyReadStandardError();

    if (exitStatus == QProcess::NormalExit)
        appendLine(tr("Command finished with exit code %1").arg(exitCode));
    else
        appendLine(tr("Command crashed"), MessageType::Error);

    emit commandFinished(currentCommand, exitCode);
    resetProcess();
}

void TerminalDock::handleProcessError(QProcess::ProcessError error)
{
    Q_UNUSED(error);

    if (!process)
        return;

    appendLine(tr("Failed to run command: %1").arg(process->errorString()), MessageType::Error);
    emit commandFailed(currentCommand, error);
    resetProcess();
}

void TerminalDock::appendPrompt(const QString& command)
{
    if (!outputView)
        return;

    if (!outputView->document()->isEmpty())
        appendText(QStringLiteral("\n"));

    appendLine(QStringLiteral("$ %1").arg(command));
}

void TerminalDock::appendText(const QString& text)
{
    if (!outputView || text.isEmpty())
        return;

    QTextCursor cursor = outputView->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text);
    outputView->setTextCursor(cursor);
    outputView->ensureCursorVisible();
}

void TerminalDock::appendLine(const QString& text, MessageType type)
{
    QString formatted = text;
    if (type == MessageType::Error)
        formatted = QStringLiteral("[error] %1").arg(text);

    if (!formatted.endsWith(QLatin1Char('\n')))
        formatted.append(QLatin1Char('\n'));

    appendText(formatted);
}

void TerminalDock::setCommandInProgress(bool running)
{
    if (commandEdit)
        commandEdit->setEnabled(!running);
    if (runButton)
        runButton->setEnabled(!running);
}

void TerminalDock::resetProcess()
{
    if (process) {
        process->deleteLater();
        process = nullptr;
    }

    setCommandInProgress(false);
    currentCommand.clear();
    focusCommandInput();
}
