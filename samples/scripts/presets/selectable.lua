return {
    on_click = function(obj)
        local current = obj:getIntProperty("selected", 0)
        local next = current == 0 and 1 or 0
        obj:setIntProperty("selected", next)
        if next == 1 then
            obj:setColor(obj:getIntProperty("sel_r", 100),
                         obj:getIntProperty("sel_g", 200),
                         obj:getIntProperty("sel_b", 255), 255)
        else
            obj:setColor(obj:getIntProperty("base_r", 255),
                         obj:getIntProperty("base_g", 255),
                         obj:getIntProperty("base_b", 255),
                         obj:getIntProperty("base_a", 255))
        end
    end
}
