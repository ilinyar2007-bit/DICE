# Ховер (подсветка при наведении)

![](../gifs/on-hover.gif)

## Сцена (JSON)

```json
{
  "scripts": ["scripts/snippets/hover_demo.lua", "scripts/snippets/_demo_overlay.lua"],
  "objects": [
    {
      "type": "GameObject",
      "id": "btn",
      "name": "Кнопка",
      "position": [400, 250],
      "scale": [2.0, 0.6],
      "textureFile": "assets/button.png",
      "zOrder": 0,
      "active": true,
      "visible": true,
      "draggable": false,
      "triggers": {
        "on_hover":      "btn_hover",
        "on_hover_exit": "btn_hover_exit"
      }
    }
  ]
}
```

## Скрипт (Lua) — `scripts/snippets/hover_demo.lua`

```lua
local hovered = false

engine.trigger("btn_hover", function(self)
    hovered = true
    cpp_set_obj_color(self:getId(), 255, 220, 80, 255)
end)

engine.trigger("btn_hover_exit", function(self)
    hovered = false
    cpp_set_obj_color(self:getId(), 255, 255, 255, 255)
end)

function draw()
    demo.drawState({hovered = tostring(hovered)})
end

function update(dt) end
```

## Что происходит

`on_hover` срабатывает когда курсор входит в bounds объекта, `on_hover_exit` — когда выходит. Здесь используется для подсветки кнопки жёлтым цветом при наведении. Текущее состояние отображается в углу через `demo.drawState`.
