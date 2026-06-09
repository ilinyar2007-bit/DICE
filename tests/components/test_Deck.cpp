#include "components/Card.hpp"
#include "components/Deck.hpp"
#include "core/GameObject.hpp"
#include <gtest/gtest.h>

using dice::components::Card;
using dice::components::Deck;

TEST(DeckTest, ConstructorSetsDefaults) {
    const Deck d("deck_1", "Main Deck");
    EXPECT_EQ(d.getId(), "deck_1");
    EXPECT_EQ(d.getName(), "Main Deck");
    EXPECT_EQ(d.getType(), "Deck");
    EXPECT_EQ(d.count(), 0);
    EXPECT_TRUE(d.isEmpty());
    EXPECT_FALSE(d.isFaceDown());
}

TEST(DeckTest, CountAndIsEmptyReflectChildren) {
    Deck d("deck_1", "Deck");
    EXPECT_TRUE(d.isEmpty());
    d.addChild(std::make_shared<Card>("c1", "Ace"));
    EXPECT_EQ(d.count(), 1);
    EXPECT_FALSE(d.isEmpty());
    d.addChild(std::make_shared<Card>("c2", "King"));
    EXPECT_EQ(d.count(), 2);
}

TEST(DeckTest, SetFaceDownTrueFlipsAllCardsFaceDown) {
    Deck d("deck_1", "Deck");
    auto c1 = std::make_shared<Card>("c1", "Card 1");
    auto c2 = std::make_shared<Card>("c2", "Card 2");
    c1->setFaceUp(true);
    c2->setFaceUp(true);
    d.addChild(c1);
    d.addChild(c2);

    d.setFaceDown(true);

    EXPECT_TRUE(d.isFaceDown());
    EXPECT_FALSE(c1->isFaceUp());
    EXPECT_FALSE(c2->isFaceUp());
}

TEST(DeckTest, SetFaceDownFalseFlipsAllCardsFaceUp) {
    Deck d("deck_1", "Deck");
    auto c1 = std::make_shared<Card>("c1", "Card 1");
    d.addChild(c1);
    d.setFaceDown(true);
    d.setFaceDown(false);
    EXPECT_FALSE(d.isFaceDown());
    EXPECT_TRUE(c1->isFaceUp());
}

TEST(DeckTest, SetFaceDownIgnoresNonCardChildren) {
    Deck d("deck_1", "Deck");
    auto obj = std::make_shared<dice::core::GameObject>("token_1", "Token");
    d.addChild(obj);
    EXPECT_NO_THROW(d.setFaceDown(true));
    EXPECT_TRUE(d.isFaceDown());
}

TEST(DeckTest, JsonRoundTrip) {
    Deck original("deck_1", "Main Deck");
    original.setFaceDown(true);
    original.setPosition(640.F, 360.F);
    original.setZOrder(3);
    original.addTag("draw_pile");

    const nlohmann::json json = original.toJson();
    EXPECT_EQ(json["type"], "Deck");
    EXPECT_EQ(json["faceDown"], true);

    Deck loaded("", "");
    loaded.fromJson(json);
    EXPECT_EQ(loaded.getId(), "deck_1");
    EXPECT_TRUE(loaded.isFaceDown());
    EXPECT_FLOAT_EQ(loaded.getPosition().x, 640.F);
    EXPECT_EQ(loaded.getZOrder(), 3);
    EXPECT_TRUE(loaded.hasTag("draw_pile"));
}

TEST(DeckTest, JsonWithoutFaceDownKeepsDefault) {
    nlohmann::json json;
    json["id"] = "deck_1";
    json["name"] = "Deck";
    json["type"] = "Deck";

    Deck loaded("", "");
    loaded.fromJson(json);
    EXPECT_FALSE(loaded.isFaceDown());
}
