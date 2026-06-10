# Архитектура DICE

## 1. Обзор модулей

DICE следует паттерну MVC, расширенному слоями скриптинга и управления ресурсами.

**Иерархия объектов:** классическое C++ наследование — `Card`, `Chip`, `Dice`, `Tile`, `Deck` наследуются от `GameObject`, который наследует `sf::Drawable` и `sf::Transformable`. Дочерние объекты хранятся в `vector<shared_ptr<GameObject>>` (shared ownership); обратная ссылка на родителя — сырой non-owning указатель `GameObject*`. `Model` содержит плоский `unordered_map<string, shared_ptr<GameObject>>` для поиска по id за O(1) и список корней. ECS и непрерывных массивов нет — объекты размещаются в куче через `shared_ptr`.

**Жизненный цикл объектов:** `shared_ptr` одновременно хранится в `children` родителя и в плоской карте `Model::objects_`. Для полного уничтожения объекта используй `Model::removeObject(id)` — он удаляет ссылку из обоих мест. Прямой вызов `parent->removeChild(id)` оставит объект живым в карте модели.

**Состояние:** Игровое состояние хранится полностью в Lua-глобалах (например, таблица `game` в dice_demo). `ActionManager` реализован в `dice::core` и поддерживает `saveSnapshot`/`undo`/`redo` через JSON-снепшоты модели, но не подключён к игровому циклу и недоступен из Lua — откат состояния пока реализуется вручную в скрипте.

| Модуль | Ответственность | Namespace |
|--------|----------------|-----------|
| `Application` | Главный цикл, связывает все подсистемы | `dice` |
| `ConfigLoader` | Читает `game.json` в типизированный конфиг | `dice` |
| `Model` | Дерево объектов сцены (корни + плоский id-map) | `dice::core` |
| `Controller` | Ввод, Lua-мост, переходы между сценами | `dice::controller` |
| `View` | Рендеринг SFML, координатное преобразование, pick объектов | `dice::view` |
| `LuaScriptEngine` | Lua VM, реестр функций, диспетчер событий | `dice::scripting` |
| `LuaScript` | Привязка скрипта к конкретному объекту | `dice::scripting` |
| `SceneValidator` | Валидирует JSON сцены до загрузки | `dice::core` |
| `ResourceManager` | Типизированный кэш ресурсов (текстуры, шрифты) | `dice::core` |
| `GameObject` | Базовый узел: позиция, текстура, дети, триггеры | `dice::core` |
| `Card` / `Chip` | Специализированные подтипы `GameObject` | `dice::components` |
| `Dice` | Кубик: количество граней, текущее значение, текстуры граней, `roll()` | `dice::components` |
| `Tile` | Клетка сетки: координаты col/row, occupant, фильтр принимаемых типов | `dice::components` |
| `Deck` | Колода: `faceDown`, управляет Card-дочерьми | `dice::components` |
| `Action` / `ActionManager` | Снепшоты модели, undo/redo *(реализовано в core, не подключено к приложению)* | `dice::core` |
| `IObjectFactory` / `DefaultObjectFactory` | Фабрика объектов: `Model` создаёт объект нужного подтипа по полю `type` из JSON | `dice::core` / `dice::scene` |
| `NetworkManager` | Сетевой фасад: Lua-биндинги, делегирование к `HostServer` / `GameClient` *(реализовано в network, не подключено к приложению — см. раздел 8)* | `dice::network` |

---

## 2. Сборка и зависимости

- **Система сборки:** CMake
- **Зависимости** загружаются автоматически через `FetchContent` при первом `cmake ..`:
  SFML, sol2, spdlog, nlohmann/json, Dear ImGui, GoogleTest.
  Lua 5.3 устанавливается системным пакетным менеджером (`apt install liblua5.3-dev`).
- Внешние пакетные менеджеры (vcpkg, Conan) не нужны

Инструкции по сборке см. в [README.md](../README.md#сборка).

---

## 3. Game Loop

```mermaid
flowchart TD
    A[init] --> B{running &&\nwindow.isOpen?}
    B -->|да| C[handleEvents]
    C --> D[update dt]
    D --> E[render]
    E --> B
    B -->|нет| F[shutdown]
```

| Фаза | Что происходит |
|------|---------------|
| **init** | Читает `game.json`, создаёт окно, загружает шрифты, настраивает Lua VM, загружает пресеты, загружает стартовую сцену |
| **handleEvents** | Опрашивает SFML-события, маршрутизирует мышь/клавиатуру в `Controller`, UI-события в `View` |
| **update** | Если есть отложенный переход сцены — выполняет его и пропускает остаток фазы; иначе вызывает `lua_global("update", dt)` и обновляет `View` |
| **render** | Очищает буфер, собирает объекты из `Model`, вызывает `view_.render()`, вызывает `lua_global("draw")`, отображает кадр |

> `Esc` или закрытие окна завершают приложение — это обрабатывается в `Application::handleEvents` до передачи события `Controller` и не перехватывается из Lua.

---

## 4. Флоу загрузки сцены

```mermaid
sequenceDiagram
    participant C as Controller
    participant V as SceneValidator
    participant L as LuaScriptEngine
    participant M as Model
    participant R as ResourceManager

    C->>V: validate(json)
    V-->>C: ok / ошибка → возврат false
    C->>L: clearSceneState()
    loop каждый скрипт из "scripts"
        C->>L: executeGlobalScript(path)
    end
    C->>M: clear()
    C->>M: fromJson(json)
    C->>L: getGlobalPresetCatalog()
    C->>M: forEachDepthFirst → mergePresetsIntoObject()
    loop каждый объект (один проход forEachDepthFirst)
        C->>R: load(textureFile), если есть
        C->>L: attachScript(obj), если есть luaScript
    end
```

**Ключевые детали:**
- Скрипты из секции `"scripts"` загружаются **до** `Model::fromJson` — они регистрируют триггеры до создания объектов. Если любой из скриптов завершается с ошибкой, загрузка сцены прерывается.
- Пресеты из `assets/presets.json` мержатся в `triggerBindings` объекта после загрузки модели — явные `triggers` в JSON имеют приоритет над пресетами.
- Если `SceneValidator` возвращает ошибку, загрузка прерывается; состояние модели не меняется.
- `clearSceneState()` сбрасывает **все** подписки текущей сцены: `scriptRegistry_`, `inlineCallbacks_`, `triggerCatalog_`, `keyHandlers_` и `moduleCache_`. Обработчики `engine.onKey`, зарегистрированные в предыдущей сцене, **не сохраняются** при переходе на новую.
- Если в JSON объекта указан несуществующий пресет, движок выводит `spdlog::warn` и пропускает его — загрузка сцены не прерывается.
- После загрузки текстур выполняется пост-проход по дереву: `Deck.faceDown` распространяется на дочерние карты, у `Dice` предзагружаются все `faceTextures` и применяется текстура текущего значения, затем пересчитываются границы поля для перетаскивания (`refreshFieldBounds`).

---

## 5. Обработка пользовательского действия: клик → Lua

```mermaid
sequenceDiagram
    participant SFML
    participant App as Application
    participant C as Controller
    participant V as View
    participant L as LuaScriptEngine

    SFML->>App: MouseButtonPressed event
    App->>C: handleEvent(event)
    C->>V: screenToWorld(mouse pos)
    C->>C: collectObjects()
    C->>V: pickObject(worldPos, objects)
    V-->>C: picked GameObject (или nullptr)
    alt объект перетаскиваемый (draggable)
        C->>L: fireEvent("on_drag_start", obj)
    else объект кликабельный
        C->>L: fireEvent("on_click", obj)
        L->>L: ищет trigger binding объекта
        L->>L: вызывает Lua-функцию
    end
```

**Важно:** Для **не-draggable** объектов `on_click` срабатывает при **нажатии** (`MouseButtonPressed`). Для **draggable** объектов `on_click` срабатывает при **отпускании** (`MouseButtonReleased`), только если не было перетаскивания (`wasDragging_ == false`).

**Механика перетаскивания:** drag активируется только после смещения курсора на 5 px от точки нажатия (`kDragThreshold`) — до этого объект не двигается и `on_move` не срабатывает, а отпускание считается кликом. Позиция перетаскиваемого объекта ограничена границами поля: если в сцене есть объект с id `board`, полем считаются его bounds, иначе — всё окно (`Controller::refreshFieldBounds`, пересчитывается при загрузке сцены).

---

## 6. Lua API Reference

### C++ функции, доступные из Lua

| Функция | Сигнатура | Сложность | Описание |
|---------|-----------|-----------|----------|
| `cpp_rand` | `(lo: int, hi: int) → int` | O(1) | Случайное целое в \[lo, hi\] |
| `cpp_shuffle_children` | `(id: string)` | O(N) прямых детей | Перемешать детей объекта по id |
| `cpp_shuffle` | `(t: table)` | O(N) | Перемешать Lua-таблицу in-place |
| `cpp_draw_text_left` | `(s, x, y, size, r, g, b)` | O(1) | Текст с выравниванием по левому краю |
| `cpp_draw_text_center` | `(s, x, y, size, r, g, b)` | O(1) | Текст по центру |
| `cpp_draw_text_right` | `(s, x, y, size, r, g, b)` | O(1) | Текст с выравниванием по правому краю |
| `cpp_draw_rect` | `(x, y, w, h, r, g, b, a)` | O(1) | Закрашенный прямоугольник |
| `cpp_set_obj_color` | `(id, r, g, b, a)` | O(1) avg | Задать цвет объекту по id |
| `cpp_set_obj_texture` | `(id, path)` | O(1) avg | Сменить текстуру объекту по id |
| `cpp_dice_roll` | `(id: string) → int` | O(1) avg | Бросить кубик: случайное значение в [1, faceCount], применить текстуру грани, вернуть значение |
| `cpp_deck_draw` | `(id: string) → string` | O(N) карт в колоде | Снять верхнюю карту с колоды, поднять её до корня сцены, вернуть её id (пусто — если колода пуста) |
| `cpp_deck_count` | `(id: string) → int` | O(1) avg | Количество карт в колоде |
| `cpp_log` | `(msg: string)` | O(1) | Лог через spdlog; поддерживает UI-коллбэк. Используется в примерах |
| `log` | `(msg: string)` | O(1) | Лог через spdlog (только вывод, без коллбэка) |

> `cpp_draw_*` вызываются из `draw()` — они рисуют прямо в текущий кадр и не сохраняют результат между кадрами.

### engine.* функции

| Функция | Сложность | Описание |
|---------|-----------|----------|
| `engine.getObject(id)` | O(1) avg | Поиск по `unordered_map` — безопасно вызывать в `update()` |
| `engine.trigger(name, fn)` | O(1) | Зарегистрировать именованный триггер |
| `engine.on(id, event, fn)` | O(1) | Зарегистрировать inline-обработчик события `event` для объекта с id; `fn(self)` — альтернатива именованным триггерам |
| `engine.onKey(key, fn)` | O(1) | Зарегистрировать обработчик клавиши; повторная регистрация той же клавиши заменяет старый обработчик (с `warn` в логе) |
| `engine.intersects(id1, id2)` | O(1) avg | Проверить пересечение AABB двух объектов по id; возвращает `false`, если объект не найден |
| `engine.reloadScene()` | — | Перезагрузить текущую сцену (отложено до следующего кадра) |
| `engine.loadScene(path)` | — | Загрузить другую сцену (отложено до следующего кадра) |

> `engine.getObject` использует `unordered_map` и безопасен в `update()`. `collectObjects()` — отдельная внутренняя функция `Controller`, делающая обход всего дерева сцены — не вызывается из Lua напрямую.

**Поддерживаемые имена клавиш** для `engine.onKey`: `"A"`–`"Z"`, `"Space"`, `"Enter"`, `"Tab"`, `"Up"`, `"Down"`, `"Left"`, `"Right"`, `"1"`–`"5"`. Другие клавиши не распознаются; `Esc` зарезервирован — закрывает приложение.

**Доступные Lua-библиотеки:** `base`, `math`, `string`, `table`. Библиотеки `io`, `os`, `coroutine` и `package` не подключены.

### Методы GameObject (доступны в обработчиках событий через `self`)

| Метод | Описание |
|-------|----------|
| `self:getId()` | Строковый id объекта |
| `self:getName()` / `self:setName(s)` | Имя объекта |
| `self:getType()` | Строка типа (`"GameObject"`, `"Card"`, `"Chip"`, `"Dice"`, `"Tile"`, `"Deck"`) |
| `self:getX()` / `self:getY()` | Позиция объекта |
| `self:setPosition(x, y)` | Задать позицию |
| `self:getZOrder()` / `self:setZOrder(z)` | Порядок слоёв |
| `self:getRotation()` / `self:setRotation(deg)` | Вращение в градусах |
| `self:getScaleX()` / `self:getScaleY()` / `self:setScale(x, y)` | Масштаб |
| `self:isVisible()` / `self:setVisible(bool)` | Видимость |
| `self:isActive()` / `self:setActive(bool)` | Активность (включая дочерние события) |
| `self:isDraggable()` / `self:setDraggable(bool)` | Перетаскиваемость |
| `self:setColor(r, g, b, a)` | Цвет (RGBA, 0–255) |
| `self:getIntProperty(key, default)` | Читать int-свойство из `properties` |
| `self:getFloatProperty(key, default)` | Читать float-свойство |
| `self:getStringProperty(key, default)` | Читать string-свойство |
| `self:getBoolProperty(key, default)` | Читать bool-свойство |
| `self:setIntProperty(key, val)` | Задать int-свойство |
| `self:setStringProperty(key, val)` | Задать string-свойство |
| `self:hasTag(tag)` / `self:getTags()` | Проверить/получить теги объекта |

`engine.getObject(id)` возвращает конкретный тип — если объект является `Card`, `Chip` и т.д., Lua получает именно этот тип и может вызывать его методы.

### Методы подтипов (доступны через `self` или объект из `engine.getObject`)

**Card:** `flip()`, `isFaceUp()`, `setFaceUp(bool)`, `setPlayer(int)`, `getPlayer()`

**Chip:** `getRadius()`, `setRadius(float)`, `getAssetId()`, `setAssetId(string)`, `setPlayer(int)`, `getPlayer()`

**Dice:** `getFaceCount()`, `getValue()` — для броска используй `cpp_dice_roll(id)`

**Tile:** `getCol()`, `getRow()`, `getOccupantId()`, `setOccupant(string)`, `clearOccupant()`, `isOccupied()`, `accepts(string)`

**Deck:** `isFaceDown()`, `count()`, `isEmpty()` — для операций с картами используй `cpp_deck_draw(id)`, `cpp_deck_count(id)`, `cpp_shuffle_children(id)`

### События (имена триггеров)

| Событие | Когда срабатывает |
|---------|-----------------|
| `on_click` | Левая кнопка мыши: для не-draggable — при нажатии; для draggable — при отпускании без перетаскивания |
| `on_hover` | Курсор вошёл в bounds объекта |
| `on_hover_exit` | Курсор вышел из bounds объекта |
| `on_drag_start` | Нажатие ЛКМ на draggable-объект (срабатывает сразу при нажатии, до любого движения) |
| `on_drag_end` | Отпускание ЛКМ с draggable-объекта — срабатывает всегда, в том числе при обычном клике без перемещения |
| `on_move` | Позиция объекта изменилась в процессе drag |

---

## 7. `game.json` Reference

### Поведение при превышении `luaMemoryLimitMb`

Реализовано через кастомный аллокатор `guardedAlloc`. При превышении лимита аллокатор возвращает `nullptr` → Lua поднимает memory error → `sol::protected_function` в `callGlobal` перехватывает его → ошибка логируется через `spdlog::error`. **Движок не падает, сцена не выгружается** — только конкретный вызов скрипта завершается с ошибкой в лог.

| Поле | Тип | По умолчанию | Описание |
|------|-----|-------------|----------|
| `title` | string | `"DICE"` | Заголовок окна |
| `windowWidth` | int | `1280` | Ширина окна в пикселях |
| `windowHeight` | int | `720` | Высота окна в пикселях |
| `framerateLimit` | int | `60` | Лимит FPS |
| `resizable` | bool | `true` | Разрешить изменение размера окна |
| `clearR` / `clearG` / `clearB` | int | `30/30/40` | Цвет фона (0–255) |
| `startScene` | string | `"scenes/demo.json"` | Путь к начальной сцене |
| `globalScript` | string | `""` | Lua-скрипт, выполняемый один раз при старте приложения (после установки лимита памяти Lua) |
| `fonts` | array | `[]` | `[{id, path}]` — список шрифтов |
| `showFPS` | bool | `true` | Оверлей счётчика FPS |
| `showObjectCount` | bool | `true` | Оверлей числа объектов |
| `showControls` | bool | `true` | Оверлей подсказок по управлению |
| `luaMemoryLimitMb` | int | `64` | Лимит памяти Lua VM в МБ; значение ≤ 0 сбрасывается в 64 с `warn` в логе |
| `maxSceneObjects` | int | `1000` | Максимум объектов в сцене; значение ≤ 0 сбрасывается в 1000 с `warn` в логе |

> **Ограничения `globalScript`:** скрипт выполняется до `registerDefaultFunctions` — в этот момент доступны только `engine.trigger` / `engine.on` / `engine.onKey` и `cpp_log`; функции `cpp_rand`, `cpp_draw_*`, `engine.getObject`, `engine.loadScene` и `log` ещё не зарегистрированы. Кроме того, загрузка первой же сцены вызывает `clearSceneState()` и стирает все триггеры и обработчики клавиш, зарегистрированные в `globalScript`. Практически он пригоден только для объявления глобальных переменных и функций.

> **Координаты и resizable-окно:** `View` использует `sf::View` с отображением 1:1 (физические пиксели = логические координаты). При `resizable: true` и изменении размера окна `sf::View` перестраивается под новые размеры — координаты в JSON и в вызовах `cpp_draw_*` / `cpp_draw_text_*` жёстко привязаны к пикселям и **не масштабируются автоматически**. Если сцена рассчитана на 1280×720, при другом размере окна абсолютные позиции поплывут. Рекомендуется либо не использовать `resizable: true`, либо вычислять позиции относительно размера окна в скрипте.

---

## 8. Сетевая часть *(библиотека, не подключена к приложению)*

> ⚠️ Библиотека `dice_network` собирается и линкуется с бинарём, но `Application` её **не создаёт**: ни `NetworkManager`, ни требуемый ему `ActionManager` не инстанцируются в приложении. Сетевые Lua-функции регистрируются только конструктором `NetworkManager`, поэтому в текущем рантайме они **недоступны** — вызов `is_host()` из игрового скрипта завершится ошибкой обращения к nil. Этот раздел описывает возможности библиотеки; они заработают после её интеграции в `Application`. Сама подсистема в демонстрационном состоянии: возможны зависания соединения, рассинхронизация состояния и неожиданные отключения.

Сетевая библиотека реализует многопользовательский режим по TCP. Основные классы находятся в `dice::network`.

### Роли

| Роль | Описание |
|------|---------|
| `SinglePlayer` | Офлайн-режим (по умолчанию) |
| `Host` | Запускает TCP-сервер, управляет клиентами и рассылает снепшоты модели |
| `Client` | Подключается к хосту, получает и применяет обновления состояния |

### Архитектура

```
NetworkManager          — фасад, Lua-биндинги, маршрутизация между Host/Client
├── HostServer          — TCP-сервер (отдельный поток): accept, receive, broadcast
└── GameClient          — TCP-клиент (отдельный поток): подключение, receive
```

**HostServer:** слушает входящие соединения в `serverLoop()` (отдельный `std::thread`). Рассылает снепшот модели всем клиентам каждые **100 мс**. Пингует клиентов каждые **5 с**; клиент отключается при отсутствии ответа более **15 с**.

**GameClient:** получает сообщения в `receiveLoop()` (отдельный `std::thread`). При получении `Snapshot` — применяет состояние к `Model`. При получении `Event` / `MoveObject` — воспроизводит действие локально.

**Протокол:** TCP, фреймирование — 4-байтный big-endian prefixed length + JSON-тело.

### Типы сообщений

| Тип | Направление | Описание |
|-----|-------------|----------|
| `Handshake` / `HandshakeAck` | Client→Host / Host→Client | Установка соединения, передача `clientId` |
| `Ping` / `Pong` | Host↔Client | Keepalive |
| `Disconnect` | любой | Корректное отключение с причиной |
| `PlayerJoined` / `PlayerLeft` | Host→всем | Лобби: игрок вошёл/вышел |
| `PlayerReady` | Client→Host, Host→всем | Игрок готов |
| `StartGame` | Host→всем | Начало игры |
| `Snapshot` | Host→всем | Полный снепшот `Model` (JSON), рассылается каждые 100 мс |
| `Event` | Client→Host→всем | Игровое событие по id объекта |
| `MoveObject` | Client→Host→всем | Перемещение объекта |
| `Chat` | любой→всем | Текстовый чат |

Клиентам разрешено отправлять только события `on_click`, `on_drag_start`, `on_drag_end` — остальные отбрасываются хостом.

### Lua API (сетевые функции)

Регистрируются в конструкторе `NetworkManager` (`registerLuaBindings()`) и становятся глобально доступны из Lua **только когда `NetworkManager` создан** — в текущем приложении это не происходит (см. предупреждение выше).

| Функция | Сигнатура | Описание |
|---------|-----------|----------|
| `is_host` | `() → bool` | `true`, если текущий игрок — хост |
| `is_client` | `() → bool` | `true`, если текущий игрок — клиент (подключён, но не хост) |
| `send_event` | `(id: string, event: string)` | Отправить игровое событие всем участникам; на хосте также выполняется локально |
| `send_move` | `(id: string, x: float, y: float)` | Переместить объект и синхронизировать позицию со всеми участниками |

**Пример использования в Lua:**

```lua
engine.trigger("move_piece", function(self)
    if is_host() then
        send_move(self:getId(), 400, 300)
    elseif is_client() then
        send_move(self:getId(), 400, 300)  -- отправляет запрос хосту
    end
end)
```

### Таблица модулей

| Класс | Namespace | Ответственность |
|-------|-----------|----------------|
| `NetworkManager` | `dice::network` | Фасад: Lua-биндинги, делегирование к `HostServer` / `GameClient` |
| `HostServer` | `dice::network` | TCP-сервер, lobby, broadcast снепшотов, таймауты |
| `GameClient` | `dice::network` | TCP-клиент, применение снепшотов и событий к локальной `Model` |
| `NetworkMessage` | `dice::network` | Типы сообщений, сериализация/десериализация (JSON + 4-byte length prefix) |
| `MessageBuffer` | `dice::network` | Потоковый буфер для сборки сообщений из TCP-фрагментов |
| `ClientInfo` | `dice::network` | Данные о подключённом игроке: id, имя, ip, порт, статус |
| `SocketUtils` | `dice::network` | `sendAll` — гарантированная отправка всего буфера в TCP-сокет (досылает остаток при partial send) |

---

## 9. ActionValidator *(в разработке)*

> В разработке — документация будет добавлена позже.
