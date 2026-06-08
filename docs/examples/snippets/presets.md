# Пресеты поведения

![](../gifs/presets.gif)

## `assets/presets.json`

```json
{
  "presets": {
    "Rollable": { "on_click": "roll_dice" }
  }
}
```

## Сцена (JSON)

```json
{
  "scripts": ["scripts/snippets/presets_demo.lua"],
  "objects": [
    {
      "type": "GameObject",
      "id": "die",
      "name": "Кубик",
      "position": [400, 250],
      "scale": [1.0, 1.0],
      "textureFile": "assets/dieWhite_border1.png",
      "zOrder": 0,
      "active": true,
      "visible": true,
      "draggable": false,
      "presets": ["Rollable"]
    }
  ]
}
```

## Скрипт (Lua) — `scripts/snippets/presets_demo.lua`

```lua
engine.trigger("roll_dice", function(self)
    local result = cpp_rand(1, 6)
    cpp_set_obj_texture(self:getId(), "assets/dieWhite_border" .. result .. ".png")
end)
```

## Что происходит

Пресет — это набор `{событие: имя_триггера}`, определённый в `assets/presets.json`. Поле `presets` в JSON объекта задаёт список применяемых пресетов. При загрузке сцены `Controller` мержит пресеты в `triggerBindings` объекта. Явные `triggers` в JSON имеют приоритет над пресетами. Один пресет можно применить ко множеству объектов.
