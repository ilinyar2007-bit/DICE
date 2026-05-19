#include <memory>

#include <SFML/System/Vector2.hpp>

#include "components/Card.hpp"
#include "components/Chip.hpp"
#include "core/ActionManager.hpp"
#include "core/Model.hpp"
#include "scene/DefaultFactory.hpp"
#include <gtest/gtest.h>

using dice::components::Card;
using dice::components::Chip;
using dice::core::ActionManager;
using dice::core::Model;
using dice::scene::makeDefaultFactory;

class ActionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        model.setFactory(makeDefaultFactory());

        auto chip = std::make_shared<Chip>("chip1", "Test Chip");
        chip->setPosition(100, 100);
        chip->setDraggable(true);
        model.addRootObject(chip);

        auto card = std::make_shared<Card>("card1", "Test Card");
        card->setPosition(200, 200);
        card->setFaceUp(false);
        sf::Texture dummyTex;
        dummyTex.create(100, 140);
        card->setFrontTexture(&dummyTex);
        card->setBackTexture(&dummyTex);
        model.addRootObject(card);
    }

    // Re-fetch after fromJson invalidates old shared_ptrs
    sf::Vector2f chipPos() {
        return model.getObject("chip1")->getPosition();
    }
    bool cardFaceUp() {
        return std::dynamic_pointer_cast<Card>(model.getObject("card1"))->isFaceUp();
    }

    Model model;       // NOLINT
    ActionManager mgr; // NOLINT
};

// ========== saveSnapshot + undo ==========

TEST_F(ActionManagerTest, SaveSnapshotAndUndo) {
    mgr.saveSnapshot(model);
    model.getObject("chip1")->setPosition({300, 400});
    EXPECT_EQ(chipPos().x, 300);

    EXPECT_TRUE(mgr.undo(model));
    EXPECT_EQ(chipPos().x, 100);
    EXPECT_EQ(chipPos().y, 100);
}

TEST_F(ActionManagerTest, UndoWhenEmpty) {
    EXPECT_FALSE(mgr.undo(model));
}

// ========== redo ==========

TEST_F(ActionManagerTest, RedoAfterUndo) {
    mgr.saveSnapshot(model);
    model.getObject("chip1")->setPosition({300, 400});

    mgr.undo(model);
    EXPECT_EQ(chipPos().x, 100);

    EXPECT_TRUE(mgr.redo(model));
    EXPECT_EQ(chipPos().x, 300);
    EXPECT_EQ(chipPos().y, 400);
}

TEST_F(ActionManagerTest, RedoWhenEmpty) {
    EXPECT_FALSE(mgr.redo(model));
}

TEST_F(ActionManagerTest, NewSnapshotClearsRedoStack) {
    mgr.saveSnapshot(model);
    model.getObject("chip1")->setPosition({300, 400});
    mgr.undo(model);
    EXPECT_TRUE(mgr.canRedo());

    // New action — redo future is gone
    mgr.saveSnapshot(model);
    model.getObject("chip1")->setPosition({500, 500});

    EXPECT_FALSE(mgr.canRedo());
    EXPECT_EQ(mgr.getRedoCount(), 0);
}

// ========== multiple undo/redo ==========

TEST_F(ActionManagerTest, MultipleUndos) {
    mgr.saveSnapshot(model);
    model.getObject("chip1")->setPosition({150, 150});

    mgr.saveSnapshot(model);
    model.getObject("chip1")->setPosition({200, 200});

    EXPECT_EQ(chipPos().x, 200);
    EXPECT_EQ(mgr.getUndoCount(), 2);

    mgr.undo(model);
    EXPECT_EQ(chipPos().x, 150);

    mgr.undo(model);
    EXPECT_EQ(chipPos().x, 100);

    EXPECT_FALSE(mgr.canUndo());
    EXPECT_EQ(mgr.getRedoCount(), 2);
}

TEST_F(ActionManagerTest, UndoThenRedoSequence) {
    mgr.saveSnapshot(model);
    model.getObject("chip1")->setPosition({150, 150});

    mgr.saveSnapshot(model);
    model.getObject("chip1")->setPosition({200, 200});

    mgr.undo(model);
    EXPECT_EQ(chipPos().x, 150);

    mgr.undo(model);
    EXPECT_EQ(chipPos().x, 100);

    mgr.redo(model);
    EXPECT_EQ(chipPos().x, 150);

    mgr.redo(model);
    EXPECT_EQ(chipPos().x, 200);

    EXPECT_FALSE(mgr.canRedo());
}

// ========== stacks state ==========

TEST_F(ActionManagerTest, CanUndoCanRedo) {
    EXPECT_FALSE(mgr.canUndo());
    EXPECT_FALSE(mgr.canRedo());

    mgr.saveSnapshot(model);
    EXPECT_TRUE(mgr.canUndo());
    EXPECT_FALSE(mgr.canRedo());

    mgr.undo(model);
    EXPECT_FALSE(mgr.canUndo());
    EXPECT_TRUE(mgr.canRedo());
}

TEST_F(ActionManagerTest, ClearHistory) {
    mgr.saveSnapshot(model);
    model.getObject("chip1")->setPosition({300, 400});
    mgr.saveSnapshot(model);

    mgr.undo(model);
    mgr.clear();

    EXPECT_FALSE(mgr.canUndo());
    EXPECT_FALSE(mgr.canRedo());
    EXPECT_EQ(mgr.getUndoCount(), 0);
    EXPECT_EQ(mgr.getRedoCount(), 0);
}

TEST_F(ActionManagerTest, ClearEmptyHistory) {
    mgr.clear(); // must not throw
    EXPECT_FALSE(mgr.canUndo());
    EXPECT_FALSE(mgr.canRedo());
}

// ========== history limit ==========

TEST_F(ActionManagerTest, HistoryLimitRespected) {
    mgr.setMaxHistorySize(3);

    for (int i = 1; i <= 5; ++i) {
        mgr.saveSnapshot(model);
        model.getObject("chip1")->setPosition({static_cast<float>(i * 10), 0});
    }

    EXPECT_EQ(mgr.getUndoCount(), 3);

    mgr.undo(model);
    mgr.undo(model);
    mgr.undo(model);
    EXPECT_FALSE(mgr.canUndo());
}

// ========== Lua side-effects covered by snapshots ==========

TEST_F(ActionManagerTest, UndoRestoresArbitraryModelChanges) {
    // Simulate: snapshot before a Lua event that flips a card
    mgr.saveSnapshot(model);
    std::dynamic_pointer_cast<Card>(model.getObject("card1"))->setFaceUp(true);
    EXPECT_TRUE(cardFaceUp());

    mgr.undo(model);
    EXPECT_FALSE(cardFaceUp()); // restored to false
}
