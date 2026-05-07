#ifndef DICE_SCRIPTING_LUA_SCRIPT_HPP
#define DICE_SCRIPTING_LUA_SCRIPT_HPP

#include <string>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}
#include <sol/sol.hpp>

namespace dice::core {
class GameObject;
} // namespace dice::core

namespace dice::scripting {

inline constexpr const char* kEventOnClick = "on_click";
inline constexpr const char* kEventOnMove = "on_move";
inline constexpr const char* kEventOnDragStart = "on_drag_start";
inline constexpr const char* kEventOnDragEnd = "on_drag_end";
inline constexpr const char* kEventOnHover = "on_hover";
inline constexpr const char* kEventOnHoverExit = "on_hover_exit";

class LuaScript {
public:
    LuaScript(std::string source_code, sol::environment env, sol::state_view lua);

    bool load();

    [[nodiscard]] bool hasHandler(const std::string& event_name) const;

    bool trigger(const std::string& event_name, dice::core::GameObject* self);

    [[nodiscard]] const std::string& getSourceCode() const {
        return sourceCode_;
    }

private:
    std::string sourceCode_;
    sol::environment env_;
    sol::state_view lua_;
    bool loaded_ = false;
};

} // namespace dice::scripting

#endif // DICE_SCRIPTING_LUA_SCRIPT_HPP
