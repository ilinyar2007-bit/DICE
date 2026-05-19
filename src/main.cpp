#include <memory>

#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>

#include "controller/Controller.hpp"
#include "core/Model.hpp"
#include "core/ResourceManager.hpp"
#include "scene/DefaultFactory.hpp"
#include "scripting/LuaScriptEngine.hpp"
#include "ui/View.hpp"
#include <spdlog/spdlog.h>

using dice::controller::Controller;
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

    auto baseTex = std::make_shared<sf::Texture>();
    {
        sf::Image img;
        img.create(100, 100, sf::Color::White);
        baseTex->loadFromImage(img);
    }

    ResourceManager<sf::Texture> textures;
    textures.setFallback(baseTex);

    View view(window);
    ViewConfig vcfg;
    vcfg.backgroundColor = sf::Color(20, 20, 30);
    vcfg.showFPS = false;
    vcfg.showObjectCount = false;
    vcfg.showControls = false;
    view.setConfig(vcfg);

    sf::Font font;
    const bool fontOk = font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
    if (fontOk) {
        spdlog::info("Font loaded");
    } else {
        spdlog::warn("Font not found — text will use fallback");
    }

    Model model(makeDefaultFactory());
    LuaScriptEngine luaEngine;

    Controller controller(model, view, luaEngine, window);
    controller.registerDefaultFunctions(textures, fontOk ? &font : nullptr);

    if (!luaEngine.executeGlobalScript("scripts/game.lua")) {
        spdlog::error("Failed to load scripts/game.lua — run from project root!");
        return 1;
    }
    if (!controller.loadScene("scenes/demo.json")) {
        spdlog::error("Failed to load scenes/demo.json");
        return 1;
    }
    controller.loadTextures(textures);
    spdlog::info("Textures loaded");

    sf::Clock clock;
    while (window.isOpen()) {
        const float dt = clock.restart().asSeconds();

        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                window.close();
            }
            controller.handleEvent(event);
        }

        controller.update(dt);

        const auto objs = controller.collectObjects();
        view.render(objs);
        luaEngine.callGlobal("draw");
        window.display();
    }

    spdlog::info("Application finished");
    return 0;
}
