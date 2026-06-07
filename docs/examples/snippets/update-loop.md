# Update-цикл (анимация)

![](../gifs/update-loop.gif)

## Сцена (JSON)

```json
{
  "scripts": ["scripts/snippets/update_demo.lua"],
  "objects": [
    {
      "type": "GameObject",
      "id": "piece",
      "name": "Фишка",
      "position": [100, 360],
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

## Скрипт (Lua) — `scripts/snippets/update_demo.lua`

```lua
local x = 100
local speed = 300  -- пикселей в секунду

function update(dt)
    x = x + speed * dt
    if x > 1180 then x = 100 end
    local obj = engine.getObject("piece")
    if obj then obj:setPosition(x, 360) end
end
```

## Что происходит

`update(dt)` вызывается движком каждый кадр. `dt` — время в секундах с прошлого кадра (ограничено сверху 0.05 с). Умножение скорости на `dt` делает анимацию независимой от FPS.
