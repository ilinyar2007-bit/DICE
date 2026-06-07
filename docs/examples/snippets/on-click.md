# Клик по объекту

![](../gifs/on-click.gif)

## Сцена (JSON)

```json
{
  "scripts": ["scripts/snippets/click_demo.lua"],
  "objects": [
    {
      "type": "GameObject",
      "id": "die",
      "name": "Кубик",
      "position": [640, 360],
      "scale": [1.0, 1.0],
      "textureFile": "assets/dieWhite_border1.png",
      "zOrder": 0,
      "active": true,
      "visible": true,
      "draggable": false,
      "triggers": { "on_click": "on_die_click" }
    }
  ]
}
```

## Скрипт (Lua) — `scripts/snippets/click_demo.lua`

```lua
engine.trigger("on_die_click", function(self)
    local roll = cpp_rand(1, 6)
    cpp_set_obj_texture(self:getId(), "assets/dieWhite_border" .. roll .. ".png")
    log("Выпало: " .. roll)
end)
```

## Что происходит

Поле `triggers` в JSON связывает событие `on_click` с именем триггера `"on_die_click"`. При клике на кубик движок вызывает Lua-функцию, зарегистрированную через `engine.trigger("on_die_click", ...)`. `self` — это объект, по которому кликнули.
