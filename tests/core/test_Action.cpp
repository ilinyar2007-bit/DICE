#include <memory>

#include <SFML/System/Vector2.hpp>

#include "components/Chip.hpp"
#include "core/Action.hpp"
#include "core/Model.hpp"
#include "scene/DefaultFactory.hpp"
#include <gtest/gtest.h>

using dice::components::Chip;
using dice::core::Model;
using dice::core::MoveObjectAction;
using dice::scene::makeDefaultFactory;

class ActionTest : public ::testing::Test {
protected:
    void SetUp() override {
        model.setFactory(makeDefaultFactory());

        chip = std::make_shared<Chip>("chip1", "Test Chip");
        chip->setPosition(100, 100);
        chip->setDraggable(true);
        model.addRootObject(chip);
    }

    Model model;                // NOLINT
    std::shared_ptr<Chip> chip; // NOLINT
};

// ========== execute ==========

TEST_F(ActionTest, MoveObjectActionExecute) {
    MoveObjectAction action("chip1", {300, 400});

    EXPECT_TRUE(action.execute(model));
    EXPECT_EQ(chip->getPosition().x, 300);
    EXPECT_EQ(chip->getPosition().y, 400);
}

TEST_F(ActionTest, MoveObjectActionExecuteFailsIfNotFound) {
    MoveObjectAction action("nonexistent", {300, 400});

    EXPECT_FALSE(action.execute(model));
    EXPECT_EQ(chip->getPosition().x, 100); // unchanged
}

TEST_F(ActionTest, MoveObjectActionExecuteIgnoresDraggableFlag) {
    chip->setDraggable(false);
    MoveObjectAction action("chip1", {300, 400});

    // execute() does not check isDraggable — only canExecute() does
    EXPECT_TRUE(action.execute(model));
    EXPECT_EQ(chip->getPosition().x, 300);
}

// ========== canExecute ==========

TEST_F(ActionTest, MoveObjectActionCanExecute) {
    const MoveObjectAction action("chip1", {300, 400});
    EXPECT_TRUE(action.canExecute(model));
}

TEST_F(ActionTest, MoveObjectActionCannotExecuteIfNotFound) {
    const MoveObjectAction action("nonexistent", {300, 400});
    EXPECT_FALSE(action.canExecute(model));
}

TEST_F(ActionTest, MoveObjectActionCannotExecuteIfNotDraggable) {
    chip->setDraggable(false);
    const MoveObjectAction action("chip1", {300, 400});
    EXPECT_FALSE(action.canExecute(model));
}
