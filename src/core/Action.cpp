#include "core/Action.hpp"

#include "core/GameObject.hpp"
#include "core/Model.hpp"
#include <spdlog/spdlog.h>

namespace dice::core {

MoveObjectAction::MoveObjectAction(std::string object_id, sf::Vector2f new_pos)
    : objectId_(std::move(object_id)), newPos_(new_pos) {}

bool MoveObjectAction::execute(Model& model) {
    auto obj = model.getObject(objectId_);
    if (!obj) {
        spdlog::error("MoveObjectAction: object '{}' not found", objectId_);
        return false;
    }
    obj->setPosition(newPos_);
    spdlog::debug("Moved '{}' to ({}, {})", objectId_, newPos_.x, newPos_.y);
    return true;
}

bool MoveObjectAction::canExecute(const Model& model) const {
    auto obj = model.getObject(objectId_);
    return obj && obj->isDraggable();
}

} // namespace dice::core
