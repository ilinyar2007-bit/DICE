-- scripts/game.lua
-- Game state and logic for the DICE demo.
-- Loaded once per scene via "scripts" array in demo.json.

game = {
    currentPlayer = 1,
    scores        = {0, 0},
    diceRoll      = {0, 0},
    hasRolled     = false,
    gameOver      = false,
    winner        = 0,
    targetScore   = 21
}

-- ===== Trigger catalog =====

engine.trigger("roll_dice", function(self)
    local player = self:getIntProperty("player", 0)
    if game.gameOver or game.hasRolled or game.currentPlayer ~= player then
        return
    end
    local roll = cpp_rand(1, 6)
    game.diceRoll[player]  = roll
    game.scores[player]    = game.scores[player] + roll
    game.hasRolled         = true
    cpp_log("Игрок " .. player .. " бросил " .. roll ..
            " — итого: " .. game.scores[player])
    if game.scores[player] >= game.targetScore then
        game.gameOver = true
        game.winner   = player
        cpp_log("Игрок " .. player .. " победил!")
    end
end)

engine.trigger("end_turn", function(self)
    if game.gameOver or not game.hasRolled then return end
    game.currentPlayer = game.currentPlayer == 1 and 2 or 1
    game.hasRolled     = false
    cpp_log("Ход передан Игроку " .. game.currentPlayer)
end)

-- ===== Keyboard =====

engine.onKey("R", function()
    engine.reloadScene()
end)

-- ===== Engine hooks =====

function update(dt)
    local d1  = engine.getObject("dice_1")
    local d2  = engine.getObject("dice_2")
    local btn = engine.getObject("end_turn_btn")
    if not d1 or not d2 or not btn then return end

    local function updateDie(obj, player, prefix)
        local roll = game.diceRoll[player]
        if roll > 0 then
            cpp_set_obj_texture(obj:getId(), "assets/" .. prefix .. roll .. ".png")
        end
        if game.currentPlayer == player and not game.hasRolled and not game.gameOver then
            obj:setColor(255, 255, 255, 255)
        else
            obj:setColor(160, 150, 130, 180)
        end
    end

    updateDie(d1, 1, "dieWhite_border")
    updateDie(d2, 2, "dieRed_border")

    if game.gameOver then
        btn:setColor(80, 80, 80, 180)
    elseif game.hasRolled then
        btn:setColor(80, 220, 100, 255)
    else
        btn:setColor(150, 150, 150, 200)
    end
end

function draw()
    local P1  = {220, 80,  80}
    local P2  = {80,  80,  220}
    local cur = game.currentPlayer == 1 and P1 or P2

    cpp_draw_text_left(
        "Игрок 1: " .. game.scores[1] .. " / " .. game.targetScore,
        20, 18, 26, table.unpack(P1))
    cpp_draw_text_right(
        "Игрок 2: " .. game.scores[2] .. " / " .. game.targetScore,
        1260, 18, 26, table.unpack(P2))

    if not game.gameOver then
        cpp_draw_text_center(
            ">>> Ход Игрока " .. game.currentPlayer .. " <<<",
            640, 22, 26, table.unpack(cur))
    end

    local btnColor = (game.hasRolled and not game.gameOver)
                     and {255, 255, 255} or {160, 160, 160}
    cpp_draw_text_center("Закончить ход", 640, 660, 22, table.unpack(btnColor))

    cpp_draw_text_left(
        "Перемещай кубик | Кликни чтобы бросить | R — перезапуск",
        20, 700, 15, 130, 130, 130)

    if game.gameOver then
        cpp_draw_rect(0, 0, 1280, 720, 0, 0, 0, 190)
        cpp_draw_text_center(
            "Игрок " .. game.winner .. " ПОБЕДИЛ!",
            640, 300, 72, table.unpack(game.winner == 1 and P1 or P2))
        cpp_draw_text_center(
            "Счёт: " .. game.scores[game.winner] .. " очков",
            640, 390, 36, 255, 255, 255)
        cpp_draw_text_center(
            "Нажмите R для перезапуска или ESC для выхода",
            640, 450, 24, 180, 180, 180)
    end
end
