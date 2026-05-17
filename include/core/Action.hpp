#ifndef DICE_ACTION_HPP
#define DICE_ACTION_HPP

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <SFML/System/Vector2.hpp>
#include <nlohmann/json.hpp>

namespace dice::core {

class Model;
class GameObject;

// ========== Types of actions ==========

enum class ActionType { Unknown, MoveObject, FlipCard, Composite };

// ========== Execution result ==========

enum class ActionResult { Success, Failed, Invalid, Partial };

// ========== The Action base class ==========

class Action {
public:
    Action() = default;
    explicit Action(const std::string& name) : name_(name) {}
    virtual ~Action() = default;

    virtual ActionResult execute(Model& model) = 0;

    virtual ActionResult undo(Model& model) = 0;

    virtual bool canExecute(const Model& model) const = 0;

    virtual std::string getName() const {
        return name_;
    }
    void setName(const std::string& name) {
        name_ = name;
    }

    virtual ActionType getType() const {
        return ActionType::Unknown;
    }

    virtual nlohmann::json toJson() const;
    virtual void fromJson(const nlohmann::json& json);

    bool isExecuted() const {
        return executed_;
    }
    void setTimestamp() {
        timestamp_ = std::chrono::steady_clock::now();
    }
    auto getTimestamp() const {
        return timestamp_;
    }

protected:
    std::string name_;
    bool executed_ = false;
    std::chrono::steady_clock::time_point timestamp_;
};

// ========== Action: moving an object ==========

class MoveObjectAction : public Action {
public:
    MoveObjectAction() = default;
    MoveObjectAction(const std::string& objectId,
                     const sf::Vector2f& newPosition,
                     const std::string& name = "Move Object");

    ActionResult execute(Model& model) override;
    ActionResult undo(Model& model) override;
    bool canExecute(const Model& model) const override;
    ActionType getType() const override {
        return ActionType::MoveObject;
    }

    nlohmann::json toJson() const override;
    void fromJson(const nlohmann::json& json) override;

private:
    std::string objectId_;
    sf::Vector2f newPosition_;
    sf::Vector2f oldPosition_;
};

// ========== Action: Flip the card ==========

class FlipCardAction : public Action {
public:
    FlipCardAction() = default;
    explicit FlipCardAction(const std::string& cardId, const std::string& name = "Flip Card");

    ActionResult execute(Model& model) override;
    ActionResult undo(Model& model) override;
    bool canExecute(const Model& model) const override;
    ActionType getType() const override {
        return ActionType::FlipCard;
    }

    nlohmann::json toJson() const override;
    void fromJson(const nlohmann::json& json) override;

private:
    std::string cardId_;
    bool oldFaceUp_ = false;
    bool newFaceUp_ = false;
};

// ========== Composite action ==========

class CompositeAction : public Action {
public:
    CompositeAction() = default;
    explicit CompositeAction(const std::string& name);

    void addAction(std::unique_ptr<Action> action);
    void addAction(std::unique_ptr<Action> action, const std::string& name);

    ActionResult execute(Model& model) override;
    ActionResult undo(Model& model) override;
    bool canExecute(const Model& model) const override;
    ActionType getType() const override {
        return ActionType::Composite;
    }

    size_t getActionCount() const {
        return actions_.size();
    }
    Action* getAction(size_t index) {
        return actions_[index].get();
    }

    nlohmann::json toJson() const override;
    void fromJson(const nlohmann::json& json) override;

private:
    std::vector<std::unique_ptr<Action>> actions_;
    std::vector<bool> executionResults_;
};

// ========== Action Factory ==========

class ActionFactory {
public:
    static std::unique_ptr<Action> createAction(ActionType type);
    static std::unique_ptr<Action> createFromJson(const nlohmann::json& json);
};

} // namespace dice::core

#endif