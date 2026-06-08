# Ввод с клавиатуры

![](../gifs/keyboard-input.gif)

## Сцена (JSON)

```json
{
  "scripts": ["scripts/snippets/keyboard_demo.lua"],
  "objects": [
    {
      "type": "GameObject",
      "id": "piece",
      "name": "Фишка",
      "position": [640, 360],
      "scale": [1.0, 1.0],
      "textureFile": "assets/dieWhite_border1.png",
      "zOrder": 0,
      "active": true,
      "visible": true,
      "draggable": false
    }
  ]
}
```

## Скрипт (Lua) — `scripts/snippets/keyboard_demo.lua`

```lua
local pos = {x = 640, y = 360}
local step = 40

engine.onKey("Up",    function() pos.y = pos.y - step end)
engine.onKey("Down",  function() pos.y = pos.y + step end)
engine.onKey("Left",  function() pos.x = pos.x - step end)
engine.onKey("Right", function() pos.x = pos.x + step end)

function update(dt)
    local obj = engine.getObject("piece")
    if obj then obj:setPosition(pos.x, pos.y) end
end
```

## Что происходит

`engine.onKey` регистрирует обработчик под имя клавиши (строка). Доступные имена: `"Up"`, `"Down"`, `"Left"`, `"Right"`, `"Space"`, `"Enter"`, `"Tab"`, `"1"`–`"5"`, `"A"`–`"Z"`. Позиция применяется в `update()` чтобы не обращаться к объекту прямо из обработчика события.
