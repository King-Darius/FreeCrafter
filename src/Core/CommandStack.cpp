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

void CommandStack::push(std::unique_ptr<Command> command)
{
    if (!undoStack_ || !command)
        return;
    command->setContext(context_);
    undoStack_->push(command.release());
}

} // namespace Core

