#include "core/GameObject.hpp"
#include "scripting/LuaScript.hpp"
#include "scripting/LuaScriptEngine.hpp"
#include <gtest/gtest.h>

using dice::core::GameObject;
using dice::scripting::LuaScriptEngine;

class LuaScriptEngineTest : public ::testing::Test {
protected:
    LuaScriptEngine engine_;
};

// ========== Создание и загрузка ==========

TEST_F(LuaScriptEngineTest, CreateFromSourceReturnsNonNull) {
    EXPECT_NE(engine_.createFromSource("function on_click(self) end"), nullptr);
}

TEST_F(LuaScriptEngineTest, LoadValidSourceReturnsTrue) {
    auto s = engine_.createFromSource("function on_click(self) end");
    EXPECT_TRUE(s->load());
}

TEST_F(LuaScriptEngineTest, LoadSyntaxErrorReturnsFalse) {
    auto s = engine_.createFromSource("@@@ not lua");
    EXPECT_FALSE(s->load());
}

TEST_F(LuaScriptEngineTest, LoadEmptyScriptSucceeds) {
    auto s = engine_.createFromSource("-- comment only");
    EXPECT_TRUE(s->load());
}

TEST_F(LuaScriptEngineTest, CreateFromMissingFileReturnsNull) {
    EXPECT_EQ(engine_.createFromFile("scripts/nonexistent_xyz.lua"), nullptr);
}

// ========== hasHandler ==========

TEST_F(LuaScriptEngineTest, HasHandlerTrueForDefinedFunction) {
    auto s = engine_.createFromSource("function on_click(self) end");
    s->load();
    EXPECT_TRUE(s->hasHandler("on_click"));
}

TEST_F(LuaScriptEngineTest, HasHandlerFalseForUndefinedFunction) {
    auto s = engine_.createFromSource("function on_click(self) end");
    s->load();
    EXPECT_FALSE(s->hasHandler("on_move"));
}

TEST_F(LuaScriptEngineTest, HasHandlerFalseBeforeLoad) {
    auto s = engine_.createFromSource("function on_click(self) end");
    EXPECT_FALSE(s->hasHandler("on_click"));
}

// ========== trigger ==========

TEST_F(LuaScriptEngineTest, TriggerModifiesGameObject) {
    auto s = engine_.createFromSource(R"(
        function on_click(self)
            self:setName("triggered")
        end
    )");
    s->load();
    GameObject obj("id1", "original");
    EXPECT_TRUE(s->trigger("on_click", &obj));
    EXPECT_EQ(obj.getName(), "triggered");
}

TEST_F(LuaScriptEngineTest, TriggerOnMoveMovesObject) {
    auto s = engine_.createFromSource(R"(
        function on_move(self)
            self:setPosition(100, 200)
        end
    )");
    s->load();
    GameObject obj("id2", "chip");
    s->trigger("on_move", &obj);
    EXPECT_FLOAT_EQ(obj.getPosition().x, 100.F);
    EXPECT_FLOAT_EQ(obj.getPosition().y, 200.F);
}

TEST_F(LuaScriptEngineTest, TriggerWithNoHandlerReturnsFalse) {
    auto s = engine_.createFromSource("-- empty");
    s->load();
    GameObject obj("id3", "test");
    EXPECT_FALSE(s->trigger("on_click", &obj));
}

TEST_F(LuaScriptEngineTest, TriggerBeforeLoadReturnsFalse) {
    auto s = engine_.createFromSource("function on_click(self) end");
    GameObject obj("id4", "test");
    EXPECT_FALSE(s->trigger("on_click", &obj));
}

TEST_F(LuaScriptEngineTest, TriggerRuntimeErrorReturnsFalse) {
    auto s = engine_.createFromSource(R"(
        function on_click(self)
            error("boom")
        end
    )");
    s->load();
    GameObject obj("id5", "test");
    EXPECT_FALSE(s->trigger("on_click", &obj));
}

TEST_F(LuaScriptEngineTest, TriggerSetActiveFromLua) {
    auto s = engine_.createFromSource(R"(
        function on_click(self)
            self:setActive(false)
        end
    )");
    s->load();
    GameObject obj("id_active", "test");
    EXPECT_TRUE(obj.isActive());
    s->trigger("on_click", &obj);
    EXPECT_FALSE(obj.isActive());
}

TEST_F(LuaScriptEngineTest, TriggerSetVisibleFromLua) {
    auto s = engine_.createFromSource(R"(
        function on_click(self)
            self:setVisible(false)
        end
    )");
    s->load();
    GameObject obj("id_vis", "test");
    EXPECT_TRUE(obj.isVisible());
    s->trigger("on_click", &obj);
    EXPECT_FALSE(obj.isVisible());
}

TEST_F(LuaScriptEngineTest, TriggerReadsPositionFromLua) {
    auto s = engine_.createFromSource(R"(
        function on_click(self)
            local x = self:getX()
            local y = self:getY()
            self:setPosition(x + 5, y + 10)
        end
    )");
    s->load();
    GameObject obj("id_pos", "test");
    obj.setPosition(10.F, 20.F);
    s->trigger("on_click", &obj);
    EXPECT_FLOAT_EQ(obj.getPosition().x, 15.F);
    EXPECT_FLOAT_EQ(obj.getPosition().y, 30.F);
}

// ========== Sandbox ==========

TEST_F(LuaScriptEngineTest, ScriptsDoNotShareGlobals) {
    auto s1 = engine_.createFromSource("x = 42");
    auto s2 = engine_.createFromSource(R"(
        function on_click(self)
            if x ~= nil then self:setName("leaked") end
        end
    )");
    s1->load();
    s2->load();

    GameObject obj("id6", "original");
    s2->trigger("on_click", &obj);
    EXPECT_EQ(obj.getName(), "original");
}

TEST_F(LuaScriptEngineTest, TwoScriptsCanDefineHandlerWithSameName) {
    auto s1 = engine_.createFromSource(R"(
        function on_click(self) self:setName("from_s1") end
    )");
    auto s2 = engine_.createFromSource(R"(
        function on_click(self) self:setName("from_s2") end
    )");
    s1->load();
    s2->load();

    GameObject obj1("obj_s1", "test");
    GameObject obj2("obj_s2", "test");
    s1->trigger("on_click", &obj1);
    s2->trigger("on_click", &obj2);

    EXPECT_EQ(obj1.getName(), "from_s1");
    EXPECT_EQ(obj2.getName(), "from_s2");
}

// ========== cpp_log callback ==========

TEST_F(LuaScriptEngineTest, CppLogCallbackFires) {
    bool fired = false;
    engine_.registerCallback("cpp_log", [&fired](const std::string&) { fired = true; });

    auto s = engine_.createFromSource(R"(
        function on_click(self) cpp_log("hello") end
    )");
    s->load();
    GameObject obj("id7", "test");
    s->trigger("on_click", &obj);
    EXPECT_TRUE(fired);
}

TEST_F(LuaScriptEngineTest, CppLogReceivesCorrectMessage) {
    std::string received;
    engine_.registerCallback("cpp_log", [&received](const std::string& m) { received = m; });

    auto s = engine_.createFromSource(R"(
        function on_click(self) cpp_log("hello dice") end
    )");
    s->load();
    GameObject obj("id8", "test");
    s->trigger("on_click", &obj);
    EXPECT_EQ(received, "hello dice");
}

// ========== attachScript / fireEvent ==========

TEST_F(LuaScriptEngineTest, AttachScriptMissingFileReturnsFalse) {
    GameObject obj("c1", "Chip");
    obj.setLuaScript("scripts/nonexistent_xyz.lua");
    EXPECT_FALSE(engine_.attachScript(obj));
}

TEST_F(LuaScriptEngineTest, AttachScriptEmptyPathReturnsFalse) {
    GameObject obj("c2", "Chip");
    EXPECT_FALSE(engine_.attachScript(obj));
}

TEST_F(LuaScriptEngineTest, FireEventNoScriptAttachedReturnsFalse) {
    GameObject obj("c3", "Chip");
    EXPECT_FALSE(engine_.fireEvent("on_click", &obj));
}

TEST_F(LuaScriptEngineTest, FireEventNullObjectReturnsFalse) {
    EXPECT_FALSE(engine_.fireEvent("on_click", nullptr));
}

// ========== Trigger Catalog ==========

TEST(LuaScriptEngineTriggerCatalog, TriggerFiredByBinding) {
    dice::scripting::LuaScriptEngine engine;

    // Register a trigger named "roll_dice" that sets a global flag
    engine.executeGlobalScriptFromSource(R"(
        called = false
        engine.trigger("roll_dice", function(obj)
            called = true
        end)
    )");

    // Create a GameObject with a trigger binding
    auto obj = std::make_shared<dice::core::GameObject>("die1", "Die");
    obj->setTriggerBinding("on_click", "roll_dice");

    engine.fireEvent("on_click", obj.get());

    // Verify trigger was called
    const bool called = engine.getGlobalVariable<bool>("called");
    EXPECT_TRUE(called);
}

// ========== Usertype bindings: setColor / hasTag / getTags ==========

TEST(LuaScriptEngineBindings, SetColorDoesNotCrash) {
    dice::scripting::LuaScriptEngine engine;
    auto obj = std::make_shared<dice::core::GameObject>("obj1", "Object");

    engine.executeGlobalScriptFromSource(R"(
        engine.on("obj1", "on_click", function(self)
            self:setColor(255, 0, 0, 255)
        end)
    )");
    engine.fireEvent("on_click", obj.get());
    SUCCEED();
}

TEST(LuaScriptEngineBindings, HasTagViaUsertype) {
    dice::scripting::LuaScriptEngine engine;
    auto obj = std::make_shared<dice::core::GameObject>("obj1", "Object");
    obj->addTag("dice");

    bool tagFound = false;
    engine.registerFunction("cpp_check_tag", [&](bool v) { tagFound = v; });
    engine.executeGlobalScriptFromSource(R"(
        engine.on("obj1", "on_click", function(self)
            cpp_check_tag(self:hasTag("dice"))
        end)
    )");
    engine.fireEvent("on_click", obj.get());
    EXPECT_TRUE(tagFound);
}

TEST(LuaScriptEngineTriggerCatalog, ClearSceneStateClearsTriggers) {
    dice::scripting::LuaScriptEngine engine;

    engine.executeGlobalScriptFromSource(R"(
        engine.trigger("my_trigger", function(obj) end)
    )");

    engine.clearSceneState();

    // After clear, trigger should not fire (no crash, just returns false)
    auto obj = std::make_shared<dice::core::GameObject>("obj1", "Card");
    obj->setTriggerBinding("on_click", "my_trigger");
    const bool fired = engine.fireEvent("on_click", obj.get());
    EXPECT_FALSE(fired);
}
