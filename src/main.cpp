#include <algorithm>
#include <fstream>
#include <memory>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>

#include "core/GameObject.hpp"
#include "core/Model.hpp"
#include "core/ResourceManager.hpp"
#include "scene/DefaultFactory.hpp"
#include "scripting/LuaScriptEngine.hpp"
#include "ui/View.hpp"
#include <spdlog/spdlog.h>

using dice::core::GameObject;
using dice::core::Model;
using dice::core::ResourceManager;
using dice::scene::makeDefaultFactory;
using dice::scripting::LuaScriptEngine;
using dice::view::View;
using dice::view::ViewConfig;

int main() {
    spdlog::set_level(spdlog::level::info);
    spdlog::info("DICE engine starting");

    sf::RenderWindow window(sf::VideoMode(1280, 720), "DICE — игра в кости");
    window.setFramerateLimit(60);

    // ======= Fallback texture =======
    auto baseTex = std::make_shared<sf::Texture>();
    {
        sf::Image img;
        img.create(100, 100, sf::Color::White);
        baseTex->loadFromImage(img);
    }

    ResourceManager<sf::Texture> textures;
    textures.setFallback(baseTex);
    std::unordered_set<std::string> loadedIds;

    // ======= View =======
    View view(window);
    ViewConfig vcfg;
    vcfg.backgroundColor = sf::Color(20, 20, 30);
    vcfg.showFPS = false;
    vcfg.showObjectCount = false;
    vcfg.showControls = false;
    view.setConfig(vcfg);

    // ======= Font =======
    sf::Font font;
    const bool fontOk = font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");

    // ======= Model =======
    Model model(makeDefaultFactory());

    // ======= Lua engine =======
    LuaScriptEngine luaEngine;

    std::mt19937 rng(std::random_device{}());
    luaEngine.registerFunction("cpp_rand", [&rng](int lo, int hi) -> int {
        return std::uniform_int_distribution<int>(lo, hi)(rng);
    });

    auto makeText = [&](const std::string& str, float size, int r, int g, int b) {
        sf::Text t;
        t.setFont(font);
        t.setString(sf::String::fromUtf8(str.begin(), str.end()));
        t.setCharacterSize(static_cast<unsigned>(size));
        t.setFillColor(sf::Color(r, g, b));
        t.setOutlineColor(sf::Color::Black);
        t.setOutlineThickness(1.5f);
        return t;
    };

    luaEngine.registerFunction(
        "cpp_draw_text_left",
        [&](const std::string& s, float x, float y, float sz, int r, int g, int b) {
            if (!fontOk)
                return;
            auto t = makeText(s, sz, r, g, b);
            t.setPosition(x, y);
            window.draw(t);
        });

    luaEngine.registerFunction(
        "cpp_draw_text_center",
        [&](const std::string& s, float x, float y, float sz, int r, int g, int b) {
            if (!fontOk)
                return;
            auto t = makeText(s, sz, r, g, b);
            const auto lb = t.getLocalBounds();
            t.setOrigin(lb.left + lb.width / 2.f, lb.top + lb.height / 2.f);
            t.setPosition(x, y);
            window.draw(t);
        });

    luaEngine.registerFunction(
        "cpp_draw_text_right",
        [&](const std::string& s, float x, float y, float sz, int r, int g, int b) {
            if (!fontOk)
                return;
            auto t = makeText(s, sz, r, g, b);
            const auto lb = t.getLocalBounds();
            t.setPosition(x - lb.width, y);
            window.draw(t);
        });

    luaEngine.registerFunction("cpp_draw_rect",
                               [&](float x, float y, float w, float h, int r, int g, int b, int a) {
                                   sf::RectangleShape rect({w, h});
                                   rect.setPosition(x, y);
                                   rect.setFillColor(sf::Color(r, g, b, static_cast<sf::Uint8>(a)));
                                   window.draw(rect);
                               });

    luaEngine.registerFunction("cpp_set_obj_color",
                               [&model](const std::string& id, int r, int g, int b, int a) {
                                   if (auto obj = model.getObject(id))
                                       obj->setColor(sf::Color(r, g, b, static_cast<sf::Uint8>(a)));
                               });

    luaEngine.registerFunction(
        "cpp_set_obj_texture",
        [&model, &textures, &loadedIds](const std::string& objId, const std::string& path) {
            auto obj = model.getObject(objId);
            if (!obj)
                return;
            if (!loadedIds.count(path)) {
                textures.load(path, path);
                loadedIds.insert(path);
            }
            obj->setTexture(textures.get(path).get());
        });

    // ======= Load game script into global Lua state =======
    if (!luaEngine.executeGlobalScript("scripts/game.lua")) {
        spdlog::error("Failed to load scripts/game.lua — run from project root!");
        return 1;
    }

    // ======= Load scene =======
    {
        std::ifstream file("scenes/demo.json");
        if (!file.is_open()) {
            spdlog::error("Cannot open scenes/demo.json — run from project root!");
            return 1;
        }
        model.fromJson(nlohmann::json::parse(file));
        spdlog::info("Scene loaded");
    }

    model.forEachDepthFirst([&](const std::shared_ptr<GameObject>& obj) {
        const std::string& tf = obj->getTextureFile();
        if (!tf.empty()) {
            if (!loadedIds.count(tf)) {
                textures.load(tf, tf);
                loadedIds.insert(tf);
            }
            obj->setTexture(textures.get(tf).get());
        } else {
            obj->setTexture(baseTex.get());
        }
        if (!obj->getLuaScript().empty())
            luaEngine.attachScript(*obj);
    });

    sf::FloatRect fieldBounds{0.f, 0.f, 1280.f, 720.f};
    if (auto board = model.getObject("board"))
        fieldBounds = board->getGlobalBounds();

    // ======= Drag state =======
    std::shared_ptr<GameObject> draggedObj;
    sf::Vector2f dragOffset;
    bool wasDragging = false;
    float chipHalfW = 0.f;
    float chipHalfH = 0.f;

    sf::Clock clock;

    //  Main loop
    while (window.isOpen()) {
        const float dt = clock.restart().asSeconds();

        std::vector<std::shared_ptr<GameObject>> allObjects;
        model.forEachDepthFirst(
            [&](const std::shared_ptr<GameObject>& obj) { allObjects.push_back(obj); });

        // Events — drag-and-drop stays in C++ (SFML-specific)
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed &&
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
                event.mouseButton.button == sf::Mouse::Left) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
                const auto wp = view.screenToWorld({event.mouseButton.x, event.mouseButton.y});
                auto picked = view.pickObject(wp, allObjects);
                if (picked && picked->isDraggable()) {
                    draggedObj = picked;
                    dragOffset = picked->getPosition() - wp;
                    wasDragging = false;
                    const auto b = picked->getGlobalBounds();
                    chipHalfW = b.width / 2.f;
                    chipHalfH = b.height / 2.f;
                } else if (picked) {
                    luaEngine.fireEvent("on_click", picked.get());
                }
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
            if (event.type == sf::Event::MouseMoved && draggedObj) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
                const auto wp = view.screenToWorld({event.mouseMove.x, event.mouseMove.y});
                sf::Vector2f newPos = wp + dragOffset;
                newPos.x = std::clamp(newPos.x,
                                      fieldBounds.left + chipHalfW,
                                      fieldBounds.left + fieldBounds.width - chipHalfW);
                newPos.y = std::clamp(newPos.y,
                                      fieldBounds.top + chipHalfH,
                                      fieldBounds.top + fieldBounds.height - chipHalfH);
                draggedObj->setPosition(newPos.x, newPos.y);
                luaEngine.fireEvent("on_move", draggedObj.get());
                wasDragging = true;
            }

            if (event.type == sf::Event::MouseButtonReleased &&
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
                event.mouseButton.button == sf::Mouse::Left) {
                if (draggedObj && !wasDragging)
                    luaEngine.fireEvent("on_click", draggedObj.get());
                draggedObj = nullptr;
            }

            view.handleEvent(event);
        }

        luaEngine.callGlobal("update", dt);

        view.update(dt);
        view.render(allObjects);

        luaEngine.callGlobal("draw");

        window.display();
    }

    spdlog::info("Application finished");
    return 0;
}