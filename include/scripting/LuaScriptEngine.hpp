#ifndef DICE_SCRIPTING_LUA_SCRIPT_ENGINE_HPP
#define DICE_SCRIPTING_LUA_SCRIPT_ENGINE_HPP

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}
#include <sol/sol.hpp>

#include <spdlog/spdlog.h>

namespace dice::core {
class GameObject;
} // namespace dice::core

namespace dice::core {
class Model;
}

#include "scripting/LuaScript.hpp"

namespace dice::scripting {

using UiCallback = std::function<void(const std::string&)>;

class LuaScriptEngine {
public:
    LuaScriptEngine();
    ~LuaScriptEngine() = default;

    LuaScriptEngine(const LuaScriptEngine&) = delete;
    LuaScriptEngine& operator=(const LuaScriptEngine&) = delete;
    LuaScriptEngine(LuaScriptEngine&&) = delete;
    LuaScriptEngine& operator=(LuaScriptEngine&&) = delete;

    std::unique_ptr<LuaScript> createFromSource(const std::string& source);

    std::unique_ptr<LuaScript> createFromFile(const std::filesystem::path& path);

    bool attachScript(dice::core::GameObject& obj, bool force_reload = false);

    bool fireEvent(const std::string& event_name, dice::core::GameObject* obj);

    void fireKeyEvent(const std::string& key_name);

    void detachScript(const std::string& object_id);

    void unregisterObject(const std::string& object_id);

    void registerCallback(const std::string& name, UiCallback callback);

    template <typename Func> void registerFunction(const std::string& name, Func&& func) {
        lua_.set_function(name, std::forward<Func>(func));
    }

    bool executeGlobalScript(const std::filesystem::path& path);

    bool executeGlobalScriptFromSource(const std::string& source);

    void clearSceneState();

    void registerModelAccess(dice::core::Model& model,
                             std::function<std::string()> get_current_path);
    void setSceneLoadCallback(std::function<void(const std::string&)> cb);

    sol::state& getLua() {
        return lua_;
    }

    template <typename... Args> void callGlobal(const std::string& name, Args&&... args) {
        const sol::protected_function fn = lua_[name];
        if (!fn.valid()) {
            return;
        }
        auto result = fn(std::forward<Args>(args)...);
        if (!result.valid()) {
            sol::error err = result;
            spdlog::error("LuaScriptEngine::callGlobal '{}': {}", name, err.what());
        }
    }

private:
    sol::state lua_;
    std::unordered_map<std::string, std::unique_ptr<LuaScript>> scriptRegistry_;
    std::unordered_map<std::string, UiCallback> callbacks_;
    std::unordered_map<std::string, std::unordered_map<std::string, sol::protected_function>>
        inlineCallbacks_;
    std::unordered_map<std::string, sol::protected_function> triggerCatalog_;
    std::unordered_map<std::string, sol::protected_function> keyHandlers_;

    std::function<void(const std::string&)> sceneLoadCallback_;

    void initLibraries();
    void registerGameObjectType();
    void registerStandardCallbacks();
    void registerEngineTable();
    sol::environment makeEnvironment();
};

} // namespace dice::scripting

#endif // DICE_SCRIPTING_LUA_SCRIPT_ENGINE_HPP
