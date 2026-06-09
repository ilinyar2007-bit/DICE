#include <filesystem>
#include <fstream>

#include "app/AppConfig.hpp"
#include "app/ConfigLoader.hpp"
#include <gtest/gtest.h>
#include <unistd.h>

class ConfigLoaderTest : public ::testing::Test {
    std::filesystem::path tmpPath_;

protected:
    void SetUp() override {
        tmpPath_ = std::filesystem::temp_directory_path() /
                   ("dice_test_" + std::to_string(::getpid()) + ".json");
    }
    void TearDown() override {
        std::filesystem::remove(tmpPath_);
    }
    void writeJson(const std::string& content) {
        std::ofstream f(tmpPath_);
        f << content;
    }
    dice::AppConfig loadTmpConfig() {
        return dice::loadConfig(tmpPath_);
    }
};

TEST_F(ConfigLoaderTest, PartialJsonKeepsProvidedFields) {
    writeJson(R"({"windowWidth": 1920, "windowHeight": 1080})");
    auto cfg = loadTmpConfig();
    EXPECT_EQ(cfg.windowWidth, 1920);
    EXPECT_EQ(cfg.windowHeight, 1080);
    EXPECT_EQ(cfg.framerateLimit, 60);
}

TEST_F(ConfigLoaderTest, MissingFieldFallsBackToDefault) {
    writeJson(R"({"windowWidth": 800})");
    auto cfg = loadTmpConfig();
    EXPECT_EQ(cfg.windowWidth, 800);
    EXPECT_EQ(cfg.framerateLimit, 60);
}

TEST_F(ConfigLoaderTest, MissingFileReturnsDefaults) {
    auto cfg = dice::loadConfig("/nonexistent/path/game.json");
    EXPECT_EQ(cfg.windowWidth, 1280);
}

TEST_F(ConfigLoaderTest, CorruptJsonReturnsDefaults) {
    writeJson("{ this is not json }");
    auto cfg = loadTmpConfig();
    EXPECT_EQ(cfg.windowWidth, 1280);
}

TEST_F(ConfigLoaderTest, GlobalScriptFieldIsLoaded) {
    writeJson(R"({"globalScript": "scripts/globals.lua"})");
    auto cfg = loadTmpConfig();
    EXPECT_EQ(cfg.globalScript, "scripts/globals.lua");
}

TEST_F(ConfigLoaderTest, NegativeLuaMemoryLimitClampsToDefault) {
    writeJson(R"({"luaMemoryLimitMb": -1})");
    auto cfg = loadTmpConfig();
    EXPECT_GT(cfg.luaMemoryLimitMb, 0);
}

TEST_F(ConfigLoaderTest, NegativeMaxSceneObjectsClampsToDefault) {
    writeJson(R"({"maxSceneObjects": -5})");
    auto cfg = loadTmpConfig();
    EXPECT_GT(cfg.maxSceneObjects, 0);
}

TEST_F(ConfigLoaderTest, ZeroMaxSceneObjectsClampsToDefault) {
    writeJson(R"({"maxSceneObjects": 0})");
    auto cfg = loadTmpConfig();
    EXPECT_GT(cfg.maxSceneObjects, 0);
}

TEST_F(ConfigLoaderTest, EmptyGlobalScriptIsIgnored) {
    writeJson(R"({})");
    auto cfg = loadTmpConfig();
    EXPECT_TRUE(cfg.globalScript.empty());
}
