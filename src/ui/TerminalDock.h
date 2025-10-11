#pragma once

#include <QProcess>
#include <QWidget>

class QPlainTextEdit;
class QLineEdit;
class QPushButton;
class QToolButton;

class TerminalDock : public QWidget {
    Q_OBJECT
public:
    explicit TerminalDock(QWidget* parent = nullptr);

    void appendMessage(const QString& message);
    void appendError(const QString& message);
    void runCommand(const QString& command);
    void focusCommandInput();

signals:
    void commandStarted(const QString& command);
    void commandFinished(const QString& command, int exitCode);
    void commandFailed(const QString& command, QProcess::ProcessError error);

private slots:
    void handleSubmit();
    void handleReadyReadStandardOutput();
    void handleReadyReadStandardError();
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleProcessError(QProcess::ProcessError error);

private:
    enum class MessageType { Info, Error };

    void appendPrompt(const QString& command);
    void appendText(const QString& text);
    void appendLine(const QString& text, MessageType type = MessageType::Info);
    void setCommandInProgress(bool running);
    void resetProcess();

    QPlainTextEdit* outputView = nullptr;
    QLineEdit* commandEdit = nullptr;
    QPushButton* runButton = nullptr;
    QToolButton* clearButton = nullptr;
    QProcess* process = nullptr;
    QString currentCommand;
};
