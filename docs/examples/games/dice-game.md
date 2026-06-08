# Игра в кости

> Демонстрирует: `triggers`, `update(dt)`, `draw()`, `cpp_set_obj_texture`, `cpp_draw_text_*`, `game state в Lua`

## Механика

Два игрока по очереди бросают кубики, нажимая на них. Каждый бросок прибавляет выпавшее число к счёту игрока. Первый набравший 21 очко побеждает. Кнопка «Закончить ход» передаёт ход сопернику. Клавиша `R` перезапускает игру.

## Файлы

- `samples/scenes/demo.json`
- `samples/scripts/dice_demo.lua`

## Ключевые части сцены

```json
{
  "scripts": ["scripts/dice_demo.lua"],
  "objects": [
    {
      "id": "dice_1",
      "type": "GameObject",
      "textureFile": "assets/dieWhite_border1.png",
      "draggable": true,
      "triggers": { "on_click": "roll_dice" },
      "luaScript": "scripts/dice.lua",
      "properties": { "player": 1 }
    }
  ]
}
```

- `draggable: true` — кубик можно перетащить, а затем кликнуть.
- `triggers.on_click: "roll_dice"` — при клике вызывается триггер `"roll_dice"`.
- `luaScript` — подключает per-object скрипт (в данном случае пустой — логика в глобальном скрипте).
- `properties.player` — номер игрока; читается в триггере через `self:getIntProperty("player", 0)`.

## Ключевые части скрипта

```lua
-- Глобальное состояние игры
game = {
    currentPlayer = 1,
    scores        = {0, 0},
    targetScore   = 21,
    hasRolled     = false,
    gameOver      = false,
}

-- Бросок кубика
engine.trigger("roll_dice", function(self)
    local player = self:getIntProperty("player", 0)
    if game.gameOver or game.hasRolled or game.currentPlayer ~= player then
        return  -- не ход этого игрока или уже бросал
    end
    local roll = cpp_rand(1, 6)
    game.scores[player] = game.scores[player] + roll
    game.hasRolled = true
    if game.scores[player] >= game.targetScore then
        game.gameOver = true
    end
end)

-- В update меняем текстуру кубика по результату броска
function update(dt)
    -- cpp_set_obj_texture обновляет грань кубика каждый кадр
end

-- В draw рисуем счёт и статус
function draw()
    cpp_draw_text_left("Игрок 1: " .. game.scores[1], 20, 18, 26, 220, 80, 80)
    cpp_draw_text_right("Игрок 2: " .. game.scores[2], 1260, 18, 26, 80, 80, 220)
end
```

**Паттерн:** состояние игры (`game`) — глобальная Lua-таблица. `update(dt)` синхронизирует визуал с состоянием. `draw()` рисует UI-оверлей поверх объектов.

## Как запустить

```bash
cp samples/game.json .
ln -s samples/scenes scenes
ln -s samples/scripts scripts
ln -s samples/assets assets
./dice
```
