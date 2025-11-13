#include "CommandStack.h"

#include <QUndoStack>

namespace Core {

CommandStack::CommandStack(QUndoStack* stack)
    : undoStack_(stack)
{
}

void CommandStack::setUndoStack(QUndoStack* stack)
{
    undoStack_ = stack;
}

void CommandStack::setContext(const CommandContext& context)
{
    context_ = context;
}

Command* CommandStack::push(std::unique_ptr<Command> command)
{
    if (!undoStack_ || !command)
        return nullptr;
    command->setContext(context_);
    Command* raw = command.get();
    undoStack_->push(command.release());
    return raw;
}

} // namespace Core

