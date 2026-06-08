# Нарды (длинные)

> Демонстрирует: `drag`, `presets`, `properties`, `on_drag_end`, `engine.getObject`, `cpp_shuffle_children`

## Механика

Классические длинные нарды на двух игроков. Белые шашки (w1–w15) движутся от поля 12 к полю 1 и далее на вынос, чёрные (b1–b15) — в обратном направлении. Кубики бросаются кликом, шашки перетаскиваются мышью. Победа — первый снял все 15 шашек.

## Файлы

- `samples/scenes/nardi.json`
- `samples/scripts/nardi.lua`

## Ключевые части сцены

```json
{
  "id": "die_1",
  "textureFile": "assets/dieWhite_border1.png",
  "draggable": true,
  "presets": ["Rollable"]
}
```

```json
{
  "id": "w1",
  "textureFile": "assets/pieceWhite_border11.png",
  "draggable": true,
  "presets": ["Checker"],
  "properties": { "player": 1 }
}
```

```json
{
  "id": "pt1",
  "visible": false,
  "presets": ["PointMarker"],
  "properties": { "point_id": 1 }
}
```

- **`Rollable`** — пресет: `on_click → "roll_dice"`. Кубик бросается кликом.
- **`Checker`** — пресет: `on_drag_start → "checker_drag_start"`, `on_drag_end → "checker_drag_end"`. Шашка при отпускании проверяет валидность хода.
- **`PointMarker`** — пресет: `on_click → "dest_click"`. Невидимые маркеры точек поля — цель для перемещения шашки.
- **`properties.point_id`** — номер пункта (1–24, 0 и 25 — вынос). Читается в Lua для расчёта допустимых ходов.

## Ключевые части скрипта

```lua
-- Бросок кубиков
engine.trigger("roll_dice", function(self)
    if game.has_rolled then return end
    game.dice = {cpp_rand(1,6), cpp_rand(1,6)}
    game.has_rolled = true
    -- обновить текстуры кубиков
end)

-- Начало перетаскивания шашки
engine.trigger("checker_drag_start", function(self)
    game.dragging_id = self:getId()
    -- рассчитать и показать допустимые ходы
    game.valid_moves = calcValidMoves(self)
    showValidMoves(game.valid_moves)
end)

-- Конец перетаскивания: проверка хода
engine.trigger("checker_drag_end", function(self)
    local target_pt = findNearestValidPoint(self)
    if target_pt then
        applyMove(self, target_pt)  -- двигаем шашку, обновляем game.w/game.b
    else
        snapBack(self)              -- возврат на исходную позицию
    end
    hideValidMoves()
    game.dragging_id = nil
end)
```

**Паттерн:** `PointMarker`-объекты (`pt1`–`pt24`) служат anchor-точками. `on_drag_end` находит ближайший валидный маркер и применяет ход. Позиции шашек пересчитываются функцией `checkerPos(pt, idx, total)` для аккуратного стекирования.

## Как запустить

```bash
cp samples/game.json .
ln -s samples/scenes scenes
ln -s samples/scripts scripts
ln -s samples/assets assets
./dice
# Затем в game.json задать startScene: "scenes/nardi.json"
```
