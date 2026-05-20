-- scripts/dice.lua
-- Per-object script for dice (optional).
-- on_click is now handled via "triggers" in JSON (engine.trigger "roll_dice").

function on_move(self)
    cpp_log(self:getName() .. ": (" ..
            math.floor(self:getX()) .. ", " .. math.floor(self:getY()) .. ")")
end
