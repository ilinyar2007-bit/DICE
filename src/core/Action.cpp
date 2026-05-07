#include "core/Action.hpp"

#include "components/Card.hpp"
#include "core/GameObject.hpp"
#include "core/Model.hpp"
#include <spdlog/spdlog.h>

namespace dice::core {

// ========== The Action base class ==========

nlohmann::json Action::toJson() const {
    nlohmann::json json;
    json["name"] = name_;
    json["type"] = static_cast<int>(getType());
    json["executed"] = executed_;
    return json;
}

void Action::fromJson(const nlohmann::json& json) {
    if (json.contains("name"))
        name_ = json["name"];
    if (json.contains("executed"))
        executed_ = json["executed"];
}

// ========== MoveObjectAction ==========

MoveObjectAction::MoveObjectAction(const std::string& objectId,
                                   const sf::Vector2f& newPosition,
                                   const std::string& name)
    : Action(name), objectId_(objectId), newPosition_(newPosition) {}

ActionResult MoveObjectAction::execute(Model& model) {
    auto obj = model.getObject(objectId_);
    if (!obj) {
        spdlog::error("MoveObjectAction: Object {} not found", objectId_);
        return ActionResult::Failed;
    }

    oldPosition_ = obj->getPosition();

    obj->setPosition(newPosition_);
    executed_ = true;

    spdlog::debug("Moved object {} to ({}, {})", objectId_, newPosition_.x, newPosition_.y);
    return ActionResult::Success;
}

ActionResult MoveObjectAction::undo(Model& model) {
    if (!executed_)
        return ActionResult::Invalid;

    auto obj = model.getObject(objectId_);
    if (!obj) {
        spdlog::error("MoveObjectAction undo: Object {} not found", objectId_);
        return ActionResult::Failed;
    }

    obj->setPosition(oldPosition_);
    spdlog::debug("Undo move of object {} to ({}, {})", objectId_, oldPosition_.x, oldPosition_.y);
    return ActionResult::Success;
}

bool MoveObjectAction::canExecute(const Model& model) const {
    auto obj = model.getObject(objectId_);
    return obj && obj->isDraggable();
}

nlohmann::json MoveObjectAction::toJson() const {
    auto json = Action::toJson();
    json["objectId"] = objectId_;
    json["newPosition"] = {newPosition_.x, newPosition_.y};
    json["oldPosition"] = {oldPosition_.x, oldPosition_.y};
    return json;
}

void MoveObjectAction::fromJson(const nlohmann::json& json) {
    Action::fromJson(json);
    if (json.contains("objectId"))
        objectId_ = json["objectId"];
    if (json.contains("newPosition")) {
        newPosition_.x = json["newPosition"][0];
        newPosition_.y = json["newPosition"][1];
    }
    if (json.contains("oldPosition")) {
        oldPosition_.x = json["oldPosition"][0];
        oldPosition_.y = json["oldPosition"][1];
    }
}

// ========== FlipCardAction ==========

FlipCardAction::FlipCardAction(const std::string& cardId, const std::string& name)
    : Action(name), cardId_(cardId) {}

ActionResult FlipCardAction::execute(Model& model) {
    auto obj = model.getObject(cardId_);
    if (!obj || obj->getType() != "Card") {
        spdlog::error("FlipCardAction: Object {} is not a card", cardId_);
        return ActionResult::Failed;
    }

    auto card = std::dynamic_pointer_cast<components::Card>(obj);
    if (!card)
        return ActionResult::Failed;

    oldFaceUp_ = card->isFaceUp();
    newFaceUp_ = !oldFaceUp_;

    card->setFaceUp(newFaceUp_);
    executed_ = true;

    spdlog::debug("Flipped card {}: {} -> {}", cardId_, oldFaceUp_, newFaceUp_);
    return ActionResult::Success;
}

ActionResult FlipCardAction::undo(Model& model) {
    if (!executed_)
        return ActionResult::Invalid;

    auto obj = model.getObject(cardId_);
    if (!obj)
        return ActionResult::Failed;

    auto card = std::dynamic_pointer_cast<components::Card>(obj);
    if (!card)
        return ActionResult::Failed;

    card->setFaceUp(oldFaceUp_);
    return ActionResult::Success;
}

bool FlipCardAction::canExecute(const Model& model) const {
    auto obj = model.getObject(cardId_);
    return obj && obj->getType() == "Card";
}

nlohmann::json FlipCardAction::toJson() const {
    auto json = Action::toJson();
    json["cardId"] = cardId_;
    json["oldFaceUp"] = oldFaceUp_;
    json["newFaceUp"] = newFaceUp_;
    return json;
}

void FlipCardAction::fromJson(const nlohmann::json& json) {
    Action::fromJson(json);
    if (json.contains("cardId"))
        cardId_ = json["cardId"];
    if (json.contains("oldFaceUp"))
        oldFaceUp_ = json["oldFaceUp"];
    if (json.contains("newFaceUp"))
        newFaceUp_ = json["newFaceUp"];
}

// ========== CompositeAction ==========

CompositeAction::CompositeAction(const std::string& name) : Action(name) {}

void CompositeAction::addAction(std::unique_ptr<Action> action) {
    actions_.push_back(std::move(action));
}

void CompositeAction::addAction(std::unique_ptr<Action> action, const std::string& name) {
    action->setName(name);
    actions_.push_back(std::move(action));
}

ActionResult CompositeAction::execute(Model& model) {
    executionResults_.clear();
    ActionResult overall = ActionResult::Success;

    for (auto& action : actions_) {
        auto result = action->execute(model);
        executionResults_.push_back(result == ActionResult::Success);

        if (result != ActionResult::Success) {
            overall = ActionResult::Partial;
            spdlog::warn("Composite action sub-action failed: {}", action->getName());
        }
    }

    executed_ = true;
    return overall;
}

ActionResult CompositeAction::undo(Model& model) {
    if (!executed_)
        return ActionResult::Invalid;

    for (auto it = actions_.rbegin(); it != actions_.rend(); ++it) {
        (*it)->undo(model);
    }

    return ActionResult::Success;
}

bool CompositeAction::canExecute(const Model& model) const {
    for (const auto& action : actions_) {
        if (!action->canExecute(model))
            return false;
    }
    return true;
}

nlohmann::json CompositeAction::toJson() const {
    auto json = Action::toJson();
    json["actions"] = nlohmann::json::array();
    for (const auto& action : actions_) {
        json["actions"].push_back(action->toJson());
    }
    return json;
}

void CompositeAction::fromJson(const nlohmann::json& json) {
    Action::fromJson(json);
    if (json.contains("actions")) {
        for (const auto& actionJson : json["actions"]) {
            auto action = ActionFactory::createFromJson(actionJson);
            if (action) {
                actions_.push_back(std::move(action));
            }
        }
    }
}

// ========== ActionFactory ==========

std::unique_ptr<Action> ActionFactory::createAction(ActionType type) {
    switch (type) {
        case ActionType::MoveObject:
            return std::make_unique<MoveObjectAction>();
        case ActionType::FlipCard:
            return std::make_unique<FlipCardAction>();
        case ActionType::Composite:
            return std::make_unique<CompositeAction>();
        default:
            return nullptr;
    }
}

std::unique_ptr<Action> ActionFactory::createFromJson(const nlohmann::json& json) {
    if (!json.contains("type"))
        return nullptr;

    auto type = static_cast<ActionType>(json["type"].get<int>());
    auto action = createAction(type);

    if (action) {
        action->fromJson(json);
    }

    return action;
}

} // namespace dice::core