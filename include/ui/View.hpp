#ifndef DICE_VIEW_HPP
#define DICE_VIEW_HPP

#include <algorithm>
#include <memory>
#include <vector>

#include <SFML/Graphics.hpp>

#include "core/GameObject.hpp"
#include "core/ResourceManager.hpp"
#include <spdlog/spdlog.h>

namespace dice::view {

struct ViewConfig {
    sf::Color backgroundColor = sf::Color(50, 50, 50);

    // Interface Settings
    bool showFPS = true;
    bool showObjectCount = true;
    bool showControls = true;

    sf::Color textColor = sf::Color::White;

    // Font
    std::string fontAssetId = "default_font";
};

class View {
public:
    explicit View(sf::RenderWindow& window);

    View(const View&) = delete;
    View& operator=(const View&) = delete;
    View(View&&) = delete;
    View& operator=(View&&) = delete;

    ~View() = default;

    // ========== Basic methods ==========

    void render(const std::vector<std::shared_ptr<core::GameObject>>& objects);

    void update(float delta_time);

    void handleEvent(const sf::Event& event);

    // ========== Settings ==========

    void setConfig(const ViewConfig& config) {
        config_ = config;
    }
    ViewConfig& getConfig() {
        return config_;
    }

    // ========== Font management ==========

    void setFontManager(core::ResourceManager<sf::Font>* manager) {
        fontManager_ = manager;
        loadFontAsset();
    }

    // ========== Coordinate transformation ==========

    sf::Vector2f screenToWorld(const sf::Vector2i& screen_point) const;
    sf::Vector2i worldToScreen(const sf::Vector2f& world_point) const;

    // ========== Object search ==========

    std::shared_ptr<core::GameObject>
    pickObject(const sf::Vector2f& world_pos,
               const std::vector<std::shared_ptr<core::GameObject>>& objects) const;

private:
    // ========== Internal rendering methods ==========

    void clear();

    void drawObjects(const std::vector<std::shared_ptr<core::GameObject>>& objects);

    void drawObject(const std::shared_ptr<core::GameObject>& obj);

    void drawHUD(const std::vector<std::shared_ptr<core::GameObject>>& objects);

    void drawFPS();
    void drawObjectCount(const std::vector<std::shared_ptr<core::GameObject>>& objects);
    void drawControls();

    // Support functions

    void loadFontAsset() const;
    sf::Font& getFont() const;
    sf::Text createText(
        const std::string& str, unsigned int size, const sf::Color& color, float x, float y) const;
    static void sortObjectsByZOrder(std::vector<std::shared_ptr<core::GameObject>>& objects);

    // ========== Condition ==========

    sf::RenderWindow& window_;
    ViewConfig config_;

    float fps_ = 0.0F;
    int frameCount_ = 0;
    float fpsTimer_ = 0.0F;

    mutable std::shared_ptr<sf::Font> font_;
    mutable bool fontLoaded_ = false;
    core::ResourceManager<sf::Font>* fontManager_ = nullptr;

    mutable std::vector<std::shared_ptr<core::GameObject>> sortedObjects_;
};

} // namespace dice::view

#endif