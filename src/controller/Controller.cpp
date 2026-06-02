#include "controller/Controller.hpp"

#include <algorithm>
#include <fstream>
#include <random>
#include <string_view>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "scripting/LuaScript.hpp"
#include <spdlog/spdlog.h>

namespace {

std::mt19937& getRng() {
    static std::mt19937 rng{std::random_device{}()};
    return rng;
}

std::string keyToString(sf::Keyboard::Key key) {
    if (key >= sf::Keyboard::A && key <= sf::Keyboard::Z) {
        return std::string{static_cast<char>(key - sf::Keyboard::A + 'A')};
    }
    static const std::unordered_map<sf::Keyboard::Key, std::string_view> kKeyNames = {
        {sf::Keyboard::Space, "Space"},
        {sf::Keyboard::Enter, "Enter"},
        {sf::Keyboard::Tab, "Tab"},
        {sf::Keyboard::Up, "Up"},
        {sf::Keyboard::Down, "Down"},
        {sf::Keyboard::Left, "Left"},
        {sf::Keyboard::Right, "Right"},
        {sf::Keyboard::Num1, "1"},
        {sf::Keyboard::Num2, "2"},
        {sf::Keyboard::Num3, "3"},
        {sf::Keyboard::Num4, "4"},
        {sf::Keyboard::Num5, "5"},
    };
    const auto it = kKeyNames.find(key);
    return it != kKeyNames.end() ? std::string{it->second} : std::string{};
}

void mergePresetsIntoObject(
    dice::core::GameObject& obj,
    const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& catalog) {
    if (obj.getPresets().empty()) {
        return;
    }
    std::unordered_map<std::string, std::string> finalBindings;
    for (const auto& preset_name : obj.getPresets()) {
        auto it = catalog.find(preset_name);
        if (it == catalog.end()) {
            spdlog::warn(
                "Controller: unknown preset '{}' on object '{}'", preset_name, obj.getId());
            continue;
        }
        for (const auto& [event, ref] : it->second) {
            finalBindings[event] = ref;
        }
    }
    for (const auto& [event, ref] : obj.getTriggerBindings()) {
        finalBindings[event] = ref;
    }
    obj.setTriggerBindings(std::move(finalBindings));
}

} // namespace

namespace dice::controller {

Controller::Controller(dice::core::Model& model,
                       dice::view::View& view,
                       dice::scripting::LuaScriptEngine& lua,
                       sf::RenderWindow& window,
                       dice::core::ResourceManager<sf::Texture>& textures)
    : model_(model), view_(view), lua_(lua), window_(window), textures_(textures),
      fieldBounds_(0.F,
                   0.F,
                   static_cast<float>(window.getSize().x),
                   static_cast<float>(window.getSize().y)) {}

bool Controller::loadScene(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        spdlog::error("Controller: cannot open scene '{}'", path.string());
        return false;
    }

    nlohmann::json sceneJson;
    try {
        sceneJson = nlohmann::json::parse(file);
    } catch (const nlohmann::json::parse_error& e) {
        spdlog::error("Controller: failed to parse scene '{}': {}", path.string(), e.what());
        return false;
    }

    draggedObj_.reset();
    hoveredObj_.reset();
    wasDragging_ = false;

    lua_.clearSceneState();

    if (sceneJson.contains("scripts") && sceneJson["scripts"].is_array()) {
        for (const auto& entry : sceneJson["scripts"]) {
            if (entry.is_string()) {
                lua_.executeGlobalScript(entry.get<std::string>());
            }
        }
    }

    model_.clear();
    model_.fromJson(sceneJson);

    // Merge behavior presets into objects
    const auto& catalog = lua_.getGlobalPresetCatalog();
    model_.forEachDepthFirst([&](const std::shared_ptr<dice::core::GameObject>& obj) {
        mergePresetsIntoObject(*obj, catalog);
    });

    loadedTextureIds_.clear();
    loadTexturesForModel();
    refreshFieldBounds();

    currentScenePath_ = path;
    spdlog::info("Controller: scene '{}' loaded", path.string());
    return true;
}

void Controller::loadTexturesForModel() {
    model_.forEachDepthFirst([&](const std::shared_ptr<dice::core::GameObject>& obj) {
        const std::string& tf = obj->getTextureFile();
        if (!tf.empty()) {
            if (!loadedTextureIds_.contains(tf)) {
                textures_.load(tf, tf);
                loadedTextureIds_.insert(tf);
            }
            obj->setTexture(textures_.get(tf).get());
        }
        if (!obj->getLuaScript().empty()) {
            lua_.attachScript(*obj);
        }
    });
}

void Controller::registerDefaultFunctions(const sf::Font* font) {
    lua_.registerFunction("cpp_rand", [](int lo, int hi) -> int {
        return std::uniform_int_distribution<int>(lo, hi)(getRng());
    });

    lua_.registerFunction("cpp_shuffle_children", [this](const std::string& id) {
        if (auto obj = model_.getObject(id)) {
            obj->shuffleChildren();
        }
    });

    lua_.registerFunction("cpp_shuffle", [](sol::table t) {
        if (!t.valid()) {
            return;
        }
        std::vector<sol::object> items;
        for (size_t i = 1;; ++i) {
            const sol::object obj = t[i];
            if (!obj.valid()) {
                break;
            }
            items.push_back(obj);
        }
        if (items.empty()) {
            return;
        }

        std::shuffle(items.begin(), items.end(), getRng());

        for (size_t i = 0; i < items.size(); ++i) {
            t[i + 1] = items[i];
        }
    });

    auto makeText = [font](const std::string& str, float size, int r, int g, int b) {
        sf::Text t;
        if (font == nullptr) {
            return t;
        }
        t.setFont(*font);
        t.setString(sf::String::fromUtf8(str.begin(), str.end()));
        t.setCharacterSize(static_cast<unsigned>(size));
        t.setFillColor(sf::Color(r, g, b));
        t.setOutlineColor(sf::Color::Black);
        t.setOutlineThickness(1.5F);
        return t;
    };

    lua_.registerFunction(
        "cpp_draw_text_left",
        [this, makeText](const std::string& s, float x, float y, float sz, int r, int g, int b) {
            auto t = makeText(s, sz, r, g, b);
            t.setPosition(x, y);
            window_.draw(t);
        });

    lua_.registerFunction(
        "cpp_draw_text_center",
        [this, makeText](const std::string& s, float x, float y, float sz, int r, int g, int b) {
            auto t = makeText(s, sz, r, g, b);
            const auto lb = t.getLocalBounds();
            t.setOrigin(lb.left + lb.width / 2.F, lb.top + lb.height / 2.F);
            t.setPosition(x, y);
            window_.draw(t);
        });

    lua_.registerFunction(
        "cpp_draw_text_right",
        [this, makeText](const std::string& s, float x, float y, float sz, int r, int g, int b) {
            auto t = makeText(s, sz, r, g, b);
            const auto lb = t.getLocalBounds();
            t.setPosition(x - lb.width, y);
            window_.draw(t);
        });

    lua_.registerFunction("cpp_draw_rect",
                          [this](float x, float y, float w, float h, int r, int g, int b, int a) {
                              sf::RectangleShape rect({w, h});
                              rect.setPosition(x, y);
                              rect.setFillColor(sf::Color(r, g, b, static_cast<sf::Uint8>(a)));
                              window_.draw(rect);
                          });

    lua_.registerFunction("cpp_set_obj_color",
                          [this](const std::string& id, int r, int g, int b, int a) {
                              if (auto obj = model_.getObject(id)) {
                                  obj->setColor(sf::Color(r, g, b, static_cast<sf::Uint8>(a)));
                              }
                          });

    lua_.registerFunction("cpp_set_obj_texture",
                          [this](const std::string& obj_id, const std::string& path) {
                              auto obj = model_.getObject(obj_id);
                              if (!obj) {
                                  return;
                              }
                              if (!loadedTextureIds_.contains(path)) {
                                  textures_.load(path, path);
                                  loadedTextureIds_.insert(path);
                              }
                              obj->setTexture(textures_.get(path).get());
                          });

    lua_.setSceneLoadCallback([this](const std::string& path) { pendingScenePath_ = path; });
    lua_.registerModelAccess(model_, [this]() { return currentScenePath_.string(); });
}

void Controller::handleEvent(const sf::Event& event) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left) // NOLINT
        onMousePressed(event.mouseButton);           // NOLINT

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    if (event.type == sf::Event::MouseMoved) {
        onMouseMoved(event.mouseMove); // NOLINT
    }

    if (event.type == sf::Event::MouseButtonReleased &&
        event.mouseButton.button == sf::Mouse::Left) // NOLINT
        onMouseReleased(event.mouseButton);          // NOLINT

    if (event.type == sf::Event::KeyPressed) {
        const std::string keyName = keyToString(event.key.code); // NOLINT
        if (!keyName.empty()) {
            lua_.fireKeyEvent(keyName);
        }
    }

    view_.handleEvent(event);
}

void Controller::update(float dt) {
    if (!pendingScenePath_.empty()) {
        const std::filesystem::path path = pendingScenePath_;
        pendingScenePath_.clear();
        if (!loadScene(path)) {
            spdlog::error("Controller: failed to load pending scene '{}'", path.string());
        }
        return;
    }
    lua_.callGlobal("update", dt);
    view_.update(dt);
}

std::vector<std::shared_ptr<dice::core::GameObject>> Controller::collectObjects() const {
    std::vector<std::shared_ptr<dice::core::GameObject>> result;
    model_.forEachDepthFirst(
        [&](const std::shared_ptr<dice::core::GameObject>& o) { result.push_back(o); });
    return result;
}

void Controller::onMousePressed(const sf::Event::MouseButtonEvent& ev) {
    const auto wp = view_.screenToWorld({ev.x, ev.y});
    const auto objs = collectObjects();
    auto picked = view_.pickObject(wp, objs);

    if (!picked) {
        spdlog::info("Controller: click at ({},{}) — no object picked", ev.x, ev.y);
    } else {
        spdlog::info("Controller: click at ({},{}) — picked '{}' draggable={} visible={} active={}",
                     ev.x,
                     ev.y,
                     picked->getId(),
                     picked->isDraggable(),
                     picked->isVisible(),
                     picked->isActive());
    }

    if (picked && picked->isDraggable()) {
        draggedObj_ = picked;
        dragOffset_ = picked->getPosition() - wp;
        wasDragging_ = false;
        const auto b = picked->getGlobalBounds();
        chipHalfW_ = b.width / 2.F;
        chipHalfH_ = b.height / 2.F;
        spdlog::info("Controller: drag start '{}'", picked->getId());
        lua_.fireEvent(dice::scripting::kEventOnDragStart, draggedObj_.get());
    } else if (picked) {
        spdlog::info("Controller: click firing event on '{}'", picked->getId());
        lua_.fireEvent(dice::scripting::kEventOnClick, picked.get());
    }
}

void Controller::onMouseMoved(const sf::Event::MouseMoveEvent& ev) {
    const auto wp = view_.screenToWorld({ev.x, ev.y});

    if (draggedObj_) {
        sf::Vector2f newPos = wp + dragOffset_;
        newPos.x = std::clamp(newPos.x,
                              fieldBounds_.left + chipHalfW_,
                              fieldBounds_.left + fieldBounds_.width - chipHalfW_);
        newPos.y = std::clamp(newPos.y,
                              fieldBounds_.top + chipHalfH_,
                              fieldBounds_.top + fieldBounds_.height - chipHalfH_);
        draggedObj_->setPosition(newPos.x, newPos.y);
        lua_.fireEvent(dice::scripting::kEventOnMove, draggedObj_.get());
        wasDragging_ = true;
    }

    const auto objs = collectObjects();
    auto picked = view_.pickObject(wp, objs);

    auto prevHovered = hoveredObj_.lock();
    if (picked != prevHovered) {
        if (prevHovered) {
            lua_.fireEvent(dice::scripting::kEventOnHoverExit, prevHovered.get());
        }
        hoveredObj_ = picked;
        if (picked) {
            lua_.fireEvent(dice::scripting::kEventOnHover, picked.get());
        }
    }
}

void Controller::onMouseReleased(const sf::Event::MouseButtonEvent& /*ev*/) {
    if (draggedObj_) {
        spdlog::info("Controller: mouse released on '{}' wasDragging={}",
                     draggedObj_->getId(),
                     wasDragging_);
        if (!wasDragging_) {
            spdlog::info("Controller: click (on release) '{}'", draggedObj_->getId());
            lua_.fireEvent(dice::scripting::kEventOnClick, draggedObj_.get());
        } else {
            spdlog::info("Controller: drag end '{}'", draggedObj_->getId());
        }
        lua_.fireEvent(dice::scripting::kEventOnDragEnd, draggedObj_.get());
        draggedObj_ = nullptr;
    }
}

void Controller::refreshFieldBounds() {
    if (auto board = model_.getObject("board")) {
        fieldBounds_ = board->getGlobalBounds();
    }
}

} // namespace dice::controller
