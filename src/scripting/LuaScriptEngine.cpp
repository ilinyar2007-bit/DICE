#include "scripting/LuaScriptEngine.hpp"

#include <fstream>
#include <sstream>

#include "core/GameObject.hpp"
#include "scripting/LuaScript.hpp"
#include <spdlog/spdlog.h>

namespace dice::scripting {

LuaScriptEngine::LuaScriptEngine() {
    initLibraries();
    registerGameObjectType();
    registerStandardCallbacks();
}

void LuaScriptEngine::initLibraries() {
    lua_.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
}

void LuaScriptEngine::registerGameObjectType() {
    lua_.new_usertype<dice::core::GameObject>(
        "GameObject",
        "getX",
        [](const dice::core::GameObject& obj) { return obj.getPosition().x; },
        "getY",
        [](const dice::core::GameObject& obj) { return obj.getPosition().y; },
        "setPosition",
        [](dice::core::GameObject& obj, float x, float y) { obj.setPosition(x, y); },
        "getName",
        &dice::core::GameObject::getName,
        "setName",
        &dice::core::GameObject::setName,
        "isActive",
        &dice::core::GameObject::isActive,
        "setActive",
        &dice::core::GameObject::setActive,
        "isVisible",
        &dice::core::GameObject::isVisible,
        "setVisible",
        &dice::core::GameObject::setVisible,
        "getIntProperty",
        [](const dice::core::GameObject& obj, const std::string& key, int def) {
            return obj.getProperty<int>(key, def);
        });
}

void LuaScriptEngine::registerStandardCallbacks() {
    lua_.set_function("cpp_log", [this](const std::string& message) {
        spdlog::info("[Lua] {}", message);
        auto it = callbacks_.find("cpp_log");
        if (it != callbacks_.end()) {
            it->second(message);
        }
    });
}

void LuaScriptEngine::registerCallback(const std::string& name, UiCallback callback) {
    callbacks_[name] = std::move(callback);
    lua_.set_function(name, [this, name](const std::string& msg) {
        auto it = callbacks_.find(name);
        if (it != callbacks_.end()) {
            it->second(msg);
        }
    });
}


sol::environment LuaScriptEngine::makeEnvironment() {
    return {lua_, sol::create, lua_.globals()};
}

std::unique_ptr<LuaScript> LuaScriptEngine::createFromSource(const std::string& source) {
    auto env = makeEnvironment();
    return std::make_unique<LuaScript>(source, std::move(env), lua_);
}

std::unique_ptr<LuaScript> LuaScriptEngine::createFromFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        spdlog::error("LuaScriptEngine: cannot open script file '{}'", path.string());
        return nullptr;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return createFromSource(ss.str());
}

bool LuaScriptEngine::attachScript(dice::core::GameObject& obj, bool force_reload) {
    const std::string& id = obj.getId();
    const std::string& path = obj.getLuaScript();

    if (path.empty()) {
        spdlog::warn("LuaScriptEngine::attachScript: object '{}' has no script path", id);
        return false;
    }

    if (!force_reload && scriptRegistry_.contains(id)) {
        return true;
    }

    auto script = createFromFile(path);
    if (!script) {
        return false;
    }
    if (!script->load()) {
        return false;
    }

    scriptRegistry_[id] = std::move(script);
    spdlog::debug("LuaScriptEngine: attached script '{}' to object '{}'", path, id);
    return true;
}

bool LuaScriptEngine::fireEvent(const std::string& event_name, dice::core::GameObject* obj) {
    if (obj == nullptr) {
        return false;
    }
    auto it = scriptRegistry_.find(obj->getId());
    if (it == scriptRegistry_.end()) {
        return false;
    }
    return it->second->trigger(event_name, obj);
}

void LuaScriptEngine::detachScript(const std::string& object_id) {
    scriptRegistry_.erase(object_id);
    spdlog::debug("LuaScriptEngine: detached script from object '{}'", object_id);
}

} // namespace dice::scripting
