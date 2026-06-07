# Переключение сцен

![](../gifs/scene-switch.gif)

## Сцена (JSON)

```json
{
  "scripts": ["scripts/snippets/switch_demo.lua", "scripts/snippets/_demo_overlay.lua"],
  "objects": [
    {
      "type": "GameObject",
      "id": "btn",
      "name": "Другая сцена",
      "position": [400, 300],
      "scale": [3.0, 0.6],
      "textureFile": "assets/button.png",
      "zOrder": 0,
      "active": true,
      "visible": true,
      "draggable": false,
      "triggers": { "on_click": "go_nardi" }
    }
  ]
}
```

## Скрипт (Lua) — `scripts/snippets/switch_demo.lua`

```lua
engine.trigger("go_nardi", function(self)
    demo.showClick(self:getX(), self:getY())
    engine.loadScene("scenes/snippets/draw-object.json")
end)

function draw()
    cpp_draw_text_center("engine.loadScene()", 400, 150, 28, 180, 180, 180)
end

function update(dt) end
```

## Что происходит

`engine.loadScene(path)` откладывает загрузку до начала следующего кадра. Текущий кадр завершается штатно. При загрузке новой сцены Lua-состояние очищается, модель пересоздаётся из нового JSON.
