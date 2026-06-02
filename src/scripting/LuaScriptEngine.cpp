#include "scripting/LuaScriptEngine.hpp"

#include <fstream>
#include <sstream>

#include "core/GameObject.hpp"
#include "core/Model.hpp"
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

    lua_.set_exception_handler([](lua_State* l,
                                  sol::optional<const std::exception&> maybe_exception,
                                  sol::string_view description) {
        if (maybe_exception) {
            spdlog::error("Lua Exception: {}", maybe_exception->what());
        } else {
            spdlog::error("Lua Exception: {}", description);
        }
        return sol::stack::push(l, description);
    });
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
        },
        // цвет
        "setColor",
        [](dice::core::GameObject& o, int r, int g, int b, int a) {
            o.setColor(sf::Color(static_cast<sf::Uint8>(r),
                                 static_cast<sf::Uint8>(g),
                                 static_cast<sf::Uint8>(b),
                                 static_cast<sf::Uint8>(a)));
        },
        // теги
        "hasTag",
        [](const dice::core::GameObject& o, const std::string& tag) -> bool {
            return o.hasTag(tag);
        },
        "getTags",
        [](const dice::core::GameObject& o) -> std::vector<std::string> { return o.getTags(); });
}

void LuaScriptEngine::registerEngineTable() {
    sol::table engine = lua_.create_named_table("engine");
    engine.set_function(
        "on",
        [this](const std::string& obj_id, const std::string& event, sol::protected_function fn) {
            inlineCallbacks_[obj_id][event] = std::move(fn);
        });
    engine.set_function("trigger", [this](const std::string& name, sol::protected_function fn) {
        triggerCatalog_[name] = std::move(fn);
    });
    engine.set_function("onKey", [this](const std::string& key_name, sol::protected_function fn) {
        if (keyHandlers_.contains(key_name)) {
            spdlog::warn("LuaScriptEngine: onKey '{}' handler overwritten", key_name);
        }
        keyHandlers_[key_name] = std::move(fn);
    });
}

void LuaScriptEngine::registerStandardCallbacks() {
    lua_.set_function("cpp_log", [this](const std::string& message) {
        spdlog::info("[Lua] {}", message);
        if (auto it = callbacks_.find("cpp_log"); it != callbacks_.end()) {
            it->second(message);
        }
    });
}

void LuaScriptEngine::registerCallback(const std::string& name, UiCallback callback) {
    callbacks_[name] = std::move(callback);
    lua_.set_function(name, [this, name](const std::string& msg) {
        if (auto it = callbacks_.find(name); it != callbacks_.end()) {
            it->second(msg);
        }
    });
}


sol::environment LuaScriptEngine::makeEnvironment() {
    return {lua_, sol::create, lua_.globals()};
}

std::unique_ptr<LuaScript> LuaScriptEngine::createFromSource(const std::string& source) {
    if (auto env = makeEnvironment(); env) {
        return std::make_unique<LuaScript>(source, std::move(env), lua_);
    }
    return nullptr;
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

bool LuaScriptEngine::fireBindingRef(const std::string& ref, dice::core::GameObject* obj) {
    if (const auto sep = ref.find(':'); sep != std::string::npos) {
        const std::string file = ref.substr(0, sep);
        const std::string func = ref.substr(sep + 1);
        sol::table mod = loadOrGetCachedModule(file);
        if (!mod.valid()) {
            return false;
        }
        const sol::protected_function fn = mod[func];
        if (!fn.valid()) {
            return false;
        }
        if (auto result = fn(obj); !result.valid()) {
            const sol::error err = result;
            spdlog::error("LuaScriptEngine: '{}' on '{}': {}", ref, obj->getId(), err.what());
        }
        return true;
    }
    if (auto tit = triggerCatalog_.find(ref); tit != triggerCatalog_.end()) {
        if (auto result = tit->second(obj); !result.valid()) {
            const sol::error err = result;
            spdlog::error(
                "LuaScriptEngine: trigger '{}' on '{}': {}", ref, obj->getId(), err.what());
        }
        return true;
    }
    return false;
}

bool LuaScriptEngine::fireEvent(const std::string& event_name, dice::core::GameObject* obj) {
    if (obj == nullptr) {
        return false;
    }
    bool fired = false;

    if (auto sit = scriptRegistry_.find(obj->getId()); sit != scriptRegistry_.end()) {
        fired |= sit->second->trigger(event_name, obj);
    }

    if (auto cit = inlineCallbacks_.find(obj->getId()); cit != inlineCallbacks_.end()) {
        if (auto eit = cit->second.find(event_name); eit != cit->second.end()) {
            if (auto result = eit->second(obj); !result.valid()) {
                const sol::error err = result;
                spdlog::error("LuaScriptEngine: inline '{}' on '{}': {}",
                              event_name,
                              obj->getId(),
                              err.what());
            }
            fired = true;
        }
    }

    const auto& bindings = obj->getTriggerBindings();
    if (auto bit = bindings.find(event_name); bit != bindings.end()) {
        fired |= fireBindingRef(bit->second, obj);
    }

    return fired;
}

sol::table LuaScriptEngine::loadOrGetCachedModule(const std::string& filepath) {
    if (auto it = moduleCache_.find(filepath); it != moduleCache_.end()) {
        return it->second;
    }
    auto result = lua_.script_file(filepath, sol::script_pass_on_error);
    if (!result.valid()) {
        const sol::error err = result;
        spdlog::error("LuaScriptEngine: cannot load module '{}': {}", filepath, err.what());
        return sol::lua_nil;
    }
    sol::table mod = result;
    if (!mod.valid()) {
        spdlog::error("LuaScriptEngine: module '{}' did not return a table", filepath);
        return sol::lua_nil;
    }
    moduleCache_[filepath] = mod;
    return mod;
}

bool LuaScriptEngine::executeGlobalScriptFromSource(const std::string& source) {
    auto result = lua_.script(source, sol::script_pass_on_error);
    if (!result.valid()) {
        const sol::error err = result;
        spdlog::error("LuaScriptEngine: script source error: {}", err.what());
        return false;
    }
    return true;
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

void LuaScriptEngine::unregisterObject(const std::string& object_id) {
    scriptRegistry_.erase(object_id);
    inlineCallbacks_.erase(object_id);
    spdlog::debug("LuaScriptEngine: unregistered object '{}'", object_id);
}

void LuaScriptEngine::loadPresets(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        spdlog::warn("LuaScriptEngine::loadPresets: cannot open '{}'", path.string());
        return;
    }
    try {
        loadPresetsFromJson(nlohmann::json::parse(file));
    } catch (const nlohmann::json::exception& e) {
        spdlog::error(
            "LuaScriptEngine::loadPresets: parse error in '{}': {}", path.string(), e.what());
    }
}

void LuaScriptEngine::loadPresetsFromJson(const nlohmann::json& j) {
    const auto& presets = j.find("presets");
    if (presets == j.end() || !presets->is_object()) {
        return;
    }
    for (const auto& [name, bindings] : presets->items()) {
        if (!bindings.is_object()) {
            continue;
        }
        auto& catalog_entry = globalPresetCatalog_[name];
        for (const auto& [event, ref] : bindings.items()) {
            if (ref.is_string()) {
                catalog_entry[event] = ref.get<std::string>();
            }
        }
    }
}

void LuaScriptEngine::clearSceneState() {
    scriptRegistry_.clear();
    inlineCallbacks_.clear();
    triggerCatalog_.clear();
    keyHandlers_.clear();
}

void LuaScriptEngine::fireKeyEvent(const std::string& key_name) {
    auto it = keyHandlers_.find(key_name);
    if (it == keyHandlers_.end()) {
        return;
    }
    if (auto result = it->second(); !result.valid()) {
        const sol::error err = result;
        spdlog::error("LuaScriptEngine: onKey '{}': {}", key_name, err.what());
    }
}

void LuaScriptEngine::registerModelAccess(dice::core::Model& model,
                                          std::function<std::string()> get_current_path) {
    sol::table engine = lua_["engine"];
    if (!engine.valid()) {
        spdlog::error("LuaScriptEngine::registerModelAccess: 'engine' table not found");
        return;
    }

    engine.set_function("getObject",
                        [&model](const std::string& id) { return model.getObject(id); });

    engine.set_function("loadScene", [this](const std::string& path) {
        if (sceneLoadCallback_) {
            sceneLoadCallback_(path);
        }
    });

    engine.set_function("reloadScene", [this, get_current_path = std::move(get_current_path)]() {
        if (sceneLoadCallback_) {
            if (auto path = get_current_path(); !path.empty()) {
                sceneLoadCallback_(path);
            }
        }
    });
}

void LuaScriptEngine::setSceneLoadCallback(std::function<void(const std::string&)> cb) {
    sceneLoadCallback_ = std::move(cb);
}

} // namespace dice::scripting
