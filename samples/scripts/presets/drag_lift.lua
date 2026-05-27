local savedZOrder = {}

return {
    on_drag_start = function(obj)
        local id = obj:getId()
        savedZOrder[id] = obj:getZOrder()
        obj:setZOrder(obj:getZOrder() + 50)
        local r = obj:getIntProperty("base_r", 255)
        local g = obj:getIntProperty("base_g", 255)
        local b = obj:getIntProperty("base_b", 255)
        obj:setColor(r, g, b, 210)
    end,
    on_drag_end = function(obj)
        local id = obj:getId()
        local saved = savedZOrder[id]
        if saved then
            obj:setZOrder(saved)
            savedZOrder[id] = nil
        end
        local r = obj:getIntProperty("base_r", 255)
        local g = obj:getIntProperty("base_g", 255)
        local b = obj:getIntProperty("base_b", 255)
        local a = obj:getIntProperty("base_a", 255)
        obj:setColor(r, g, b, a)
    end
}
