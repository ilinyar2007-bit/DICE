#include "components/Tile.hpp"
#include <gtest/gtest.h>

using dice::components::Tile;

TEST(TileTest, ConstructorSetsDefaults) {
    const Tile t("t1", "Cell A1");
    EXPECT_EQ(t.getId(), "t1");
    EXPECT_EQ(t.getName(), "Cell A1");
    EXPECT_EQ(t.getType(), "Tile");
    EXPECT_EQ(t.getCol(), 0);
    EXPECT_EQ(t.getRow(), 0);
    EXPECT_FALSE(t.isOccupied());
    EXPECT_TRUE(t.getOccupantId().empty());
    EXPECT_TRUE(t.getAcceptedTypes().empty());
}

TEST(TileTest, ColRowSetterGetter) {
    Tile t("t1", "Cell");
    t.setCol(3);
    t.setRow(7);
    EXPECT_EQ(t.getCol(), 3);
    EXPECT_EQ(t.getRow(), 7);
}

TEST(TileTest, OccupantSetClearAndIsOccupied) {
    Tile t("t1", "Cell");
    EXPECT_FALSE(t.isOccupied());
    t.setOccupant("chip_5");
    EXPECT_TRUE(t.isOccupied());
    EXPECT_EQ(t.getOccupantId(), "chip_5");
    t.clearOccupant();
    EXPECT_FALSE(t.isOccupied());
    EXPECT_TRUE(t.getOccupantId().empty());
}

TEST(TileTest, AcceptsAllWhenListEmpty) {
    const Tile t("t1", "Cell");
    EXPECT_TRUE(t.accepts("Chip"));
    EXPECT_TRUE(t.accepts("Card"));
    EXPECT_TRUE(t.accepts("Dice"));
    EXPECT_TRUE(t.accepts("GameObject"));
}

TEST(TileTest, AcceptsOnlyListedTypes) {
    Tile t("t1", "Cell");
    t.setAcceptedTypes({"Chip", "Card"});
    EXPECT_TRUE(t.accepts("Chip"));
    EXPECT_TRUE(t.accepts("Card"));
    EXPECT_FALSE(t.accepts("Dice"));
    EXPECT_FALSE(t.accepts("GameObject"));
}

TEST(TileTest, JsonRoundTrip) {
    Tile original("t1", "Cell A1");
    original.setCol(3);
    original.setRow(5);
    original.setOccupant("chip_7");
    original.setAcceptedTypes({"Chip", "Pawn"});
    original.setPosition(320.F, 240.F);
    original.setZOrder(2);
    original.addTag("grid");

    const nlohmann::json json = original.toJson();
    EXPECT_EQ(json["type"], "Tile");
    EXPECT_EQ(json["col"], 3);
    EXPECT_EQ(json["row"], 5);
    EXPECT_EQ(json["occupantId"], "chip_7");
    ASSERT_EQ(json["acceptedTypes"].size(), 2U);
    EXPECT_EQ(json["acceptedTypes"][0], "Chip");

    Tile loaded("", "");
    loaded.fromJson(json);
    EXPECT_EQ(loaded.getId(), "t1");
    EXPECT_EQ(loaded.getCol(), 3);
    EXPECT_EQ(loaded.getRow(), 5);
    EXPECT_EQ(loaded.getOccupantId(), "chip_7");
    EXPECT_TRUE(loaded.isOccupied());
    ASSERT_EQ(loaded.getAcceptedTypes().size(), 2U);
    EXPECT_EQ(loaded.getAcceptedTypes()[0], "Chip");
    EXPECT_FLOAT_EQ(loaded.getPosition().x, 320.F);
    EXPECT_EQ(loaded.getZOrder(), 2);
    EXPECT_TRUE(loaded.hasTag("grid"));
}

TEST(TileTest, JsonWithoutOptionalFieldsKeepsDefaults) {
    nlohmann::json json;
    json["id"] = "t1";
    json["name"] = "Cell";
    json["type"] = "Tile";
    json["col"] = 2;
    json["row"] = 4;

    Tile loaded("", "");
    loaded.fromJson(json);
    EXPECT_EQ(loaded.getCol(), 2);
    EXPECT_EQ(loaded.getRow(), 4);
    EXPECT_FALSE(loaded.isOccupied());
    EXPECT_TRUE(loaded.getAcceptedTypes().empty());
}
