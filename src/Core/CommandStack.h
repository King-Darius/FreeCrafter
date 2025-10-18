#pragma once

#include "Command.h"

#include <memory>

class QUndoStack;

namespace Core {

class CommandStack {
public:
    CommandStack() = default;
    explicit CommandStack(QUndoStack* stack);

    void setUndoStack(QUndoStack* stack);
    void setContext(const CommandContext& context);

    const CommandContext& context() const { return context_; }
    QUndoStack* undoStack() const { return undoStack_; }

    void push(std::unique_ptr<Command> command);

private:
    QUndoStack* undoStack_ = nullptr;
    CommandContext context_;
};

} // namespace Core

