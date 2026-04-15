-- Unified dice handler.
-- Each die stores its player number in properties: {"player": N}
-- cpp_roll_dice(player) is registered from C++ and returns the rolled value.

function on_click(self)
    local player = self:getIntProperty("player", 0)
    cpp_roll_dice(player)
end

function on_move(self)
    cpp_log(self:getName() .. ": (" ..
            math.floor(self:getX()) .. ", " .. math.floor(self:getY()) .. ")")
end
