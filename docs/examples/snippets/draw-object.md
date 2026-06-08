# Отрисовать объект

![](../gifs/draw-object.gif)

## Сцена (JSON)

```json
{
  "scripts": [],
  "objects": [
    {
      "type": "GameObject",
      "id": "die_white_1",
      "name": "Белый кубик 1",
      "position": [160, 180],
      "scale": [1.0, 1.0],
      "textureFile": "assets/dieWhite_border1.png",
      "zOrder": 1,
      "active": true,
      "visible": true,
      "draggable": false
    },
    {
      "type": "GameObject",
      "id": "die_white_5",
      "name": "Белый кубик 5",
      "position": [400, 180],
      "scale": [1.0, 1.0],
      "textureFile": "assets/dieWhite_border5.png",
      "zOrder": 1,
      "active": true,
      "visible": true,
      "draggable": false
    },
    {
      "type": "GameObject",
      "id": "die_red_3",
      "name": "Красный кубик 3",
      "position": [640, 180],
      "scale": [1.0, 1.0],
      "textureFile": "assets/dieRed_border3.png",
      "zOrder": 1,
      "active": true,
      "visible": true,
      "draggable": false
    },
    {
      "type": "GameObject",
      "id": "piece_white",
      "name": "Белая шашка",
      "position": [270, 360],
      "scale": [1.0, 1.0],
      "textureFile": "assets/pieceWhite_border11.png",
      "zOrder": 1,
      "active": true,
      "visible": true,
      "draggable": false
    },
    {
      "type": "GameObject",
      "id": "piece_black",
      "name": "Чёрная шашка",
      "position": [530, 360],
      "scale": [1.0, 1.0],
      "textureFile": "assets/pieceBlack_border11.png",
      "zOrder": 1,
      "active": true,
      "visible": true,
      "draggable": false
    }
  ]
}
```

## Что происходит

Каждый объект размещается по координатам `position` и отображается с текстурой из `assets/`. Lua-скрипт не нужен — только JSON. Можно использовать любые текстуры: кубики (`dieWhite_border*.png`, `dieRed_border*.png`) и шашки (`pieceWhite_border11.png`, `pieceBlack_border11.png`).
