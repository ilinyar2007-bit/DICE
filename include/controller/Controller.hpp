#ifndef DICE_CONTROLLER_CONTROLLER_HPP
#define DICE_CONTROLLER_CONTROLLER_HPP

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include <SFML/Graphics.hpp>

#include "core/GameObject.hpp"
#include "core/Model.hpp"
#include "core/ResourceManager.hpp"
#include "scripting/LuaScriptEngine.hpp"
#include "ui/View.hpp"

namespace dice::controller {

class Controller {
public:
    Controller(dice::core::Model& model,
               dice::view::View& view,
               dice::scripting::LuaScriptEngine& lua,
               sf::RenderWindow& window,
               dice::core::ResourceManager<sf::Texture>& textures);

    ~Controller() = default;
    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;
    Controller(Controller&&) = delete;
    Controller& operator=(Controller&&) = delete;

    [[nodiscard]] bool loadScene(const std::filesystem::path& path);

    void registerDefaultFunctions(const sf::Font* font);

    void handleEvent(const sf::Event& event);
    void update(float dt);

    [[nodiscard]] std::vector<std::shared_ptr<dice::core::GameObject>> collectObjects() const;

private:
    void onMousePressed(const sf::Event::MouseButtonEvent& ev);
    void onMouseMoved(const sf::Event::MouseMoveEvent& ev);
    void onMouseReleased(const sf::Event::MouseButtonEvent& ev);
    void refreshFieldBounds();
    void loadTexturesForModel();

    dice::core::Model& model_;
    dice::view::View& view_;
    dice::scripting::LuaScriptEngine& lua_;
    sf::RenderWindow& window_;
    dice::core::ResourceManager<sf::Texture>& textures_;

    std::filesystem::path currentScenePath_;
    std::filesystem::path pendingScenePath_;

    std::shared_ptr<dice::core::GameObject> draggedObj_;
    sf::Vector2f dragOffset_;
    bool wasDragging_{false};
    float chipHalfW_{0.F};
    float chipHalfH_{0.F};

    std::weak_ptr<dice::core::GameObject> hoveredObj_;
    sf::FloatRect fieldBounds_{0.F, 0.F, 1280.F, 720.F};
    std::unordered_set<std::string> loadedTextureIds_;
};

} // namespace dice::controller
#endif // DICE_CONTROLLER_CONTROLLER_HPP
