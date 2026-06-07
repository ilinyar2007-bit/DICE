# Случайные числа

![](../gifs/random.gif)

## Сцена (JSON)

```json
{
  "scripts": ["scripts/snippets/random_demo.lua"],
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
      "triggers": { "on_click": "roll" }
    }
  ]
}
```

## Скрипт (Lua) — `scripts/snippets/random_demo.lua`

```lua
engine.trigger("roll", function(self)
    local result = cpp_rand(1, 6)
    cpp_set_obj_texture(self:getId(), "assets/dieWhite_border" .. result .. ".png")
    log("Выпало: " .. result)
end)
```

## Что происходит

`cpp_rand(lo, hi)` возвращает равномерно распределённое целое в \[lo, hi\] включительно. Генератор инициализируется из `std::random_device` — результаты не воспроизводимы между запусками.
