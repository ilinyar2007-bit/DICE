-- Unified dice handler.
-- Each die stores its player number in properties: {"player": N}
-- game.roll() is defined in scripts/game.lua (loaded as global).

function on_click(self)
    local player = self:getIntProperty("player", 0)
    game.roll(player)
end

function on_move(self)
    cpp_log(self:getName() .. ": (" ..
            math.floor(self:getX()) .. ", " .. math.floor(self:getY()) .. ")")
end
