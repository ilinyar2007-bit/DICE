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
    registerEngineTable();
}

void LuaScriptEngine::initLibraries() {
    lua_.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
}

void LuaScriptEngine::registerGameObjectType() {
    lua_.new_usertype<dice::core::GameObject>(
        "GameObject",
        // позиция
        "getX",
        [](const dice::core::GameObject& o) { return o.getPosition().x; },
        "getY",
        [](const dice::core::GameObject& o) { return o.getPosition().y; },
        "setPosition",
        [](dice::core::GameObject& o, float x, float y) { o.setPosition(x, y); },
        // идентификация
        "getId",
        &dice::core::GameObject::getId,
        "getName",
        &dice::core::GameObject::getName,
        "setName",
        &dice::core::GameObject::setName,
        "getType",
        &dice::core::GameObject::getType,
        // состояние
        "isActive",
        &dice::core::GameObject::isActive,
        "setActive",
        &dice::core::GameObject::setActive,
        "isVisible",
        &dice::core::GameObject::isVisible,
        "setVisible",
        &dice::core::GameObject::setVisible,
        // z-порядок
        "getZOrder",
        &dice::core::GameObject::getZOrder,
        "setZOrder",
        &dice::core::GameObject::setZOrder,
        // трансформации (sf::Transformable)
        "getRotation",
        [](const dice::core::GameObject& o) { return o.getRotation(); },
        "setRotation",
        [](dice::core::GameObject& o, float a) { o.setRotation(a); },
        "getScaleX",
        [](const dice::core::GameObject& o) { return o.getScale().x; },
        "getScaleY",
        [](const dice::core::GameObject& o) { return o.getScale().y; },
        "setScale",
        [](dice::core::GameObject& o, float x, float y) { o.setScale(x, y); },
        // свойства
        "getIntProperty",
        [](const dice::core::GameObject& o, const std::string& k, int d) {
            return o.getProperty<int>(k, d);
        },
        "getFloatProperty",
        [](const dice::core::GameObject& o, const std::string& k, float d) {
            return o.getProperty<float>(k, d);
        },
        "getStringProperty",
        [](const dice::core::GameObject& o, const std::string& k, const std::string& d) {
            return o.getProperty<std::string>(k, d);
        },
        "getBoolProperty",
        [](const dice::core::GameObject& o, const std::string& k, bool d) {
            return o.getProperty<bool>(k, d);
        },
        "setIntProperty",
        [](dice::core::GameObject& o, const std::string& k, int v) { o.setProperty<int>(k, v); },
        "setStringProperty",
        [](dice::core::GameObject& o, const std::string& k, const std::string& v) {
            o.setProperty<std::string>(k, v);
        });
}

void LuaScriptEngine::registerEngineTable() {
    sol::table engine = lua_.create_named_table("engine");
    engine.set_function(
        "on",
        [this](const std::string& obj_id, const std::string& event, sol::protected_function fn) {
            inlineCallbacks_[obj_id][event] = std::move(fn);
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
    bool fired = false;

    auto sit = scriptRegistry_.find(obj->getId());
    if (sit != scriptRegistry_.end()) {
        fired |= sit->second->trigger(event_name, obj);
    }

    auto cit = inlineCallbacks_.find(obj->getId());
    if (cit != inlineCallbacks_.end()) {
        auto eit = cit->second.find(event_name);
        if (eit != cit->second.end()) {
            auto result = eit->second(obj);
            if (!result.valid()) {
                const sol::error err = result;
                spdlog::error("LuaScriptEngine: inline '{}' on '{}': {}",
                              event_name,
                              obj->getId(),
                              err.what());
            }
            fired = true;
        }
    }

    return fired;
}

bool LuaScriptEngine::executeGlobalScript(const std::filesystem::path& path) {
    auto result = lua_.script_file(path.string(), sol::script_pass_on_error);
    if (!result.valid()) {
        const sol::error err = result;
        spdlog::error("LuaScriptEngine: global script error '{}': {}", path.string(), err.what());
        return false;
    }
    spdlog::debug("LuaScriptEngine: executed global script '{}'", path.string());
    return true;
}

void LuaScriptEngine::detachScript(const std::string& object_id) {
    scriptRegistry_.erase(object_id);
    spdlog::debug("LuaScriptEngine: detached script from object '{}'", object_id);
}

void LuaScriptEngine::clearSceneState() {}

} // namespace dice::scripting
