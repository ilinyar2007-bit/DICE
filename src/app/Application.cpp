#include "app/Application.hpp"

#include <filesystem>

#include "app/ConfigLoader.hpp"
#include "scene/DefaultFactory.hpp"
#include <spdlog/spdlog.h>

namespace dice {

Application::Application() : view_(window_), controller_(model_, view_, lua_, window_, textures_) {
    model_.setFactory(dice::scene::makeDefaultFactory());
}

Application::~Application() {
    shutdown();
}

void Application::run(const std::string& start_scene) {
    if (!init(start_scene)) {
        spdlog::critical("Failed to initialize application");
        return;
    }

    running_ = true;
    spdlog::info("=== DICE Application Started ===");

    while (running_ && window_.isOpen()) {
        float dt = clock_.restart().asSeconds();
        if (dt > 0.05F) {
            dt = 0.05F; // Cap dt for stability
        }

        handleEvents();
        update(dt);
        render();
    }

    spdlog::info("=== DICE Application Stopped ===");
}

bool Application::init(const std::string& start_scene) {
    config_ = loadConfig("game.json");
    if (!start_scene.empty()) {
        config_.startScene = start_scene;
    }

    // Window Setup
    const sf::Uint32 style =
        sf::Style::Titlebar | sf::Style::Close | (config_.resizable ? sf::Style::Resize : 0);
    window_.create(sf::VideoMode(config_.windowWidth, config_.windowHeight), config_.title, style);
    window_.setFramerateLimit(config_.framerateLimit);

    if (!window_.isOpen()) {
        spdlog::error("Failed to create window");
        return false;
    }

    // Resources
    auto fallbackTex = std::make_shared<sf::Texture>();
    sf::Image img;
    img.create(32, 32, sf::Color::Magenta);
    fallbackTex->loadFromImage(img);
    textures_.setFallback(fallbackTex);

    for (const auto& f : config_.fonts) {
        if (std::filesystem::exists(f.path)) {
            fonts_.load(f.id, f.path);
        } else {
            spdlog::warn("Font file not found: {}", f.path);
        }
    }

    if (fonts_.isEmpty()) {
        spdlog::warn("No fonts loaded. UI text may not be rendered correctly.");
    }

    // View Config
    view_.setFontManager(&fonts_);
    const view::ViewConfig vcfg{
        .backgroundColor = sf::Color(config_.clearR, config_.clearG, config_.clearB),
        .showFPS = config_.showFPS,
        .showObjectCount = config_.showObjectCount,
        .showControls = config_.showControls,
        .fontAssetId = config_.fonts.empty() ? std::string{} : config_.fonts[0].id,
    };
    view_.setConfig(vcfg);

    // Lua Setup
    lua_.setMemoryLimit(static_cast<size_t>(config_.luaMemoryLimitMb) * 1024ULL * 1024ULL);
    if (!config_.globalScript.empty()) {
        if (!lua_.executeGlobalScript(config_.globalScript)) {
            spdlog::error("Application: failed to execute globalScript: {}", config_.globalScript);
        }
    }
    lua_.registerFunction("log", [](const std::string& msg) { spdlog::info("[Lua] {}", msg); });

    // Controller Setup
    sf::Font* mainFont = nullptr;
    if (!config_.fonts.empty()) {
        mainFont = fonts_.get(config_.fonts[0].id).get();
    } else {
        spdlog::warn("no fonts are available");
    }
    lua_.loadPresets("assets/presets.json");
    controller_.setMaxSceneObjects(config_.maxSceneObjects);
    controller_.registerDefaultFunctions(mainFont);

    if (!controller_.loadScene(config_.startScene)) {
        spdlog::error("Failed to load start scene: {}", config_.startScene);
        return false;
    }

    return true;
}

void Application::handleEvents() {
    sf::Event event{};
    while (window_.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            running_ = false;
        }
        // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
            running_ = false;
        }
        // NOLINTEND(cppcoreguidelines-pro-type-union-access)
        controller_.handleEvent(event);
    }
}

void Application::update(float dt) {
    controller_.update(dt);
}

void Application::render() {
    auto objects = controller_.collectObjects();
    view_.render(objects);

    lua_.callGlobalIfExists("draw");

    window_.display();
}

void Application::shutdown() {
    if (window_.isOpen()) {
        window_.close();
    }
}

} // namespace dice
