#include "app/AppConfig.hpp"
#include "app/ConfigLoader.hpp"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <unistd.h>

class ConfigLoaderTest : public ::testing::Test {
protected:
    std::filesystem::path tmpPath_;
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
};

TEST_F(ConfigLoaderTest, PartialJsonKeepsProvidedFields) {
    // File without luaMemoryLimitMb — windowWidth should be applied
    writeJson(R"({"windowWidth": 1920, "windowHeight": 1080})");
    auto cfg = dice::loadConfig(tmpPath_);
    EXPECT_EQ(cfg.windowWidth, 1920);
    EXPECT_EQ(cfg.windowHeight, 1080);
    EXPECT_EQ(cfg.framerateLimit, 60); // unspecified → default preserved
}

TEST_F(ConfigLoaderTest, MissingFieldFallsBackToDefault) {
    writeJson(R"({"windowWidth": 800})");
    auto cfg = dice::loadConfig(tmpPath_);
    EXPECT_EQ(cfg.windowWidth, 800);
    EXPECT_EQ(cfg.framerateLimit, 60); // default preserved
}

TEST_F(ConfigLoaderTest, MissingFileReturnsDefaults) {
    auto cfg = dice::loadConfig("/nonexistent/path/game.json");
    EXPECT_EQ(cfg.windowWidth, 1280);
}

TEST_F(ConfigLoaderTest, CorruptJsonReturnsDefaults) {
    writeJson("{ this is not json }");
    auto cfg = dice::loadConfig(tmpPath_);
    EXPECT_EQ(cfg.windowWidth, 1280);
}

TEST_F(ConfigLoaderTest, GlobalScriptFieldIsLoaded) {
    writeJson(R"({"globalScript": "scripts/globals.lua"})");
    auto cfg = dice::loadConfig(tmpPath_);
    EXPECT_EQ(cfg.globalScript, "scripts/globals.lua");
}
