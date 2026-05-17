#include <memory>

#include <SFML/System/Vector2.hpp>

#include "components/Card.hpp"
#include "components/Chip.hpp"
#include "core/Action.hpp"
#include "core/GameObject.hpp"
#include "core/Model.hpp"
#include "scene/DefaultFactory.hpp"
#include <gtest/gtest.h>

using dice::core::Action;
using dice::core::ActionFactory;
using dice::core::ActionResult;
using dice::core::ActionType;
using dice::core::CompositeAction;
using dice::core::FlipCardAction;
using dice::core::Model;
using dice::core::MoveObjectAction;

using dice::components::Card;
using dice::components::Chip;

using dice::scene::makeDefaultFactory;

// ========== Fixture ==========

class ActionTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto factory = makeDefaultFactory();
        model.setFactory(factory);

        chip = std::make_shared<Chip>("chip1", "Test Chip");
        chip->setPosition(100, 100);
        chip->setDraggable(true);
        model.addRootObject(chip);

        card = std::make_shared<Card>("card1", "Test Card");
        card->setPosition(200, 200);
        model.addRootObject(card);

        dummyTex.create(100, 140);
        card->setFrontTexture(&dummyTex);
        card->setBackTexture(&dummyTex);
    }

    sf::Texture dummyTex;       // NOLINT
    Model model;                // NOLINT
    std::shared_ptr<Chip> chip; // NOLINT
    std::shared_ptr<Card> card; // NOLINT
};

// ========== MoveObjectAction Tests ==========

TEST_F(ActionTest, MoveObjectActionExecute) {
    const sf::Vector2f newPos(300, 400);
    MoveObjectAction action("chip1", newPos, "Move Chip");

    auto result = action.execute(model);

    EXPECT_EQ(result, ActionResult::Success);
    EXPECT_TRUE(action.isExecuted());
    EXPECT_EQ(chip->getPosition().x, 300);
    EXPECT_EQ(chip->getPosition().y, 400);
}

TEST_F(ActionTest, MoveObjectActionUndo) {
    const sf::Vector2f oldPos = chip->getPosition();
    const sf::Vector2f newPos(300, 400);

    MoveObjectAction action("chip1", newPos, "Move Chip");
    action.execute(model);
    EXPECT_EQ(chip->getPosition().x, 300);

    action.undo(model);
    EXPECT_FALSE(action.isExecuted());
    EXPECT_EQ(chip->getPosition().x, oldPos.x);
    EXPECT_EQ(chip->getPosition().y, oldPos.y);
}

TEST_F(ActionTest, MoveObjectActionCanExecute) {
    const MoveObjectAction action("chip1", {300, 400});
    EXPECT_TRUE(action.canExecute(model));
}

TEST_F(ActionTest, MoveObjectActionCannotExecuteWhenNotFound) {
    const MoveObjectAction action("nonexistent", {300, 400});
    EXPECT_FALSE(action.canExecute(model));
}

TEST_F(ActionTest, MoveObjectActionCannotExecuteWhenNotDraggable) {
    chip->setDraggable(false);
    const MoveObjectAction action("chip1", {300, 400});
    EXPECT_FALSE(action.canExecute(model));
}

TEST_F(ActionTest, MoveObjectActionUndoWithoutExecute) {
    MoveObjectAction action("chip1", {300, 400});
    auto result = action.undo(model);
    EXPECT_EQ(result, ActionResult::Invalid);
}

TEST_F(ActionTest, MoveObjectActionSerialization) {
    MoveObjectAction original("chip1", {300, 400}, "My Move");
    original.execute(model);

    auto json = original.toJson();
    MoveObjectAction restored;
    restored.fromJson(json);

    EXPECT_EQ(restored.getName(), "My Move");
    EXPECT_EQ(restored.getType(), ActionType::MoveObject);
}

// ========== FlipCardAction Tests ==========

TEST_F(ActionTest, FlipCardActionExecute) {
    const bool oldFaceUp = card->isFaceUp();

    FlipCardAction action("card1", "Flip");
    auto result = action.execute(model);

    EXPECT_EQ(result, ActionResult::Success);
    EXPECT_TRUE(action.isExecuted());
    EXPECT_EQ(card->isFaceUp(), !oldFaceUp);
}

TEST_F(ActionTest, FlipCardActionUndo) {
    const bool oldFaceUp = card->isFaceUp();

    FlipCardAction action("card1", "Flip");
    action.execute(model);
    EXPECT_EQ(card->isFaceUp(), !oldFaceUp);

    action.undo(model);
    EXPECT_FALSE(action.isExecuted());
    EXPECT_EQ(card->isFaceUp(), oldFaceUp);
}

TEST_F(ActionTest, FlipCardActionCanExecute) {
    const FlipCardAction action("card1");
    EXPECT_TRUE(action.canExecute(model));
}

TEST_F(ActionTest, FlipCardActionCannotExecuteOnChip) {
    const FlipCardAction action("chip1");
    EXPECT_FALSE(action.canExecute(model));
}

TEST_F(ActionTest, FlipCardActionSerialization) {
    FlipCardAction original("card1", "My Flip");
    original.execute(model);

    auto json = original.toJson();
    FlipCardAction restored;
    restored.fromJson(json);

    EXPECT_EQ(restored.getName(), "My Flip");
    EXPECT_EQ(restored.getType(), ActionType::FlipCard);
}

// ========== CompositeAction Tests ==========

TEST_F(ActionTest, CompositeActionExecuteMultipleActions) {
    CompositeAction composite("Move and Flip");

    composite.addAction(std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(300, 400)));
    composite.addAction(std::make_unique<FlipCardAction>("card1"));

    auto result = composite.execute(model);

    EXPECT_EQ(result, ActionResult::Success);
    EXPECT_TRUE(composite.isExecuted());
    EXPECT_EQ(chip->getPosition().x, 300);
    EXPECT_EQ(chip->getPosition().y, 400);
    EXPECT_FALSE(!card->isFaceUp());
}

TEST_F(ActionTest, CompositeActionUndo) {
    const sf::Vector2f oldChipPos = chip->getPosition();
    const bool oldCardFaceUp = card->isFaceUp();

    CompositeAction composite("Move and Flip");
    composite.addAction(std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(300, 400)));
    composite.addAction(std::make_unique<FlipCardAction>("card1"));

    composite.execute(model);
    composite.undo(model);

    EXPECT_FALSE(composite.isExecuted());
    EXPECT_EQ(chip->getPosition().x, oldChipPos.x);
    EXPECT_EQ(chip->getPosition().y, oldChipPos.y);
    EXPECT_EQ(card->isFaceUp(), oldCardFaceUp);
}

TEST_F(ActionTest, CompositeActionPartialFailure) {
    CompositeAction composite("Partial Failure");

    composite.addAction(std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(300, 400)));
    composite.addAction(std::make_unique<FlipCardAction>("nonexistent_card"));
    composite.addAction(std::make_unique<FlipCardAction>("card1"));

    auto result = composite.execute(model);

    EXPECT_EQ(result, ActionResult::Failed);
    EXPECT_FALSE(composite.isExecuted());
    EXPECT_EQ(chip->getPosition().x, 100);
}

TEST_F(ActionTest, CompositeActionCanExecute) {
    CompositeAction composite("Valid");
    composite.addAction(std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(300, 400)));
    composite.addAction(std::make_unique<FlipCardAction>("card1"));

    EXPECT_TRUE(composite.canExecute(model));
}

TEST_F(ActionTest, CompositeActionCannotExecuteIfAnyFails) {
    CompositeAction composite("Invalid");
    composite.addAction(std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(300, 400)));
    composite.addAction(std::make_unique<FlipCardAction>("nonexistent_card"));

    EXPECT_FALSE(composite.canExecute(model));
}

TEST_F(ActionTest, CompositeActionAddActionWithCustomName) {
    CompositeAction composite("Parent");
    composite.addAction(std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(300, 400)),
                        "Move Step");

    EXPECT_EQ(composite.getActionCount(), 1);
    EXPECT_EQ(composite.getAction(0)->getName(), "Move Step");
}

TEST_F(ActionTest, CompositeActionSerialization) {
    CompositeAction original("Test Composite");
    original.addAction(std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(300, 400)), "Move");
    original.addAction(std::make_unique<FlipCardAction>("card1"), "Flip");

    auto json = original.toJson();
    CompositeAction restored;
    restored.fromJson(json);

    EXPECT_EQ(restored.getName(), "Test Composite");
    EXPECT_EQ(restored.getActionCount(), 2);
    EXPECT_EQ(restored.getType(), ActionType::Composite);
}

// ========== ActionFactory Tests ==========

TEST_F(ActionTest, ActionFactoryCreatesMoveAction) {
    auto action = ActionFactory::createAction(ActionType::MoveObject);
    ASSERT_NE(action, nullptr);
    EXPECT_EQ(action->getType(), ActionType::MoveObject);
}

TEST_F(ActionTest, ActionFactoryCreatesFlipAction) {
    auto action = ActionFactory::createAction(ActionType::FlipCard);
    ASSERT_NE(action, nullptr);
    EXPECT_EQ(action->getType(), ActionType::FlipCard);
}

TEST_F(ActionTest, ActionFactoryCreatesCompositeAction) {
    auto action = ActionFactory::createAction(ActionType::Composite);
    ASSERT_NE(action, nullptr);
    EXPECT_EQ(action->getType(), ActionType::Composite);
}

TEST_F(ActionTest, ActionFactoryReturnsNullForUnknownType) {
    auto action = ActionFactory::createAction(static_cast<ActionType>(999));
    EXPECT_EQ(action, nullptr);
}

TEST_F(ActionTest, ActionFactoryCreatesFromJson) {
    const MoveObjectAction original("chip1", sf::Vector2f(300, 400), "Move");
    auto json = original.toJson();

    auto restored = ActionFactory::createFromJson(json);
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(restored->getType(), ActionType::MoveObject);
    EXPECT_EQ(restored->getName(), "Move");
}

TEST_F(ActionTest, ActionFactoryReturnsNullForInvalidJson) {
    nlohmann::json invalidJson;
    invalidJson["something"] = "else";

    auto action = ActionFactory::createFromJson(invalidJson);
    EXPECT_EQ(action, nullptr);
}

// ========== Base Action Tests ==========

TEST_F(ActionTest, BaseActionSetName) {
    class TestAction : public Action {
    public:
        ActionResult execute(Model&) override {
            return ActionResult::Success;
        }
        ActionResult undo(Model&) override {
            return ActionResult::Success;
        }
        [[nodiscard]] bool canExecute(const Model&) const override {
            return true;
        }
    };

    TestAction action;
    action.setName("Test Name");
    EXPECT_EQ(action.getName(), "Test Name");
}

TEST_F(ActionTest, BaseActionTimestamp) {
    class TestAction : public Action {
    public:
        ActionResult execute(Model&) override {
            return ActionResult::Success;
        }
        ActionResult undo(Model&) override {
            return ActionResult::Success;
        }
        [[nodiscard]] bool canExecute(const Model&) const override {
            return true;
        }
    };

    TestAction action;
    action.setTimestamp();
    auto timestamp = action.getTimestamp();
    EXPECT_GT(timestamp.time_since_epoch().count(), 0);
}

// ========== Integration Tests ==========

TEST_F(ActionTest, ExecuteSequenceOfActions) {
    MoveObjectAction move1("chip1", sf::Vector2f(150, 150));
    MoveObjectAction move2("chip1", sf::Vector2f(200, 200));

    move1.execute(model);
    EXPECT_EQ(chip->getPosition().x, 150);

    move2.execute(model);
    EXPECT_EQ(chip->getPosition().x, 200);

    move2.undo(model);
    EXPECT_EQ(chip->getPosition().x, 150);

    move1.undo(model);
    EXPECT_EQ(chip->getPosition().x, 100);
}

TEST_F(ActionTest, FlipThenMove) {
    FlipCardAction flip("card1");
    MoveObjectAction move("card1", sf::Vector2f(300, 300));

    flip.execute(model);
    EXPECT_FALSE(!card->isFaceUp());

    move.execute(model);
    EXPECT_EQ(card->getPosition().x, 300);

    move.undo(model);
    EXPECT_EQ(card->getPosition().x, 200);

    flip.undo(model);
    EXPECT_TRUE(!card->isFaceUp());
}
