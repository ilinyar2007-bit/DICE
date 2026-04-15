-- End turn button handler
-- cpp_end_turn() is registered from C++ and advances the turn to the next player

function on_click(self)
    cpp_end_turn()
end
