#include "components/Dice.hpp"
#include <gtest/gtest.h>

using dice::components::Dice;

TEST(DiceTest, ConstructorSetsDefaults) {
    const Dice d("dice_1", "My Dice");
    EXPECT_EQ(d.getId(), "dice_1");
    EXPECT_EQ(d.getName(), "My Dice");
    EXPECT_EQ(d.getType(), "Dice");
    EXPECT_EQ(d.getFaceCount(), 6);
    EXPECT_EQ(d.getValue(), 1);
    EXPECT_TRUE(d.getFaceTextures().empty());
}

TEST(DiceTest, SetAndGetFaceCount) {
    Dice d("dice_1", "Dice");
    d.setFaceCount(20);
    EXPECT_EQ(d.getFaceCount(), 20);
}

TEST(DiceTest, SetAndGetValue) {
    Dice d("dice_1", "Dice");
    d.setValue(4);
    EXPECT_EQ(d.getValue(), 4);
}

TEST(DiceTest, RollInRange) {
    Dice d("dice_1", "Dice");
    d.setFaceCount(6);
    for (int i = 0; i < 100; ++i) {
        const int result = d.roll();
        EXPECT_GE(result, 1);
        EXPECT_LE(result, 6);
        EXPECT_EQ(d.getValue(), result);
    }
}

TEST(DiceTest, RollUpdateValue) {
    Dice d("dice_1", "Dice");
    d.setFaceCount(4);
    const int result = d.roll();
    EXPECT_EQ(d.getValue(), result);
    EXPECT_GE(result, 1);
    EXPECT_LE(result, 4);
}

TEST(DiceTest, GetFaceTexturePathValidIndex) {
    Dice d("dice_1", "Dice");
    d.setFaceTextures(
        {"face1.png", "face2.png", "face3.png", "face4.png", "face5.png", "face6.png"});
    EXPECT_EQ(d.getFaceTexturePath(1), "face1.png");
    EXPECT_EQ(d.getFaceTexturePath(6), "face6.png");
}

TEST(DiceTest, GetFaceTexturePathInvalidIndexReturnsEmpty) {
    Dice d("dice_1", "Dice");
    d.setFaceTextures({"face1.png", "face2.png"});
    EXPECT_TRUE(d.getFaceTexturePath(0).empty());
    EXPECT_TRUE(d.getFaceTexturePath(3).empty());
    EXPECT_TRUE(d.getFaceTexturePath(-1).empty());
}

TEST(DiceTest, JsonRoundTrip) {
    Dice original("dice_1", "Red Dice");
    original.setFaceCount(8);
    original.setValue(5);
    original.setFaceTextures({"f1.png", "f2.png"});

    const nlohmann::json json = original.toJson();
    EXPECT_EQ(json["type"], "Dice");
    EXPECT_EQ(json["faceCount"], 8);
    EXPECT_EQ(json["value"], 5);
    ASSERT_EQ(json["faceTextures"].size(), 2U);
    EXPECT_EQ(json["faceTextures"][0], "f1.png");

    Dice loaded("", "");
    loaded.fromJson(json);
    EXPECT_EQ(loaded.getId(), "dice_1");
    EXPECT_EQ(loaded.getFaceCount(), 8);
    EXPECT_EQ(loaded.getValue(), 5);
    ASSERT_EQ(loaded.getFaceTextures().size(), 2U);
    EXPECT_EQ(loaded.getFaceTextures()[0], "f1.png");
}

TEST(DiceTest, JsonWithoutOptionalFieldsKeepsDefaults) {
    nlohmann::json json;
    json["id"] = "dice_1";
    json["name"] = "Dice";
    json["type"] = "Dice";

    Dice loaded("", "");
    loaded.fromJson(json);
    EXPECT_EQ(loaded.getFaceCount(), 6);
    EXPECT_EQ(loaded.getValue(), 1);
    EXPECT_TRUE(loaded.getFaceTextures().empty());
}
