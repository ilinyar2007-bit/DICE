-- scripts/nardi.lua  — длинные нарды

-- ============================================================
-- Board constants
-- ============================================================
local PT_X = {
    1180, 1088,  996,  904,  812,  720,  -- 1-6   нижний правый (пр→лев)
     568,  476,  384,  292,  200,  108,  -- 7-12  нижний левый  (пр→лев)
     108,  200,  292,  384,  476,  568,  -- 13-18 верхний левый (лев→пр)
     720,  812,  904,  996, 1088, 1180   -- 19-24 верхний правый(лев→пр)
}

local PT_BASE_Y = {}
local PT_DIR    = {}
for i = 1, 12 do PT_BASE_Y[i] = 640; PT_DIR[i] = -1 end  -- нижний → стопка вверх
for i = 13, 24 do PT_BASE_Y[i] = 80;  PT_DIR[i] =  1 end  -- верхний → стопка вниз

local CHECKER_R   = 33
local MAX_STACK_H = 240  -- max stack height in px (half-board)

-- ============================================================
-- Game state
-- ============================================================
local game = {}

local function initGame()
    game.w = {}; game.b = {}
    for i = 1, 24 do game.w[i] = 0; game.b[i] = 0 end
    game.w[12] = 15   -- белые стартуют на поле 12 (нижний левый), идут к 13 (через 1 и 24)
    game.b[24] = 15   -- чёрные стартуют на поле 24 (верхний правый), идут к 1
    game.white_borne   = 0
    game.black_borne   = 0
    game.current_player = 1
    game.dice           = {}
    game.has_rolled     = false
    game.selected_pt    = 0
    game.valid_moves    = {}
    game.dragging_id     = nil
    game.moved_this_turn = false
    game.game_over      = false
    game.winner         = 0
    game.msg            = ""
    game.msg_timer      = 0
    game.auto_end_timer = 0
end

-- ============================================================
-- Helpers
-- ============================================================

local function checkerPos(pt, idx, total)
    local gap
    if total <= 1 then
        gap = 0
    elseif total <= 6 then
        gap = CHECKER_R * 2 + 2
    else
        gap = (MAX_STACK_H - CHECKER_R * 2) / (total - 1)
    end
    return PT_X[pt], PT_BASE_Y[pt] + PT_DIR[pt] * (idx - 1) * gap
end

local function whiteInHome()
    for i = 1, 12 do if game.w[i] > 0 then return false end end
    for i = 19, 24 do if game.w[i] > 0 then return false end end
    return true
end

local function blackInHome()
    for i = 7, 24 do if game.b[i] > 0 then return false end end
    return true
end

local function noWhiteAbove(pt)
    for i = pt + 1, 24 do if game.w[i] > 0 then return false end end
    return true
end

local function noBlackAbove(pt)
    for i = pt + 1, 24 do if game.b[i] > 0 then return false end end
    return true
end

-- ============================================================
-- Move logic
-- ============================================================

local function getValidMoves(from_pt)
    local moves = {}
    local count = game.current_player == 1 and game.w[from_pt] or game.b[from_pt]
    if count == 0 then return moves end

    local seen = {}
    for i, d in ipairs(game.dice) do
        if not seen[d] then
            seen[d] = true
            if game.current_player == 1 then
                local to = from_pt - d
                if from_pt >= 13 and from_pt <= 18 and to < 13 then
                    if whiteInHome() and (to == 12 or noWhiteAbove(from_pt)) then
                        table.insert(moves, { to_pt = 25, die_idx = i, die_val = d })
                    end
                else
                    if to < 1 then to = to + 24 end
                    if game.b[to] == 0 then
                        table.insert(moves, { to_pt = to, die_idx = i, die_val = d })
                    end
                end
            else
                local to = from_pt - d
                if to < 1 then
                    if blackInHome() and (to == 0 or noBlackAbove(from_pt)) then
                        table.insert(moves, { to_pt = 0, die_idx = i, die_val = d })
                    end
                elseif game.w[to] == 0 then
                    table.insert(moves, { to_pt = to, die_idx = i, die_val = d })
                end
            end
        end
    end
    return moves
end

local function hasAnyMove()
    for pt = 1, 24 do
        local count = game.current_player == 1 and game.w[pt] or game.b[pt]
        if count > 0 and #getValidMoves(pt) > 0 then return true end
    end
    return false
end

local function removeDie(idx)
    table.remove(game.dice, idx)
end

local function executeMove(from_pt, to_pt, die_idx)
    removeDie(die_idx)
    if game.current_player == 1 then
        game.w[from_pt] = game.w[from_pt] - 1
        if to_pt == 25 then
            game.white_borne = game.white_borne + 1
            if game.white_borne == 15 then
                game.game_over = true; game.winner = 1
            end
        else
            game.w[to_pt] = game.w[to_pt] + 1
        end
    else
        game.b[from_pt] = game.b[from_pt] - 1
        if to_pt == 0 then
            game.black_borne = game.black_borne + 1
            if game.black_borne == 15 then
                game.game_over = true; game.winner = 2
            end
        else
            game.b[to_pt] = game.b[to_pt] + 1
        end
    end
end

-- ============================================================
-- Visual helpers
-- ============================================================

local function hideAllPoints()
    for i = 1, 24 do
        local o = engine.getObject("pt" .. i)
        if o then o:setVisible(false) end
    end
    local bw = engine.getObject("pt_bearoff_w")
    if bw then bw:setVisible(false) end
    local bb = engine.getObject("pt_bearoff_b")
    if bb then bb:setVisible(false) end
end

local function showValidDestinations(moves)
    hideAllPoints()
    for _, m in ipairs(moves) do
        local o
        if m.to_pt == 25 then
            o = engine.getObject("pt_bearoff_w")
        elseif m.to_pt == 0 then
            o = engine.getObject("pt_bearoff_b")
        else
            o = engine.getObject("pt" .. m.to_pt)
        end
        if o then
            o:setVisible(true)
            o:setColor(60, 230, 80, 220)
        end
    end
end

local function updateCheckers()
    local wi = 1
    for pt = 1, 24 do
        local n = game.w[pt]
        for k = 1, n do
            local o = engine.getObject("w" .. wi)
            if o then
                o:setIntProperty("on_point", pt)
                if o:getId() ~= game.dragging_id then
                    local x, y = checkerPos(pt, k, n)
                    o:setPosition(x, y)
                    o:setVisible(true)
                end
                if pt == game.selected_pt and game.current_player == 1 then
                    o:setColor(255, 255, 80, 255)
                else
                    o:setColor(255, 255, 255, 255)
                end
            end
            wi = wi + 1
        end
    end
    while wi <= 15 do
        local o = engine.getObject("w" .. wi)
        if o then o:setVisible(false) end
        wi = wi + 1
    end

    local bi = 1
    for pt = 1, 24 do
        local n = game.b[pt]
        for k = 1, n do
            local o = engine.getObject("b" .. bi)
            if o then
                o:setIntProperty("on_point", pt)
                if o:getId() ~= game.dragging_id then
                    local x, y = checkerPos(pt, k, n)
                    o:setPosition(x, y)
                    o:setVisible(true)
                end
                if pt == game.selected_pt and game.current_player == 2 then
                    o:setColor(255, 200, 50, 255)
                else
                    o:setColor(255, 255, 255, 255)
                end
            end
            bi = bi + 1
        end
    end
    while bi <= 15 do
        local o = engine.getObject("b" .. bi)
        if o then o:setVisible(false) end
        bi = bi + 1
    end
end

local function setDiceTextures()
    local v1 = game.dice[1] or 1
    local v2 = (game.dice[2] or game.dice[1]) or 1
    cpp_set_obj_texture("die_1", "assets/dieWhite_border" .. v1 .. ".png")
    cpp_set_obj_texture("die_2", "assets/dieRed_border"   .. v2 .. ".png")
end

local function resetDiceTextures()
    cpp_set_obj_texture("die_1", "assets/dieWhite_border1.png")
    cpp_set_obj_texture("die_2", "assets/dieRed_border1.png")
end

local function updateDiceColors()
    local d1 = engine.getObject("die_1")
    local d2 = engine.getObject("die_2")
    if not d1 or not d2 then return end
    if game.has_rolled then
        d1:setColor(190, 190, 190, 200)
        d2:setColor(190, 190, 190, 200)
    else
        d1:setColor(255, 255, 255, 255)
        d2:setColor(255, 255, 255, 255)
    end
end

local function updateEndTurnButton()
    local btn = engine.getObject("btn_end_turn")
    if not btn then return end
    if game.game_over then
        btn:setVisible(false)
    elseif game.has_rolled then
        btn:setVisible(true); btn:setColor(80, 210, 100, 255)
    else
        btn:setVisible(true); btn:setColor(140, 140, 140, 180)
    end
end

-- ============================================================
-- Auto-end turn helper
-- ============================================================
local function doEndTurn()
    game.current_player  = game.current_player == 1 and 2 or 1
    game.has_rolled      = false
    game.dice            = {}
    game.selected_pt     = 0
    game.valid_moves     = {}
    game.moved_this_turn = false
    hideAllPoints()
    resetDiceTextures()
end

-- ============================================================
-- Triggers
-- ============================================================

engine.trigger("roll_dice", function(self)
    if game.game_over or game.has_rolled then return end
    local r1 = cpp_rand(1, 6)
    local r2 = cpp_rand(1, 6)
    if r1 == r2 then
        game.dice = { r1, r1, r1, r1 }
    else
        game.dice = { r1, r2 }
    end
    game.has_rolled = true
    setDiceTextures()
    if not hasAnyMove() then
        game.msg            = "Нет ходов — ход пропущен"
        game.msg_timer      = 2.0
        game.auto_end_timer = 2.0
    end
end)

engine.trigger("checker_drag_start", function(self)
    cpp_log("drag_start: id=" .. self:getId()
        .. " has_rolled=" .. tostring(game.has_rolled)
        .. " player=" .. self:getIntProperty("player",0)
        .. " current=" .. game.current_player
        .. " on_point=" .. self:getIntProperty("on_point",0))
    if game.game_over or not game.has_rolled then return end
    if self:getIntProperty("player", 0) ~= game.current_player then return end
    local pt = self:getIntProperty("on_point", 0)
    if pt == 0 then return end
    local moves = getValidMoves(pt)
    cpp_log("drag_start: moves=" .. #moves .. " dice=" .. #game.dice)
    if #moves == 0 then return end
    game.dragging_id = self:getId()
    game.selected_pt = pt
    game.valid_moves = moves
    showValidDestinations(moves)
end)

engine.trigger("checker_drag_end", function(self)
    local cx, cy = self:getX(), self:getY()
    cpp_log("drag_end: id=" .. self:getId()
        .. " pos=(" .. cx .. "," .. cy .. ")"
        .. " selected_pt=" .. game.selected_pt
        .. " moves=" .. #game.valid_moves)
    game.dragging_id = nil
    if game.selected_pt == 0 then return end

    local best_move = nil
    local best_dist = 70
    for _, m in ipairs(game.valid_moves) do
        local o
        if m.to_pt == 25 then
            o = engine.getObject("pt_bearoff_w")
        elseif m.to_pt == 0 then
            o = engine.getObject("pt_bearoff_b")
        else
            o = engine.getObject("pt" .. m.to_pt)
        end
        if o then
            local dx = cx - o:getX()
            local dy = cy - o:getY()
            local dist = math.sqrt(dx*dx + dy*dy)
            cpp_log("drag_end: checking pt" .. m.to_pt .. " obj_pos=(" .. o:getX() .. "," .. o:getY() .. ") dist=" .. math.floor(dist))
            if dist < best_dist then
                best_dist = dist
                best_move = m
            end
        end
    end

    local from_pt = game.selected_pt
    game.selected_pt = 0
    game.valid_moves = {}
    hideAllPoints()

    cpp_log("drag_end: best_move=" .. tostring(best_move ~= nil))
    if not best_move then return end

    executeMove(from_pt, best_move.to_pt, best_move.die_idx)
    game.moved_this_turn = true

    if not game.game_over and #game.dice == 0 then
        -- все кости использованы — ждём ручного завершения хода
    elseif not game.game_over and not hasAnyMove() then
        game.msg            = "Нет ходов — ход пропущен"
        game.msg_timer      = 2.0
        game.auto_end_timer = 2.0
    end
end)

engine.trigger("dest_click", function(self)
    if game.selected_pt == 0 then return end
    local to_pt = self:getIntProperty("point_id", -1)
    if to_pt == -1 then return end
    local move = nil
    for _, m in ipairs(game.valid_moves) do
        if m.to_pt == to_pt then move = m; break end
    end
    if not move then return end

    executeMove(game.selected_pt, to_pt, move.die_idx)
    game.selected_pt = 0
    game.valid_moves = {}
    hideAllPoints()

    if game.game_over then return end

    if #game.dice == 0 then
        -- all dice used — wait for manual end-turn click
    elseif not hasAnyMove() then
        game.msg            = "Нет ходов — ход пропущен"
        game.msg_timer      = 2.0
        game.auto_end_timer = 2.0
    end
end)

engine.trigger("end_turn", function(self)
    if game.game_over or not game.has_rolled then return end
    if not game.moved_this_turn and hasAnyMove() and #game.dice > 0 then return end
    game.auto_end_timer = 0
    game.msg            = ""
    doEndTurn()
end)

engine.trigger("new_game", function(self)
    initGame()
    hideAllPoints()
    resetDiceTextures()
    local ng = engine.getObject("btn_new_game")
    if ng then ng:setActive(false) end
end)

engine.onKey("R", function() engine.reloadScene() end)

-- ============================================================
-- Engine hooks
-- ============================================================

function update(dt)
    if game.msg_timer > 0 then
        game.msg_timer = game.msg_timer - dt
        if game.msg_timer <= 0 then game.msg = "" end
    end

    if game.auto_end_timer > 0 then
        game.auto_end_timer = game.auto_end_timer - dt
        if game.auto_end_timer <= 0 then
            doEndTurn()
        end
    end

    updateCheckers()
    updateDiceColors()
    updateEndTurnButton()

    local ng = engine.getObject("btn_new_game")
    if ng then ng:setActive(game.game_over) end
end

function draw()
    local C1 = { 255, 255, 200 }  -- белые
    local C2 = { 180, 180, 255 }  -- чёрные

    -- Текущий ход
    if not game.game_over then
        local pname = game.current_player == 1 and "Белые" or "Чёрные"
        local pc    = game.current_player == 1 and C1 or C2
        cpp_draw_text_center(">>> Ход: " .. pname .. " <<<", 640, 10, 22, table.unpack(pc))
    end

    -- Выброшено
    cpp_draw_text_left( "Белые: " .. game.white_borne .. "/15", 20,  10, 18, table.unpack(C1))
    cpp_draw_text_right("Чёрные: " .. game.black_borne .. "/15", 1260, 10, 18, table.unpack(C2))

    -- Значения кубиков
    if game.has_rolled then
        if #game.dice > 0 then
            local s = "Кубики: "
            for _, v in ipairs(game.dice) do s = s .. v .. "  " end
            cpp_draw_text_center(s, 640, 706, 18, 230, 215, 80)
        else
            cpp_draw_text_center("Кубики использованы — закончите ход", 640, 706, 16, 160, 160, 160)
        end
    elseif not game.game_over then
        cpp_draw_text_center("Кликни на кубик для броска", 640, 706, 16, 140, 140, 140)
    end

    -- Надпись на кнопке «Закончить ход»
    if not game.game_over then
        local tc = game.has_rolled and { 255, 255, 255 } or { 100, 100, 100 }
        cpp_draw_text_center("Закончить ход", 640, 688, 20, table.unpack(tc))
    end

    -- Надпись у зон выброса
    cpp_draw_text_center("Выход\nбелых", 35, 295, 14, table.unpack(C1))
    cpp_draw_text_center("Выход\nчёрных", 1245, 295, 14, table.unpack(C2))

    -- Временное сообщение
    if game.msg ~= "" then
        cpp_draw_rect(290, 338, 700, 44, 0, 0, 0, 170)
        cpp_draw_text_center(game.msg, 640, 352, 22, 255, 100, 100)
    end

    -- Подсказка: выбранная точка
    if game.selected_pt > 0 and not game.game_over then
        cpp_draw_text_center(
            "Выбрано поле " .. game.selected_pt .. " — кликни зелёный кружок",
            640, 695, 14, 80, 230, 100)
    end

    -- Оверлей победы
    if game.game_over then
        cpp_draw_rect(0, 0, 1280, 720, 0, 0, 0, 185)
        local wname = game.winner == 1 and "Белые" or "Чёрные"
        local wc    = game.winner == 1 and C1 or C2
        cpp_draw_text_center(wname .. " победили!", 640, 270, 72, table.unpack(wc))
        cpp_draw_text_center(
            "Выброшено: " .. (game.winner == 1 and game.white_borne or game.black_borne) .. " / 15",
            640, 370, 32, 255, 255, 255)
        cpp_draw_text_center("Новая игра", 640, 460, 26, 180, 255, 180)
        cpp_draw_text_center("R — перезапуск", 640, 500, 20, 150, 150, 150)
    end
end

-- ============================================================
-- Start
-- ============================================================
initGame()
