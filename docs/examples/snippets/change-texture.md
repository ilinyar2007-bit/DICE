# Смена текстуры из Lua

![](../gifs/change-texture.gif)

## Сцена (JSON)

```json
{
  "scripts": ["scripts/snippets/texture_demo.lua"],
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
      "triggers": { "on_click": "next_face" }
    }
  ]
}
```

## Скрипт (Lua) — `scripts/snippets/texture_demo.lua`

```lua
local face = 1

engine.trigger("next_face", function(self)
    face = face % 6 + 1
    cpp_set_obj_texture(self:getId(), "assets/dieWhite_border" .. face .. ".png")
end)
```

## Что происходит

`cpp_set_obj_texture(id, path)` загружает текстуру по пути (кэшируется — повторная загрузка того же файла бесплатна) и применяет к объекту. Клик циклически переключает грани кубика от 1 до 6.
