#include <algorithm>
#include <memory>
#include <vector>

#include <nlohmann/json.hpp>

#include "components/Card.hpp"
#include "components/Chip.hpp"
#include "core/GameObject.hpp"
#include "core/Model.hpp"
#include "scene/DefaultFactory.hpp"
#include <gtest/gtest.h>

using dice::core::GameObject;
using dice::core::Model;

TEST(ModelCleanTest, AddRootRegistersObjectAndChildrenInIndex) {
    Model model;

    auto root = std::make_shared<GameObject>();
    root->setId("root");

    auto child = std::make_shared<GameObject>();
    child->setId("child");

    root->addChild(child);
    model.addRootObject(root);

    ASSERT_NE(model.getObject("root"), nullptr);
    ASSERT_NE(model.getObject("child"), nullptr);
    EXPECT_EQ(model.getObject("child")->getParent(), root.get());
}

TEST(ModelCleanTest, AttachToSuccessAndFailure) {
    Model model;

    auto parent = std::make_shared<GameObject>();
    parent->setId("parent");
    model.addRootObject(parent);

    auto child = std::make_shared<GameObject>();
    child->setId("child");

    EXPECT_FALSE(model.attachTo("missing_parent", child));

    EXPECT_TRUE(model.attachTo("parent", child));
    ASSERT_NE(model.getObject("child"), nullptr);
    EXPECT_EQ(model.getObject("child")->getParent(), parent.get());
}

TEST(ModelCleanTest, RemoveRootObjectRemovesFromRootsAndIndex) {
    Model model;

    auto root = std::make_shared<GameObject>();
    root->setId("root");

    auto child = std::make_shared<GameObject>();
    child->setId("child");
    root->addChild(child);

    model.addRootObject(root);

    EXPECT_TRUE(model.removeObject("root"));

    EXPECT_EQ(model.getObject("root"), nullptr);
    EXPECT_EQ(model.getObject("child"), nullptr);
    EXPECT_TRUE(model.roots().empty());
}

TEST(ModelCleanTest, RemoveChildDetachesFromParentAndIndex) {
    Model model;

    auto parent = std::make_shared<GameObject>();
    parent->setId("parent");

    auto child = std::make_shared<GameObject>();
    child->setId("child");

    parent->addChild(child);
    model.addRootObject(parent);

    EXPECT_TRUE(model.removeObject("child"));

    EXPECT_EQ(model.getObject("child"), nullptr);
    EXPECT_EQ(child->getParent(), nullptr);

    ASSERT_NE(model.getObject("parent"), nullptr);

    bool stillInChildren = false;
    for (const auto& ch : parent->getChildren()) {
        if (ch && ch->getId() == "child") {
            stillInChildren = true;
        }
    }
    EXPECT_FALSE(stillInChildren);
}

TEST(ModelCleanTest, ForEachDepthFirstVisitsAllObjects) {
    Model model;

    auto r1 = std::make_shared<GameObject>();
    r1->setId("r1");
    auto r2 = std::make_shared<GameObject>();
    r2->setId("r2");

    auto a = std::make_shared<GameObject>();
    a->setId("a");
    auto b = std::make_shared<GameObject>();
    b->setId("b");
    auto c = std::make_shared<GameObject>();
    c->setId("c");

    r1->addChild(a);
    a->addChild(b);
    r2->addChild(c);

    model.addRootObject(r1);
    model.addRootObject(r2);

    std::vector<std::string> visited;
    model.forEachDepthFirst(
        [&](const std::shared_ptr<GameObject>& obj) { visited.push_back(obj->getId()); });

    auto has = [&](const std::string& id) {
        return std::find(visited.begin(), visited.end(), id) != visited.end();
    };

    EXPECT_TRUE(has("r1"));
    EXPECT_TRUE(has("a"));
    EXPECT_TRUE(has("b"));
    EXPECT_TRUE(has("r2"));
    EXPECT_TRUE(has("c"));
}

TEST(ModelJsonFactoryTest, CreatesCorrectDerivedTypesFromJson) {
    Model model(dice::scene::makeDefaultFactory());

    const nlohmann::json j = {
        {"objects",
         nlohmann::json::array(
             {{{"type", "Chip"}, {"id", "chip_test"}, {"name", "TestChip"}},
              {{"type", "Card"}, {"id", "card_test"}, {"name", "TestCard"}},
              {{"type", "GameObject"}, {"id", "obj_test"}, {"name", "TestObj"}}})}};

    model.fromJson(j);

    auto chip = model.getObject("chip_test");
    auto card = model.getObject("card_test");
    auto obj = model.getObject("obj_test");

    ASSERT_NE(chip, nullptr);
    ASSERT_NE(card, nullptr);
    ASSERT_NE(obj, nullptr);

    EXPECT_NE(dynamic_cast<dice::components::Chip*>(chip.get()), nullptr);
    EXPECT_NE(dynamic_cast<dice::components::Card*>(card.get()), nullptr);
    EXPECT_EQ(dynamic_cast<dice::components::Chip*>(obj.get()), nullptr);
    EXPECT_EQ(dynamic_cast<dice::components::Card*>(obj.get()), nullptr);
}

namespace {
nlohmann::json makeChildrenTestJson() {
    const nlohmann::json chipJson = {{"type", "Chip"}, {"id", "chip_1"}, {"name", "RedChip"}};
    const nlohmann::json cardJson = {{"type", "Card"}, {"id", "card_1"}, {"name", "AceOfSpades"}};
    const nlohmann::json tableJson = {{"type", "GameObject"},
                                      {"id", "table"},
                                      {"name", "MainTable"},
                                      {"children", nlohmann::json::array({chipJson, cardJson})}};
    const nlohmann::json chipRootJson = {
        {"type", "Chip"}, {"id", "chip_root"}, {"name", "BlueChip"}};
    return {{"objects", nlohmann::json::array({tableJson, chipRootJson})}};
}

TEST(ModelJsonFactoryTest, LoadsChildrenRecursively_ObjectsRegistered) {
    Model model(dice::scene::makeDefaultFactory());
    const nlohmann::json j = makeChildrenTestJson();

    model.fromJson(j);

    ASSERT_NE(model.getObject("table"), nullptr);
    ASSERT_NE(model.getObject("chip_1"), nullptr);
    ASSERT_NE(model.getObject("card_1"), nullptr);
    ASSERT_NE(model.getObject("chip_root"), nullptr);

    EXPECT_NE(dynamic_cast<dice::components::Chip*>(model.getObject("chip_1").get()), nullptr);
    EXPECT_NE(dynamic_cast<dice::components::Card*>(model.getObject("card_1").get()), nullptr);
}

void checkChildrenHierarchy(const std::shared_ptr<GameObject>& table) {
    bool hasChip1 = false;
    bool hasCard1 = false;
    for (const auto& ch : table->getChildren()) {
        if (!ch) {
            continue;
        }
        hasChip1 |= (ch->getId() == "chip_1");
        hasCard1 |= (ch->getId() == "card_1");
        EXPECT_EQ(ch->getParent(), table.get());
    }
    EXPECT_TRUE(hasChip1);
    EXPECT_TRUE(hasCard1);
}
} // namespace

TEST(ModelJsonFactoryTest, LoadsChildrenRecursively_HierarchyCorrect) {
    Model model(dice::scene::makeDefaultFactory());
    const nlohmann::json j = makeChildrenTestJson();

    model.fromJson(j);

    ASSERT_NE(model.getObject("table"), nullptr);
    checkChildrenHierarchy(model.getObject("table"));
}

TEST(ModelCleanTest, ToJsonContainsObjectsArray) {
    Model model;

    auto root = std::make_shared<GameObject>();
    root->setId("root");
    model.addRootObject(root);

    auto j = model.toJson();
    ASSERT_TRUE(j.contains("objects"));
    ASSERT_TRUE(j["objects"].is_array());
    ASSERT_EQ(j["objects"].size(), 1U);
}

TEST(ModelCleanTest, ClearEmptiesRootsAndIndex) {
    Model model;

    auto root = std::make_shared<GameObject>();
    root->setId("root");

    auto child = std::make_shared<GameObject>();
    child->setId("child");
    root->addChild(child);

    model.addRootObject(root);

    model.clear();

    EXPECT_TRUE(model.roots().empty());
    EXPECT_EQ(model.getObject("root"), nullptr);
    EXPECT_EQ(model.getObject("child"), nullptr);
}

TEST(ModelCleanTest, UnregisterCallbackCalledOnRemove) {
    Model model;
    auto root = std::make_shared<GameObject>();
    root->setId("root");
    auto child = std::make_shared<GameObject>();
    child->setId("child");
    root->addChild(child);
    model.addRootObject(root);

    std::vector<std::string> removedIds;
    model.setUnregisterCallback([&](const std::string& id) { removedIds.push_back(id); });

    model.removeObject("root");

    ASSERT_EQ(removedIds.size(), 2U);
    EXPECT_EQ(removedIds[0], "root");
    EXPECT_EQ(removedIds[1], "child");
}
