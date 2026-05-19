#ifndef DICE_APP_APPLICATION_HPP
#define DICE_APP_APPLICATION_HPP

#include <memory>
#include <string>

#include <SFML/Graphics.hpp>

#include "controller/Controller.hpp"
#include "core/ActionManager.hpp"
#include "core/Model.hpp"
#include "core/ResourceManager.hpp"
#include "scripting/LuaScriptEngine.hpp"
#include "ui/View.hpp"
#include <spdlog/spdlog.h>

namespace dice {

class Application {
public:
    Application();
    ~Application();

    void run();

private:
    bool initWindow();
    void initResources();
    void initLua();
    void initView();
    void initController();
    void loadScene();

    void handleEvents();
    void update(float dt);
    void render();
    void shutdown();

    sf::RenderWindow window_;

    core::ResourceManager<sf::Texture> textures_;
    core::ResourceManager<sf::Font> fonts_;

    core::Model model_;
    core::ActionManager actions_;

    view::View view_;
    scripting::LuaScriptEngine lua_;
    controller::Controller controller_;

    bool running_ = true;
    bool initialized_ = false;
    sf::Clock clock_;
};

} // namespace dice

#endif