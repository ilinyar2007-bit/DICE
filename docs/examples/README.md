# Примеры DICE

Здесь собраны примеры для тех, кто создаёт игры с помощью DICE.

## Как запустить любой пример

1. Скопируйте нужный JSON-файл как `game.json` в корень проекта (или укажите путь в `startScene`).
2. Убедитесь что ресурсы доступны:
   ```bash
   ln -s samples/assets assets
   ln -s samples/scenes scenes
   ln -s samples/scripts scripts
   ```
3. Запустите:
   ```bash
   ./dice
   ```

> Все сниппеты используют ресурсы из `samples/assets/`. Символическая ссылка `assets → samples/assets` обязательна.

---

## Сниппеты

Изолированные примеры одной конкретной техники. Идеально для копипасты.

### Базовые объекты

| | Сниппет | Описание | Теги |
|--|---------|----------|------|
| ![](gifs/draw-object.gif) | [Отрисовать объект](snippets/draw-object.md) | Разместить объект с текстурой на сцене | `json` |
| ![](gifs/object-color.gif) | [Цвет объекта](snippets/object-color.md) | Задать цвет в JSON и сменить из Lua | `json` `lua` |
| ![](gifs/zorder.gif) | [Порядок слоёв](snippets/zorder.md) | Управлять zOrder для перекрытия объектов | `json` |
| ![](gifs/object-visibility.gif) | [Видимость объекта](snippets/object-visibility.md) | Скрыть / показать объект из Lua | `lua` |

### Взаимодействие

| | Сниппет | Описание | Теги |
|--|---------|----------|------|
| ![](gifs/on-click.gif) | [Клик по объекту](snippets/on-click.md) | Реакция на нажатие через `triggers` | `triggers` `lua` |
| ![](gifs/drag-object.gif) | [Перетаскивание](snippets/drag-object.md) | Объект с `draggable: true` | `json` |
| ![](gifs/on-hover.gif) | [Ховер](snippets/on-hover.md) | Подсветка при наведении мыши | `triggers` `lua` |
| ![](gifs/keyboard-input.gif) | [Клавиатура](snippets/keyboard-input.md) | Двигать объект стрелками | `lua` `input` |

### Lua-скриптинг

| | Сниппет | Описание | Теги |
|--|---------|----------|------|
| ![](gifs/change-texture.gif) | [Смена текстуры](snippets/change-texture.md) | Менять текстуру объекта из Lua | `lua` `texture` |
| ![](gifs/draw-text.gif) | [Рисовать текст](snippets/draw-text.md) | Вывод текста через `cpp_draw_text_*` | `lua` `ui` |
| ![](gifs/draw-rect.gif) | [Рисовать прямоугольник](snippets/draw-rect.md) | Рисовать фигуры через `cpp_draw_rect` | `lua` `ui` |
| ![](gifs/random.gif) | [Случайные числа](snippets/random.md) | Бросить кубик через `cpp_rand` | `lua` |
| ![](gifs/update-loop.gif) | [Update-цикл](snippets/update-loop.md) | Анимация через `update(dt)` | `lua` `animation` |

### Объекты и данные

| | Сниппет | Описание | Теги |
|--|---------|----------|------|
| ![](gifs/object-properties.gif) | [Свойства объекта](snippets/object-properties.md) | Кастомные данные через `properties` | `json` `lua` |
| ![](gifs/object-hierarchy.gif) | [Иерархия объектов](snippets/object-hierarchy.md) | Вложенные объекты через `children` | `json` |
| ![](gifs/shuffle-children.gif) | [Перемешать детей](snippets/shuffle-children.md) | Перемешать порядок дочерних объектов | `lua` |

### Сцены

| | Сниппет | Описание | Теги |
|--|---------|----------|------|
| ![](gifs/scene-switch.gif) | [Переключение сцен](snippets/scene-switch.md) | Загрузить другую сцену по клику | `lua` `scene` |
| ![](gifs/presets.gif) | [Пресеты поведения](snippets/presets.md) | Переиспользовать триггеры через `presets` | `json` `presets` |

---

## Готовые игры

| | Игра | Описание | Теги |
|--|------|----------|------|
| ![](gifs/dice-game.gif) | [Игра в кости](games/dice-game.md) | Два игрока бросают кубики, набирая очки до 21 | `triggers` `scripting` `textures` `state` |
| ![](gifs/nardi.gif) | [Нарды](games/nardi.md) | Длинные нарды: перетаскивание шашек, пресеты, Lua-логика хода | `drag` `presets` `hierarchy` `keyboard` |
