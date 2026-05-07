#ifndef DICE_ACTION_MANAGER_HPP
#define DICE_ACTION_MANAGER_HPP

#include <memory>
#include <stack>
#include <vector>

#include "core/Action.hpp"

namespace dice::core {

class Model;

class ActionManager {
public:
    ActionManager() = default;
    ~ActionManager() = default;

    bool execute(std::unique_ptr<Action> action);

    bool undo();

    bool redo();

    void clear();

    bool canUndo() const {
        return !undoDeque_.empty();
    }
    bool canRedo() const {
        return !redoDeque_.empty();
    }

    size_t getUndoCount() const {
        return undoDeque_.size();
    }
    size_t getRedoCount() const {
        return redoDeque_.size();
    }

    Action* getLastAction() const {
        return undoDeque_.empty() ? nullptr : undoDeque_.back().get();
    }

    void setMaxHistorySize(size_t maxSize) {
        maxHistorySize_ = maxSize;
    }

private:
    void trimHistory();

    Model& model_;
    std::deque<std::unique_ptr<Action>> undoDeque_;
    std::deque<std::unique_ptr<Action>> redoDeque_;
    size_t maxHistorySize_ = 100;
};

} // namespace dice::core

#endif