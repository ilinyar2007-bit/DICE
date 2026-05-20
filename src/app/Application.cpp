#include "app/Application.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace dice {

Application::Application()
    : view_(window_),
      controller_(model_, view_, lua_, window_, textures_) {
    model_.setFactory(dice::scene::makeDefaultFactory());
    initialized_ = false;
}

Application::~Application() {
    shutdown();
}

void Application::run() {
    if (initialized_) {
        spdlog::warn("Application already running");
        return;
    }
    initialized_ = true;

    config_ = AppConfig::loadFromFile("game.json");

    if (!initWindow()) {
        spdlog::error("Failed to initialize window");
        return;
    }

    initResources();
    initLua();
    initView();
    initController();

    spdlog::info("=== DICE Application Started ===");

    while (running_ && window_.isOpen()) {
        float dt = clock_.restart().asSeconds();
        if (dt > 0.033f) {
            dt = 0.033f;
        }
        handleEvents();
        update(dt);
        render();
    }

    spdlog::info("=== DICE Application Stopped ===");
}

bool Application::initWindow() {
    try {
        window_.create(
            sf::VideoMode(config_.windowWidth, config_.windowHeight),
            config_.title);

        if (!window_.isOpen()) {
            spdlog::error("Window failed to open");
            return false;
        }

        window_.setFramerateLimit(config_.framerateLimit);
        spdlog::info("Window created: {}x{}", config_.windowWidth, config_.windowHeight);
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Window creation failed: {}", e.what());
        return false;
    }
}

void Application::initResources() {
    auto fallback = std::make_shared<sf::Texture>();
    if (!fallback->create(64, 64)) {
        spdlog::error("Failed to create fallback texture");
    }
    textures_.setFallback(fallback);

    for (const auto& entry : config_.fonts) {
        if (fs::exists(entry.path)) {
            fonts_.load(entry.id, entry.path);
            spdlog::info("Font loaded: {}", entry.path);
        }
    }
}

void Application::initLua() {
    try {
        lua_.registerFunction("log", [](const std::string& msg) { spdlog::info("[Lua] {}", msg); });

        lua_.registerFunction("warn",
                              [](const std::string& msg) { spdlog::warn("[Lua] {}", msg); });

        spdlog::info("Lua initialized");
    } catch (const std::exception& e) {
        spdlog::error("Lua initialization failed: {}", e.what());
    }
}

void Application::initView() {
    view_.setFontManager(&fonts_);

    view::ViewConfig vcfg;
    vcfg.showFPS         = config_.showFPS;
    vcfg.showObjectCount = config_.showObjectCount;
    vcfg.showControls    = config_.showControls;
    vcfg.fontAssetId     = config_.fonts.empty() ? "default_font" : config_.fonts[0].id;
    view_.setConfig(vcfg);

    spdlog::info("View initialized");
}

void Application::initController() {
    auto fontId = config_.fonts.empty() ? "default_font" : config_.fonts[0].id;
    auto font = fonts_.get(fontId);
    controller_.registerDefaultFunctions(font.get());
    controller_.loadScene(config_.startScene);
    spdlog::info("Controller initialized");
}

void Application::handleEvents() {
    sf::Event event;
    while (window_.pollEvent(event)) {
        try {
            if (event.type == sf::Event::Closed) {
                running_ = false;
                window_.close();
                return;
            }
            if (event.type == sf::Event::KeyPressed &&
                event.key.code == sf::Keyboard::Escape) {
                running_ = false;
                window_.close();
                return;
            }
            controller_.handleEvent(event);
        } catch (const std::exception& e) {
            spdlog::error("Error handling event: {}", e.what());
        }
    }
}

void Application::update(float dt) {
    try {
        controller_.update(dt);
    } catch (const std::exception& e) {
        spdlog::error("Error in update: {}", e.what());
    }
}

void Application::render() {
    try {
        window_.clear(sf::Color(config_.clearR, config_.clearG, config_.clearB));
        auto objects = controller_.collectObjects();
        view_.render(objects);
        window_.display();
    } catch (const std::exception& e) {
        spdlog::error("Error in render: {}", e.what());
        window_.display();
    }
}

void Application::shutdown() {
    running_ = false;
    initialized_ = false;
    if (window_.isOpen()) {
        window_.close();
    }
    spdlog::info("Application shut down");
}

} // namespace dice
