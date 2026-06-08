# Перемешать дочерние объекты

![](../gifs/shuffle-children.gif)

## Сцена (JSON)

```json
{
  "scripts": ["scripts/snippets/shuffle_demo.lua", "scripts/snippets/_demo_overlay.lua"],
  "objects": [
    {
      "type": "GameObject",
      "id": "btn",
      "name": "Перемешать",
      "position": [400, 380],
      "scale": [2.5, 0.6],
      "textureFile": "assets/button.png",
      "zOrder": 1,
      "active": true,
      "visible": true,
      "draggable": false,
      "triggers": { "on_click": "shuffle_deck" }
    },
    {
      "type": "GameObject",
      "id": "deck",
      "name": "Колода",
      "position": [400, 210],
      "scale": [1.0, 1.0],
      "color": [255, 255, 255, 0],
      "textureFile": "assets/dieWhite_border1.png",
      "zOrder": 0,
      "active": true,
      "visible": true,
      "draggable": false,
      "children": [
        {"type":"GameObject","id":"c1","name":"1","position":[-40,-40],"scale":[1.0,1.0],"textureFile":"assets/dieWhite_border1.png","zOrder":1,"active":true,"visible":true,"draggable":false},
        {"type":"GameObject","id":"c2","name":"2","position":[0,0],"scale":[1.0,1.0],"textureFile":"assets/dieWhite_border3.png","zOrder":2,"active":true,"visible":true,"draggable":false},
        {"type":"GameObject","id":"c3","name":"3","position":[40,40],"scale":[1.0,1.0],"color":[255,100,100,255],"textureFile":"assets/dieRed_border5.png","zOrder":3,"active":true,"visible":true,"draggable":false}
      ]
    }
  ]
}
```

## Скрипт (Lua) — `scripts/snippets/shuffle_demo.lua`

```lua
engine.trigger("shuffle_deck", function(self)
    cpp_shuffle_children("deck")
    demo.showClick(self:getX(), self:getY())
end)

function draw()
    cpp_draw_text_center("Кликни — порядок отрисовки изменится", 400, 462, 13, 100, 100, 100)
end

function update(dt) end
```

## Что происходит

`cpp_shuffle_children(id)` перемешивает порядок дочерних объектов в массиве `children`. Это меняет порядок отрисовки depth-first — при следующем рендере объекты рисуются в новом порядке, и у перекрывающихся объектов меняется то, какой из них «сверху». Дочерние позиции задаются **относительно родителя**. Родитель должен быть `visible: true`, иначе трансформация не применяется к детям.
