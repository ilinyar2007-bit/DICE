return {
    on_hover = function(obj)
        local r = obj:getIntProperty("hover_r", 255)
        local g = obj:getIntProperty("hover_g", 255)
        local b = obj:getIntProperty("hover_b", 80)
        obj:setColor(r, g, b, 255)
    end,
    on_hover_exit = function(obj)
        local r = obj:getIntProperty("base_r", 255)
        local g = obj:getIntProperty("base_g", 255)
        local b = obj:getIntProperty("base_b", 255)
        local a = obj:getIntProperty("base_a", 255)
        obj:setColor(r, g, b, a)
    end
}
