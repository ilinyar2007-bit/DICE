# Видимость объекта

![](../gifs/object-visibility.gif)

## Сцена (JSON)

```json
{
  "scripts": ["scripts/snippets/visibility_demo.lua", "scripts/snippets/_demo_overlay.lua"],
  "objects": [
    {
      "type": "GameObject",
      "id": "btn",
      "name": "Кнопка",
      "position": [400, 380],
      "scale": [2.0, 0.6],
      "textureFile": "assets/button.png",
      "zOrder": 1,
      "active": true,
      "visible": true,
      "draggable": false,
      "triggers": { "on_click": "toggle_piece" }
    },
    {
      "type": "GameObject",
      "id": "piece",
      "name": "Фишка",
      "position": [400, 200],
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

## Скрипт (Lua) — `scripts/snippets/visibility_demo.lua`

```lua
local visible = true

engine.trigger("toggle_piece", function(self)
    local piece = engine.getObject("piece")
    if piece then
        visible = not piece:isVisible()
        piece:setVisible(visible)
        demo.showClick(self:getX(), self:getY())
    end
end)

function draw()
    demo.drawState({visible = tostring(visible)})
end

function update(dt) end
```

## Что происходит

Клик по кнопке переключает видимость фишки. `setVisible(false)` скрывает объект — он не рендерится, но остаётся в модели и по-прежнему реагирует на события. Текущее состояние отображается в углу через `demo.drawState`.
