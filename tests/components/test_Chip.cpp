#include "components/Chip.hpp"
#include <gtest/gtest.h>

using dice::components::Chip;

TEST(ChipTest, ConstructorSetsFieldsAndType) {
    const Chip chip("chip_1", "Red Chip");

    EXPECT_EQ(chip.getId(), "chip_1");
    EXPECT_EQ(chip.getName(), "Red Chip");
    EXPECT_EQ(chip.getType(), "Chip");
    EXPECT_EQ(chip.getPlayer(), 0);
    EXPECT_TRUE(chip.getAssetId().empty());
    EXPECT_FLOAT_EQ(chip.getRadius(), 32.F);
}

TEST(ChipTest, PlayerSetterGetter) {
    Chip chip("chip_1", "Red Chip");
    chip.setPlayer(2);
    EXPECT_EQ(chip.getPlayer(), 2);
}

TEST(ChipTest, JsonSerializationRoundTrip) {
    Chip original("chip_1", "Red Chip");
    original.setPlayer(1);
    original.setAssetId("assets/chips/red.png");
    original.setRadius(24.F);
    original.setPosition(10.F, 20.F);
    original.setRotation(30.F);
    original.setZOrder(7);
    original.addTag("movable");

    nlohmann::json json = original.toJson();

    EXPECT_EQ(json["id"], "chip_1");
    EXPECT_EQ(json["name"], "Red Chip");
    EXPECT_EQ(json["type"], "Chip");
    EXPECT_EQ(json["player"], 1);
    EXPECT_EQ(json["assetId"], "assets/chips/red.png");
    EXPECT_FLOAT_EQ(json["radius"].get<float>(), 24.F);
    EXPECT_EQ(json["zOrder"], 7);

    Chip loaded("", "");
    loaded.fromJson(json);

    EXPECT_EQ(loaded.getId(), "chip_1");
    EXPECT_EQ(loaded.getName(), "Red Chip");
    EXPECT_EQ(loaded.getType(), "Chip");
    EXPECT_EQ(loaded.getPlayer(), 1);
    EXPECT_EQ(loaded.getAssetId(), "assets/chips/red.png");
    EXPECT_FLOAT_EQ(loaded.getRadius(), 24.F);
    EXPECT_FLOAT_EQ(loaded.getPosition().x, 10.F);
    EXPECT_FLOAT_EQ(loaded.getPosition().y, 20.F);
    EXPECT_FLOAT_EQ(loaded.getRotation(), 30.F);
    EXPECT_EQ(loaded.getZOrder(), 7);
    EXPECT_TRUE(loaded.hasTag("movable"));
}

TEST(ChipTest, JsonDeserializationWithoutPlayerKeepsDefault) {
    nlohmann::json json;
    json["id"] = "chip_1";
    json["name"] = "Red Chip";
    json["type"] = "Chip";

    Chip loaded("", "");
    loaded.fromJson(json);

    EXPECT_EQ(loaded.getPlayer(), 0);
    EXPECT_TRUE(loaded.getAssetId().empty());
    EXPECT_FLOAT_EQ(loaded.getRadius(), 32.F);
}

TEST(ChipTest, HitTestUsesRadiusWhenSet) {
    Chip chip("chip_1", "Red Chip");
    chip.setPosition(100.F, 100.F);
    chip.setRadius(10.F);

    EXPECT_TRUE(chip.hitTest({100.F, 100.F}));  // center
    EXPECT_TRUE(chip.hitTest({109.9F, 100.F})); // inside
    EXPECT_FALSE(chip.hitTest({111.F, 100.F})); // outside
}

TEST(ChipTest, HitTestRespectsScale) {
    Chip chip("chip_1", "Red Chip");
    chip.setPosition(0.F, 0.F);
    chip.setRadius(10.F);
    chip.setScale(2.F, 2.F);

    EXPECT_TRUE(chip.hitTest({19.9F, 0.F}));
    EXPECT_FALSE(chip.hitTest({21.F, 0.F}));
}
