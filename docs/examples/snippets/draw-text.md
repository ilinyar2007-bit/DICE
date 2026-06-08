# Рисовать текст

![](../gifs/draw-text.gif)

## Сцена (JSON)

```json
{
  "scripts": ["scripts/snippets/text_demo.lua"],
  "objects": []
}
```

## Скрипт (Lua) — `scripts/snippets/text_demo.lua`

```lua
local score = 0

engine.onKey("Space", function()
    score = score + 1
end)

function draw()
    cpp_draw_text_center("Счёт: " .. score,      640, 300, 48, 255, 255, 255)
    cpp_draw_text_left("Пробел — добавить очко",  20,  680, 20, 180, 180, 180)
end
```

## Что происходит

`cpp_draw_text_*` вызываются из глобальной функции `draw()`, которую движок вызывает каждый кадр после рендеринга объектов. Сигнатура: `(текст, x, y, размер, r, g, b)`. Текст не сохраняется — нужно рисовать каждый кадр заново.
