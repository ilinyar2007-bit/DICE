#include "core/GameObject.hpp"
#include "scripting/LuaScript.hpp"
#include "scripting/LuaScriptEngine.hpp"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using dice::core::GameObject;
using dice::scripting::LuaScriptEngine;
using dice::scripting::kEventOnClick;

TEST(BehaviorPresets, LoadPresetsFromJsonPopulatesCatalog) {
    LuaScriptEngine engine;
    nlohmann::json j = nlohmann::json::parse(R"({
        "presets": {
            "Rollable": { "on_click": "scripts/dice_preset.lua:on_roll" }
        }
    })");
    engine.loadPresetsFromJson(j);
    const auto& catalog = engine.getGlobalPresetCatalog();
    ASSERT_EQ(catalog.count("Rollable"), 1u);
    EXPECT_EQ(catalog.at("Rollable").at("on_click"), "scripts/dice_preset.lua:on_roll");
}

TEST(BehaviorPresets, LoadPresetsFromMissingFileDoesNotCrash) {
    LuaScriptEngine engine;
    engine.loadPresets("nonexistent_path/presets.json");
    EXPECT_TRUE(engine.getGlobalPresetCatalog().empty());
}

TEST(BehaviorPresets, ClearSceneStateDoesNotClearPresets) {
    LuaScriptEngine engine;
    nlohmann::json j = nlohmann::json::parse(R"({"presets":{"P1":{"on_click":"f.lua:fn"}}})");
    engine.loadPresetsFromJson(j);
    engine.clearSceneState();
    EXPECT_EQ(engine.getGlobalPresetCatalog().count("P1"), 1u);
}
