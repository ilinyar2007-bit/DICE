#include "controller/Controller.hpp"

#include <algorithm>
#include <fstream>
#include <random>

#include <nlohmann/json.hpp>

#include "scripting/LuaScript.hpp"
#include <spdlog/spdlog.h>

namespace dice::controller {

Controller::Controller(dice::core::Model& model,
                       dice::view::View& view,
                       dice::scripting::LuaScriptEngine& lua,
                       sf::RenderWindow& window)
    : model_(model), view_(view), lua_(lua), window_(window),
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
    model_.fromJson(nlohmann::json::parse(file));
    refreshFieldBounds();
    spdlog::info("Controller: scene '{}' loaded", path.string());
    return true;
}

void Controller::loadTextures(dice::core::ResourceManager<sf::Texture>& textures) {
    model_.forEachDepthFirst([&](const std::shared_ptr<dice::core::GameObject>& obj) {
        const std::string& tf = obj->getTextureFile();
        if (!tf.empty()) {
            if (!loadedTextureIds_.contains(tf)) {
                textures.load(tf, tf);
                loadedTextureIds_.insert(tf);
                spdlog::info("Controller: texture loaded '{}'", tf);
            }
            obj->setTexture(textures.get(tf).get());
        }
        if (!obj->getLuaScript().empty()) {
            lua_.attachScript(*obj);
        }
    });
}

void Controller::registerDefaultFunctions(dice::core::ResourceManager<sf::Texture>& textures,
                                          const sf::Font* font) {
    lua_.registerFunction("cpp_rand", [](int lo, int hi) -> int {
        static std::mt19937 rng(std::random_device{}());
        return std::uniform_int_distribution<int>(lo, hi)(rng);
    });

    auto makeText = [font](const std::string& str, float size, int r, int g, int b) {
        sf::Text t;
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
        [this, font, makeText](
            const std::string& s, float x, float y, float sz, int r, int g, int b) {
            if (font == nullptr) {
                return;
            }
            auto t = makeText(s, sz, r, g, b);
            t.setPosition(x, y);
            window_.draw(t);
        });

    lua_.registerFunction(
        "cpp_draw_text_center",
        [this, font, makeText](
            const std::string& s, float x, float y, float sz, int r, int g, int b) {
            if (font == nullptr) {
                return;
            }
            auto t = makeText(s, sz, r, g, b);
            const auto lb = t.getLocalBounds();
            t.setOrigin(lb.left + lb.width / 2.F, lb.top + lb.height / 2.F);
            t.setPosition(x, y);
            window_.draw(t);
        });

    lua_.registerFunction(
        "cpp_draw_text_right",
        [this, font, makeText](
            const std::string& s, float x, float y, float sz, int r, int g, int b) {
            if (font == nullptr) {
                return;
            }
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
                          [this, &textures](const std::string& obj_id, const std::string& path) {
                              auto obj = model_.getObject(obj_id);
                              if (!obj) {
                                  return;
                              }
                              if (!loadedTextureIds_.contains(path)) {
                                  textures.load(path, path);
                                  loadedTextureIds_.insert(path);
                              }
                              obj->setTexture(textures.get(path).get());
                          });
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

    view_.handleEvent(event);
}

void Controller::update(float dt) {
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

    if (picked && picked->isDraggable()) {
        draggedObj_ = picked;
        dragOffset_ = picked->getPosition() - wp;
        wasDragging_ = false;
        const auto b = picked->getGlobalBounds();
        chipHalfW_ = b.width / 2.F;
        chipHalfH_ = b.height / 2.F;
        spdlog::debug("Controller: drag start '{}'", picked->getId());
        lua_.fireEvent(dice::scripting::kEventOnDragStart, draggedObj_.get());
    } else if (picked) {
        spdlog::debug("Controller: click '{}'", picked->getId());
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

    // Hover detection (работает независимо от drag)
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
        if (!wasDragging_) {
            spdlog::debug("Controller: click (on release) '{}'", draggedObj_->getId());
            lua_.fireEvent(dice::scripting::kEventOnClick, draggedObj_.get());
        } else {
            spdlog::debug("Controller: drag end '{}'", draggedObj_->getId());
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
