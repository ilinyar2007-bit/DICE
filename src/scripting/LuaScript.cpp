#include "scripting/LuaScript.hpp"

#include "core/GameObject.hpp"
#include <spdlog/spdlog.h>

namespace dice::scripting {

LuaScript::LuaScript(std::string source_code, sol::environment env, sol::state_view lua)
    : sourceCode_(std::move(source_code)), env_(std::move(env)), lua_(std::move(lua)) {}

bool LuaScript::load() {
    auto result = lua_.script(sourceCode_, env_, sol::script_pass_on_error);
    if (!result.valid()) {
        const sol::error err = result;
        spdlog::error("LuaScript::load() failed: {}", err.what());
        loaded_ = false;
        return false;
    }
    loaded_ = true;
    return true;
}

bool LuaScript::hasHandler(const std::string& event_name) const {
    if (!loaded_) {
        return false;
    }
    const sol::object obj = env_[event_name];
    return obj.is<sol::function>();
}

bool LuaScript::trigger(const std::string& event_name, dice::core::GameObject* self) {
    if (!loaded_) {
        spdlog::warn("LuaScript::trigger('{}') called before load()", event_name);
        return false;
    }

    sol::optional<sol::protected_function> handler = env_[event_name];
    if (!handler) {
        return false;
    }

    auto result = (*handler)(self);
    if (!result.valid()) {
        const sol::error err = result;
        spdlog::error("LuaScript::trigger('{}') error: {}", event_name, err.what());
        return false;
    }
    return true;
}

} // namespace dice::scripting
