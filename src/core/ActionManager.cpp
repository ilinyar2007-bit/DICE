#include "core/ActionManager.hpp"

#include "core/Model.hpp"
#include <spdlog/spdlog.h>

namespace dice::core {

bool ActionManager::execute(std::unique_ptr<Action> action) {
    if (!action)
        return false;

    redoDeque_.clear();

    if (action->execute(model_) != ActionResult::Success) {
        spdlog::error("Action execution failed: {}", action->getName());
        return false;
    }

    action->setTimestamp();
    undoDeque_.push_back(std::move(action));

    trimHistory();

    spdlog::debug(
        "Action executed: {}, history size: {}", undoDeque_.back()->getName(), undoDeque_.size());
    return true;
}

bool ActionManager::undo() {
    if (undoDeque_.empty())
        return false;

    auto action = std::move(undoDeque_.back());
    undoDeque_.pop_back();

    if (action->undo(model_) != ActionResult::Success) {
        spdlog::error("Undo failed for: {}", action->getName());
        return false;
    }

    redoDeque_.push_back(std::move(action));
    spdlog::debug("Undo executed, history size: {}", undoDeque_.size());
    return true;
}

bool ActionManager::redo() {
    if (redoDeque_.empty())
        return false;

    auto action = std::move(redoDeque_.back());
    redoDeque_.pop_back();

    if (action->execute(model_) != ActionResult::Success) {
        spdlog::error("Redo failed for: {}", action->getName());
        return false;
    }

    undoDeque_.push_back(std::move(action));
    spdlog::debug("Redo executed, history size: {}", undoDeque_.size());
    return true;
}

void ActionManager::clear() {
    if (undoDeque_.empty() && redoDeque_.empty())
        return;
    undoDeque_.clear();
    redoDeque_.clear();
    spdlog::debug("Action history cleared");
}

void ActionManager::trimHistory() {
    while (undoDeque_.size() > maxHistorySize_) {
        undoDeque_.pop_front();
    }
}

} // namespace dice::core