#include <memory>

#include <SFML/System/Vector2.hpp>

#include "components/Card.hpp"
#include "components/Chip.hpp"
#include "core/ActionManager.hpp"
#include "core/GameObject.hpp"
#include "core/Model.hpp"
#include "scene/DefaultFactory.hpp"
#include <gtest/gtest.h>

using dice::core::Action;
using dice::core::ActionManager;
using dice::core::ActionResult;
using dice::core::CompositeAction;
using dice::core::FlipCardAction;
using dice::core::Model;
using dice::core::MoveObjectAction;

using dice::components::Card;
using dice::components::Chip;

using dice::scene::makeDefaultFactory;

// ========== Test Fixture ==========

class ActionManagerTest : public ::testing::Test {
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

        sf::Texture dummyTex;
        dummyTex.create(100, 140);
        card->setFrontTexture(&dummyTex);
        card->setBackTexture(&dummyTex);

        actionManager = std::make_unique<ActionManager>(model);
    }

    Model model;                                  // NOLINT
    std::shared_ptr<Chip> chip;                   // NOLINT
    std::shared_ptr<Card> card;                   // NOLINT
    std::unique_ptr<ActionManager> actionManager; // NOLINT
};

// ========== Basic Execution Tests ==========

TEST_F(ActionManagerTest, ExecuteSingleAction) {
    auto action = std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(300, 400), "Move");

    const bool result = actionManager->execute(std::move(action));

    EXPECT_TRUE(result);
    EXPECT_EQ(chip->getPosition().x, 300);
    EXPECT_EQ(chip->getPosition().y, 400);
    EXPECT_TRUE(actionManager->canUndo());
    EXPECT_FALSE(actionManager->canRedo());
    EXPECT_EQ(actionManager->getUndoCount(), 1);
    EXPECT_EQ(actionManager->getRedoCount(), 0);
}

TEST_F(ActionManagerTest, ExecuteNullAction) {
    const bool result = actionManager->execute(nullptr);

    EXPECT_FALSE(result);
    EXPECT_FALSE(actionManager->canUndo());
}

TEST_F(ActionManagerTest, ExecuteActionThatCannotExecute) {
    auto action = std::make_unique<MoveObjectAction>("nonexistent", sf::Vector2f(300, 400));

    const bool result = actionManager->execute(std::move(action));

    EXPECT_FALSE(result);
    EXPECT_FALSE(actionManager->canUndo());
}

TEST_F(ActionManagerTest, ExecuteMultipleActions) {
    auto action1 = std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(150, 150));
    auto action2 = std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(200, 200));

    actionManager->execute(std::move(action1));
    actionManager->execute(std::move(action2));

    EXPECT_EQ(chip->getPosition().x, 200);
    EXPECT_EQ(chip->getPosition().y, 200);
    EXPECT_EQ(actionManager->getUndoCount(), 2);
}

// ========== Undo Tests ==========

TEST_F(ActionManagerTest, UndoAfterExecute) {
    auto action = std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(300, 400));
    actionManager->execute(std::move(action));

    const bool result = actionManager->undo();

    EXPECT_TRUE(result);
    EXPECT_EQ(chip->getPosition().x, 100);
    EXPECT_EQ(chip->getPosition().y, 100);
    EXPECT_FALSE(actionManager->canUndo());
    EXPECT_TRUE(actionManager->canRedo());
    EXPECT_EQ(actionManager->getUndoCount(), 0);
    EXPECT_EQ(actionManager->getRedoCount(), 1);
}

TEST_F(ActionManagerTest, UndoWhenEmpty) {
    const bool result = actionManager->undo();

    EXPECT_FALSE(result);
}

TEST_F(ActionManagerTest, UndoTwice) {
    auto action = std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(300, 400));
    actionManager->execute(std::move(action));

    actionManager->undo();
    const bool secondUndo = actionManager->undo();

    EXPECT_FALSE(secondUndo);
    EXPECT_FALSE(actionManager->canUndo());
    EXPECT_TRUE(actionManager->canRedo());
}

// ========== Redo Tests ==========

TEST_F(ActionManagerTest, RedoAfterUndo) {
    auto action = std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(300, 400));
    actionManager->execute(std::move(action));
    actionManager->undo();

    const bool result = actionManager->redo();

    EXPECT_TRUE(result);
    EXPECT_EQ(chip->getPosition().x, 300);
    EXPECT_TRUE(actionManager->canUndo());
    EXPECT_FALSE(actionManager->canRedo());
}

TEST_F(ActionManagerTest, RedoWhenEmpty) {
    const bool result = actionManager->redo();

    EXPECT_FALSE(result);
}

TEST_F(ActionManagerTest, RedoAfterNewActionClearsRedo) {
    auto action1 = std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(150, 150));
    auto action2 = std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(200, 200));

    actionManager->execute(std::move(action1));
    actionManager->undo();
    EXPECT_TRUE(actionManager->canRedo());

    actionManager->execute(std::move(action2));

    EXPECT_FALSE(actionManager->canRedo());
    EXPECT_EQ(actionManager->getRedoCount(), 0);
    EXPECT_EQ(actionManager->getUndoCount(), 1);
    EXPECT_EQ(chip->getPosition().x, 200);
}

// ========== Clear Tests ==========

TEST_F(ActionManagerTest, ClearHistory) {
    auto action1 = std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(150, 150));
    auto action2 = std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(200, 200));

    actionManager->execute(std::move(action1));
    actionManager->execute(std::move(action2));

    actionManager->clear();

    EXPECT_FALSE(actionManager->canUndo());
    EXPECT_FALSE(actionManager->canRedo());
    EXPECT_EQ(actionManager->getUndoCount(), 0);
    EXPECT_EQ(actionManager->getRedoCount(), 0);
    EXPECT_EQ(chip->getPosition().x, 200);
}

TEST_F(ActionManagerTest, ClearEmptyHistory) {
    actionManager->clear();
    EXPECT_FALSE(actionManager->canUndo());
    EXPECT_FALSE(actionManager->canRedo());
}

// ========== History Limit Tests ==========

TEST_F(ActionManagerTest, HistoryLimitRespected) {
    actionManager->setMaxHistorySize(3);

    for (int i = 0; i < 5; ++i) {
        auto action = std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(100 + i * 10, 100));
        actionManager->execute(std::move(action));
    }

    EXPECT_EQ(actionManager->getUndoCount(), 3);

    actionManager->undo();
    actionManager->undo();
    actionManager->undo();
    EXPECT_FALSE(actionManager->canUndo());
}

// ========== FlipCardAction with ActionManager ==========

TEST_F(ActionManagerTest, ExecuteFlipCardAction) {
    const bool oldFaceUp = card->isFaceUp();

    auto action = std::make_unique<FlipCardAction>("card1", "Flip");
    actionManager->execute(std::move(action));

    EXPECT_EQ(card->isFaceUp(), !oldFaceUp);
    EXPECT_TRUE(actionManager->canUndo());
}

TEST_F(ActionManagerTest, UndoFlipCardAction) {
    const bool oldFaceUp = card->isFaceUp();

    auto action = std::make_unique<FlipCardAction>("card1", "Flip");
    actionManager->execute(std::move(action));
    EXPECT_EQ(card->isFaceUp(), !oldFaceUp);

    actionManager->undo();
    EXPECT_EQ(card->isFaceUp(), oldFaceUp);
}

TEST_F(ActionManagerTest, RedoFlipCardAction) {
    const bool oldFaceUp = card->isFaceUp();

    auto action = std::make_unique<FlipCardAction>("card1", "Flip");
    actionManager->execute(std::move(action));
    actionManager->undo();
    EXPECT_EQ(card->isFaceUp(), oldFaceUp);

    actionManager->redo();
    EXPECT_EQ(card->isFaceUp(), !oldFaceUp);
}

// ========== CompositeAction with ActionManager ==========

TEST_F(ActionManagerTest, ExecuteCompositeAction) {
    auto composite = std::make_unique<CompositeAction>("Move and Flip");
    composite->addAction(std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(300, 400)));
    composite->addAction(std::make_unique<FlipCardAction>("card1"));

    actionManager->execute(std::move(composite));

    EXPECT_EQ(chip->getPosition().x, 300);
    EXPECT_TRUE(card->isFaceUp());
    EXPECT_TRUE(actionManager->canUndo());
}

TEST_F(ActionManagerTest, UndoCompositeAction) {
    const sf::Vector2f oldPos = chip->getPosition();
    const bool oldFaceUp = card->isFaceUp();

    auto composite = std::make_unique<CompositeAction>("Move and Flip");
    composite->addAction(std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(300, 400)));
    composite->addAction(std::make_unique<FlipCardAction>("card1"));

    actionManager->execute(std::move(composite));
    actionManager->undo();

    EXPECT_EQ(chip->getPosition().x, oldPos.x);
    EXPECT_EQ(card->isFaceUp(), oldFaceUp);
}

// ========== GetLastAction Tests ==========

TEST_F(ActionManagerTest, GetLastAction) {
    EXPECT_EQ(actionManager->getLastAction(), nullptr);

    auto action = std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(300, 400), "Last Move");
    actionManager->execute(std::move(action));

    ASSERT_NE(actionManager->getLastAction(), nullptr);
    EXPECT_EQ(actionManager->getLastAction()->getName(), "Last Move");
}

TEST_F(ActionManagerTest, GetLastActionAfterUndo) {
    auto action = std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(300, 400));
    actionManager->execute(std::move(action));

    actionManager->undo();

    EXPECT_EQ(actionManager->getLastAction(), nullptr);
}

// ========== Integration Tests ==========

TEST_F(ActionManagerTest, ComplexSequence) {
    // Execute: move chip
    auto move1 = std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(150, 150));
    actionManager->execute(std::move(move1));
    EXPECT_EQ(chip->getPosition().x, 150);

    // Execute: flip card
    auto flip = std::make_unique<FlipCardAction>("card1");
    actionManager->execute(std::move(flip));
    EXPECT_TRUE(card->isFaceUp());

    // Execute: move chip again
    auto move2 = std::make_unique<MoveObjectAction>("chip1", sf::Vector2f(200, 200));
    actionManager->execute(std::move(move2));
    EXPECT_EQ(chip->getPosition().x, 200);

    // Undo last move
    actionManager->undo();
    EXPECT_EQ(chip->getPosition().x, 150);

    // Undo flip
    actionManager->undo();
    EXPECT_FALSE(card->isFaceUp());

    // Redo flip
    actionManager->redo();
    EXPECT_TRUE(card->isFaceUp());

    // Redo move
    actionManager->redo();
    EXPECT_EQ(chip->getPosition().x, 200);
}

TEST_F(ActionManagerTest, ActionWithInvalidObject) {
    auto action = std::make_unique<MoveObjectAction>("nonexistent", sf::Vector2f(300, 400));

    const bool result = actionManager->execute(std::move(action));

    EXPECT_FALSE(result);
    EXPECT_FALSE(actionManager->canUndo());
}
