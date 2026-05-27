local originalScale = {}

return {
    on_hover = function(obj)
        local id = obj:getId()
        originalScale[id] = { obj:getScaleX(), obj:getScaleY() }
        local factor = obj:getFloatProperty("hover_scale", 1.15)
        obj:setScale(originalScale[id][1] * factor, originalScale[id][2] * factor)
    end,
    on_hover_exit = function(obj)
        local id = obj:getId()
        local saved = originalScale[id]
        if saved then
            obj:setScale(saved[1], saved[2])
            originalScale[id] = nil
        end
    end
}
