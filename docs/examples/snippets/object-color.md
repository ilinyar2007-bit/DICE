# Цвет объекта

![](../gifs/object-color.gif)

## Сцена (JSON)

```json
{
  "scripts": ["scripts/snippets/color_demo.lua"],
  "objects": [
    {
      "type": "GameObject",
      "id": "piece",
      "name": "Фишка",
      "position": [400, 250],
      "scale": [1.0, 1.0],
      "color": [255, 0, 0, 255],
      "textureFile": "assets/dieWhite_border1.png",
      "zOrder": 0,
      "active": true,
      "visible": true,
      "draggable": false,
      "triggers": { "on_click": "change_color" }
    }
  ]
}
```

## Скрипт (Lua) — `scripts/snippets/color_demo.lua`

```lua
local colors = {
    {255, 0,   0,   255},
    {0,   255, 0,   255},
    {0,   0,   255, 255},
}
local idx = 1

engine.trigger("change_color", function(self)
    idx = idx % #colors + 1
    local c = colors[idx]
    cpp_set_obj_color(self:getId(), c[1], c[2], c[3], c[4])
end)
```

## Что происходит

Цвет задаётся в JSON через поле `color` (RGBA). По клику Lua циклически меняет его через `cpp_set_obj_color`. Цвет умножается на текстуру объекта — белая текстура даёт чистый цвет, цветная текстура смешивается.
