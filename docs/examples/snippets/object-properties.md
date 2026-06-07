# Свойства объекта (properties)

![](../gifs/object-properties.gif)

## Сцена (JSON)

```json
{
  "scripts": ["scripts/snippets/props_demo.lua"],
  "objects": [
    {
      "type": "GameObject",
      "id": "token",
      "name": "Фишка игрока",
      "position": [400, 155],
      "scale": [1.0, 1.0],
      "textureFile": "assets/dieWhite_border1.png",
      "zOrder": 0,
      "active": true,
      "visible": true,
      "draggable": false,
      "triggers": { "on_click": "show_info" },
      "properties": { "player": 1, "health": 100 }
    }
  ]
}
```

## Скрипт (Lua) — `scripts/snippets/props_demo.lua`

```lua
engine.trigger("show_info", function(self)
    local player = self:getIntProperty("player", 0)
    local health = self:getIntProperty("health", 0)
    log("Игрок: " .. player .. ", HP: " .. health)
end)
```

## Что происходит

`properties` в JSON — словарь произвольных данных объекта. Из Lua читается через `getIntProperty(key, default)` (аналогично — `getStringProperty`, `getBoolProperty`). Удобно хранить игровые данные прямо в объекте.
