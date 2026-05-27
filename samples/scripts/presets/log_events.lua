local function log(ev, obj)
    cpp_log("[preset:" .. ev .. "] " .. obj:getId())
end

return {
    on_click      = function(obj) log("on_click",      obj) end,
    on_hover      = function(obj) log("on_hover",      obj) end,
    on_hover_exit = function(obj) log("on_hover_exit", obj) end,
    on_drag_start = function(obj) log("on_drag_start", obj) end,
    on_drag_end   = function(obj) log("on_drag_end",   obj) end
}
