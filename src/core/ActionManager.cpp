#include "core/ActionManager.hpp"

#include "core/Model.hpp"
#include <spdlog/spdlog.h>

namespace dice::core {

void ActionManager::saveSnapshot(const Model& model) {
    redoStack_.clear();
    undoStack_.push_back(model.toJson());
    trimHistory();
    spdlog::debug("Snapshot saved, undo stack: {}", undoStack_.size());
}

bool ActionManager::undo(Model& model) {
    if (undoStack_.empty()) {
        return false;
    }
    redoStack_.push_back(model.toJson());
    model.fromJson(undoStack_.back());
    undoStack_.pop_back();
    spdlog::debug("Undo: undo={}, redo={}", undoStack_.size(), redoStack_.size());
    return true;
}

bool ActionManager::redo(Model& model) {
    if (redoStack_.empty()) {
        return false;
    }
    undoStack_.push_back(model.toJson());
    model.fromJson(redoStack_.back());
    redoStack_.pop_back();
    spdlog::debug("Redo: undo={}, redo={}", undoStack_.size(), redoStack_.size());
    return true;
}

void ActionManager::clear() {
    undoStack_.clear();
    redoStack_.clear();
    spdlog::debug("History cleared");
}

void ActionManager::trimHistory() {
    while (undoStack_.size() > maxHistorySize_) {
        undoStack_.pop_front();
    }
}

} // namespace dice::core
