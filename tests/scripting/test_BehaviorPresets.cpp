#include <nlohmann/json.hpp>

#include "core/GameObject.hpp"
#include "scripting/LuaScript.hpp"
#include "scripting/LuaScriptEngine.hpp"
#include <gtest/gtest.h>

using dice::core::GameObject;
using dice::scripting::kEventOnClick;
using dice::scripting::LuaScriptEngine;

namespace {
void applyPresets(
    dice::core::GameObject& obj,
    const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& catalog) {
    if (obj.getPresets().empty()) {
        return;
    }
    std::unordered_map<std::string, std::string> finalBindings;
    for (const auto& preset_name : obj.getPresets()) {
        auto it = catalog.find(preset_name);
        if (it == catalog.end()) {
            continue;
        }
        for (const auto& [event, ref] : it->second) {
            finalBindings[event] = ref;
        }
    }
    for (const auto& [event, ref] : obj.getTriggerBindings()) {
        finalBindings[event] = ref;
    }
    obj.clearTriggerBindings();
    for (const auto& [event, ref] : finalBindings) {
        obj.setTriggerBinding(event, ref);
    }
}
} // namespace

TEST(BehaviorPresets, LoadPresetsFromJsonPopulatesCatalog) {
    LuaScriptEngine engine;
    const nlohmann::json j = nlohmann::json::parse(R"({
        "presets": {
            "Rollable": { "on_click": "scripts/dice_preset.lua:on_roll" }
        }
    })");
    engine.loadPresetsFromJson(j);
    const auto& catalog = engine.getGlobalPresetCatalog();
    ASSERT_EQ(catalog.count("Rollable"), 1U);
    EXPECT_EQ(catalog.at("Rollable").at("on_click"), "scripts/dice_preset.lua:on_roll");
}

TEST(BehaviorPresets, LoadPresetsFromMissingFileDoesNotCrash) {
    LuaScriptEngine engine;
    engine.loadPresets("nonexistent_path/presets.json");
    EXPECT_TRUE(engine.getGlobalPresetCatalog().empty());
}

TEST(BehaviorPresets, ClearSceneStateDoesNotClearPresets) {
    LuaScriptEngine engine;
    const nlohmann::json j = nlohmann::json::parse(R"({"presets":{"P1":{"on_click":"f.lua:fn"}}})");
    engine.loadPresetsFromJson(j);
    engine.clearSceneState();
    EXPECT_EQ(engine.getGlobalPresetCatalog().count("P1"), 1U);
}

TEST(BehaviorPresets, FileColonFunctionDispatchCallsModule) {
    LuaScriptEngine engine;

    auto obj = std::make_shared<GameObject>("obj1", "original");
    obj->setTriggerBinding("on_click", "tests/fixtures/click_preset.lua:on_click");

    engine.fireEvent(kEventOnClick, obj.get());

    EXPECT_EQ(obj->getName(), "preset_fired");
}

TEST(BehaviorPresets, FileColonFunctionMissingFilDoesNotCrash) {
    LuaScriptEngine engine;
    auto obj = std::make_shared<GameObject>("obj1", "test");
    obj->setTriggerBinding("on_click", "nonexistent/file.lua:fn");

    const bool fired = engine.fireEvent(kEventOnClick, obj.get());
    EXPECT_FALSE(fired);
}

TEST(BehaviorPresets, FileColonFunctionMissingFuncDoesNotCrash) {
    LuaScriptEngine engine;
    auto obj = std::make_shared<GameObject>("obj1", "test");
    obj->setTriggerBinding("on_click", "tests/fixtures/click_preset.lua:no_such_func");

    const bool fired = engine.fireEvent(kEventOnClick, obj.get());
    EXPECT_FALSE(fired);
}

TEST(BehaviorPresets, GameObjectParsesPresetsFromJson) {
    GameObject obj;
    const nlohmann::json j = nlohmann::json::parse(R"({"presets":["A","B","C"]})");
    obj.fromJson(j);
    ASSERT_EQ(obj.getPresets().size(), 3U);
    EXPECT_EQ(obj.getPresets()[0], "A");
    EXPECT_EQ(obj.getPresets()[1], "B");
    EXPECT_EQ(obj.getPresets()[2], "C");
}

TEST(BehaviorPresets, GameObjectEmptyPresetsWhenFieldMissing) {
    GameObject obj;
    const nlohmann::json j = nlohmann::json::parse(R"({"id":"x"})");
    obj.fromJson(j);
    EXPECT_TRUE(obj.getPresets().empty());
}

TEST(BehaviorPresets, MergeTwoPresetsLatterWins) {
    LuaScriptEngine engine;
    const nlohmann::json j = nlohmann::json::parse(R"({
        "presets": {
            "P1": { "on_click": "f1.lua:fn" },
            "P2": { "on_click": "f2.lua:fn" }
        }
    })");
    engine.loadPresetsFromJson(j);

    auto obj = std::make_shared<GameObject>("obj1", "test");
    const nlohmann::json objJson = nlohmann::json::parse(R"({"presets":["P1","P2"]})");
    obj->fromJson(objJson);

    applyPresets(*obj, engine.getGlobalPresetCatalog());

    EXPECT_EQ(obj->getTriggerBindings().at("on_click"), "f2.lua:fn");
}

TEST(BehaviorPresets, IndividualTriggerOverridesPreset) {
    LuaScriptEngine engine;
    const nlohmann::json j =
        nlohmann::json::parse(R"({"presets":{"P1":{"on_click":"preset.lua:fn"}}})");
    engine.loadPresetsFromJson(j);

    auto obj = std::make_shared<GameObject>("obj1", "test");
    const nlohmann::json objJson = nlohmann::json::parse(R"({
        "presets": ["P1"],
        "triggers": {"on_click": "my_individual_trigger"}
    })");
    obj->fromJson(objJson);

    applyPresets(*obj, engine.getGlobalPresetCatalog());

    EXPECT_EQ(obj->getTriggerBindings().at("on_click"), "my_individual_trigger");
}

TEST(BehaviorPresets, UnknownPresetIsSkipped) {
    const LuaScriptEngine engine;

    auto obj = std::make_shared<GameObject>("obj1", "test");
    const nlohmann::json objJson = nlohmann::json::parse(R"({"presets":["NonExistent"]})");
    obj->fromJson(objJson);

    applyPresets(*obj, engine.getGlobalPresetCatalog());

    EXPECT_TRUE(obj->getTriggerBindings().empty());
}

TEST(BehaviorPresets, IntegrationPresetTriggerFiresViaModule) {
    LuaScriptEngine engine;

    // 1. Load preset catalog
    const nlohmann::json catalog = nlohmann::json::parse(R"({
        "presets": {
            "Clickable": {
                "on_click": "tests/fixtures/click_preset.lua:on_click"
            }
        }
    })");
    engine.loadPresetsFromJson(catalog);

    // 2. Create object as if loaded from scene JSON
    auto obj = std::make_shared<GameObject>("obj1", "original");
    const nlohmann::json objJson = nlohmann::json::parse(R"({
        "id": "obj1",
        "name": "original",
        "presets": ["Clickable"]
    })");
    obj->fromJson(objJson);

    // 3. Simulate Controller merge
    applyPresets(*obj, engine.getGlobalPresetCatalog());

    // 4. Fire event
    engine.fireEvent(kEventOnClick, obj.get());

    // 5. Verify preset function was called
    EXPECT_EQ(obj->getName(), "preset_fired");
}
