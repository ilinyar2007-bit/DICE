#ifndef DICE_ACTION_MANAGER_HPP
#define DICE_ACTION_MANAGER_HPP

#include <deque>

#include <nlohmann/json.hpp>

namespace dice::core {

class Model;

class ActionManager {
public:
    ActionManager() = default;

    void saveSnapshot(const Model& model);

    bool undo(Model& model);
    bool redo(Model& model);

    bool canUndo() const {
        return !undoStack_.empty();
    }
    bool canRedo() const {
        return !redoStack_.empty();
    }
    size_t getUndoCount() const {
        return undoStack_.size();
    }
    size_t getRedoCount() const {
        return redoStack_.size();
    }

    void clear();
    void setMaxHistorySize(size_t size) {
        maxHistorySize_ = size;
    }

private:
    void trimHistory();

    std::deque<nlohmann::json> undoStack_;
    std::deque<nlohmann::json> redoStack_;
    size_t maxHistorySize_ = 50;
};

} // namespace dice::core

#endif
