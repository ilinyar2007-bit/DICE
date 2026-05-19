#include "app/Application.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace dice {

Application::Application() : actions_(), view_(window_), controller_(model_, view_, lua_, window_) {
    // controller_.setActionManager(&actions_);
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

    if (!initWindow()) {
        spdlog::error("Failed to initialize window");
        return;
    }

    initResources();
    initLua();
    initView();
    initController();
    loadScene();

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
        window_.create(sf::VideoMode(1280, 720), "DICE");

        if (!window_.isOpen()) {
            spdlog::error("Window failed to open");
            return false;
        }

        window_.setFramerateLimit(60);
        spdlog::info("Window created: 1280x720");
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

    std::vector<std::string> fontPaths = {"assets/fonts/OpenSans-Regular.ttf"};

    bool fontLoaded = false;
    for (const auto& path : fontPaths) {
        if (fs::exists(path)) {
            if (fonts_.load("default_font", path)) {
                spdlog::info("Font loaded: {}", path);
                fontLoaded = true;
                break;
            }
        }
    }

    if (!fontLoaded) {
        spdlog::warn("No font found, text rendering will be disabled");
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

    view::ViewConfig config;
    config.showFPS = true;
    config.showObjectCount = true;
    config.showControls = true;
    config.fontAssetId = "default_font";
    view_.setConfig(config);

    spdlog::info("View initialized");
}

void Application::initController() {
    controller_.loadTextures(textures_);

    auto font = fonts_.get("default_font");
    controller_.registerDefaultFunctions(textures_, font.get());

    spdlog::info("Controller initialized");
}

void Application::loadScene() {
    std::string scenePath = "assets/scenes/main.json";

    if (!fs::exists(scenePath)) {
        spdlog::warn("Scene not found: {}", scenePath);
        return;
    }

    std::ifstream file(scenePath);
    if (!file.is_open()) {
        spdlog::error("Cannot open scene: {}", scenePath);
        return;
    }

    try {
        nlohmann::json json;
        file >> json;
        model_.fromJson(json);
        actions_.saveSnapshot(model_);
        spdlog::info("Scene loaded: {}", scenePath);
    } catch (const nlohmann::json::parse_error& e) {
        spdlog::error("Failed to parse scene JSON: {}", e.what());
    } catch (const std::exception& e) {
        spdlog::error("Failed to load scene: {}", e.what());
    }
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

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    running_ = false;
                    window_.close();
                    return;
                }

                if (event.key.control) {
                    if (event.key.code == sf::Keyboard::Z && actions_.canUndo()) {
                        actions_.undo(model_);
                    }
                    if (event.key.code == sf::Keyboard::Y && actions_.canRedo()) {
                        actions_.redo(model_);
                    }
                }
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
        window_.clear(sf::Color(30, 30, 40));

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