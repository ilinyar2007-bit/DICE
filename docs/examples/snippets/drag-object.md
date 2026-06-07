# Перетаскивание объекта

![](../gifs/drag-object.gif)

## Сцена (JSON)

```json
{
  "scripts": [],
  "objects": [
    {
      "type": "GameObject",
      "id": "chip",
      "name": "Фишка",
      "position": [400, 250],
      "scale": [1.0, 1.0],
      "textureFile": "assets/dieWhite_border1.png",
      "zOrder": 0,
      "active": true,
      "visible": true,
      "draggable": true
    }
  ]
}
```

## Что происходит

Поле `draggable: true` позволяет тащить объект мышью. Lua-скрипт не нужен. Движок ограничивает перемещение bounds объекта `"board"` — если объект с id `"board"` отсутствует, граница — весь экран.
