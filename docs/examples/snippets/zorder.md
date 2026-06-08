# Порядок слоёв (zOrder)

![](../gifs/zorder.gif)

## Сцена (JSON)

```json
{
  "scripts": [],
  "objects": [
    {
      "type": "GameObject",
      "id": "back",
      "name": "Задний объект",
      "position": [400, 250],
      "scale": [1.0, 1.0],
      "color": [255, 80, 80, 255],
      "textureFile": "assets/dieWhite_border1.png",
      "zOrder": 0,
      "active": true,
      "visible": true,
      "draggable": false
    },
    {
      "type": "GameObject",
      "id": "front",
      "name": "Передний объект",
      "position": [440, 280],
      "scale": [1.0, 1.0],
      "color": [80, 80, 255, 255],
      "textureFile": "assets/dieRed_border1.png",
      "zOrder": 1,
      "active": true,
      "visible": true,
      "draggable": false
    }
  ]
}
```

## Что происходит

Объект с большим `zOrder` рисуется поверх. `front` (zOrder=1) перекрывает `back` (zOrder=0). Объекты в JSON можно перечислять в любом порядке — сортировка по zOrder происходит в `View`.
