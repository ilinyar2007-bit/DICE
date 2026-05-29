#ifndef DICE_GAME_OBJECT_HPP
#define DICE_GAME_OBJECT_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>

namespace dice::core {

class GameObject : public sf::Drawable, public sf::Transformable {
public:
    GameObject();

    GameObject(std::string id, std::string name);

    ~GameObject() override = default;

    GameObject(const GameObject&) = delete;
    GameObject& operator=(const GameObject&) = delete;
    GameObject(GameObject&&) = delete;
    GameObject& operator=(GameObject&&) = delete;

    // ========== Identification ==========

    const std::string& getId() const {
        return id_;
    }

    void setId(const std::string& id) {
        id_ = id;
    }

    const std::string& getName() const {
        return name_;
    }

    void setName(const std::string& name) {
        name_ = name;
    }

    const std::string& getType() const {
        return type_;
    }

    void setType(const std::string& type) {
        type_ = type;
    }

    // ========== Metadata ==========

    const std::string& getDescription() const {
        return description_;
    }

    void setDescription(const std::string& desc) {
        description_ = desc;
    }

    void addTag(const std::string& tag);

    void removeTag(const std::string& tag);

    bool hasTag(const std::string& tag) const;

    const std::vector<std::string>& getTags() const {
        return tags_;
    }

    // ========== Visual representation ==========

    void setTexture(const sf::Texture* texture);

    sf::Sprite& getSprite() {
        return sprite_;
    }
    const sf::Sprite& getSprite() const {
        return sprite_;
    }

    void setColor(const sf::Color& color);

    sf::Color getColor() const {
        return sprite_.getColor();
    }

    // ========== Z-order (layers) ==========

    int getZOrder() const {
        return zOrder_;
    }

    void setZOrder(int order) {
        zOrder_ = order;
    }


    void moveUp() {
        ++zOrder_;
    }

    void moveDown() {
        --zOrder_;
    }

    // ========== Boundaries and collisions ==========

    sf::FloatRect getGlobalBounds() const;


    sf::FloatRect getLocalBounds() const;

    // Check if an object contains a point
    virtual bool contains(const sf::Vector2f& point) const;

    // Check if an object contains a point
    bool intersects(const GameObject& other) const;

    // ========== Hierarchy of objects ==========

    void addChild(std::shared_ptr<GameObject> child);

    void removeChild(const std::string& child_id);

    void shuffleChildren();

    std::shared_ptr<GameObject> getChild(const std::string& child_id);

    const std::vector<std::shared_ptr<GameObject>>& getChildren() const {
        return children_;
    }

    GameObject* getParent() {
        return parent_;
    }
    const GameObject* getParent() const {
        return parent_;
    }

    void setParent(GameObject* parent) {
        parent_ = parent;
    }

    // ========== Condition ==========

    bool isActive() const {
        return active_;
    }

    void setActive(bool active) {
        active_ = active;
    }

    bool isVisible() const {
        return visible_;
    }

    void setVisible(bool visible) {
        visible_ = visible;
    }

    bool isDraggable() const {
        return draggable_;
    }

    void setDraggable(bool draggable) {
        draggable_ = draggable;
    }

    // ========== Custom properties ==========

    // Set a custom property
    template <typename T> void setProperty(const std::string& key, const T& value) {
        properties_[key] = value;
    }

    // Get a custom property
    template <std::default_initializable T>
    T getProperty(const std::string& key, const T& default_value = T()) const {
        auto it = properties_.find(key);
        if (it != properties_.end()) {
            try {
                return it->second.get<T>();
            } catch (...) {
                return default_value;
            }
        }
        return default_value;
    }

    bool hasProperty(const std::string& key) const {
        return properties_.find(key) != properties_.end();
    }

    // ========== Updating and rendering ==========

    virtual void update(float delta_time);

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    // ========== Serialization ==========

    virtual nlohmann::json toJson() const;

    virtual void fromJson(const nlohmann::json& json);

    // ========== Lua integration ==========

    const std::string& getLuaScript() const {
        return luaScript_;
    }

    void setLuaScript(const std::string& script) {
        luaScript_ = script;
    }

    // ========== Trigger bindings ==========

    void setTriggerBinding(const std::string& event, const std::string& trigger_name) {
        triggerBindings_[event] = trigger_name;
    }

    const std::unordered_map<std::string, std::string>& getTriggerBindings() const {
        return triggerBindings_;
    }

    void clearTriggerBindings() {
        triggerBindings_.clear();
    }

    void setTriggerBindings(std::unordered_map<std::string, std::string> bindings) {
        triggerBindings_ = std::move(bindings);
    }

    // ========== Behavior Presets ==========

    const std::vector<std::string>& getPresets() const {
        return presets_;
    }

    // ========== Texture file ==========

    const std::string& getTextureFile() const {
        return textureFile_;
    }

    void setTextureFile(const std::string& path) {
        textureFile_ = path;
    }

private:
    std::string id_;
    std::string name_;
    std::string type_;
    std::string description_;
    std::vector<std::string> tags_;

    sf::Sprite sprite_;
    int zOrder_;

    GameObject* parent_;
    std::vector<std::shared_ptr<GameObject>> children_;

    bool active_;
    bool visible_;
    bool draggable_;

    std::unordered_map<std::string, nlohmann::json> properties_;

    std::string luaScript_;
    std::string textureFile_;
    std::unordered_map<std::string, std::string> triggerBindings_;
    std::vector<std::string> presets_;
};

} // namespace dice::core

#endif // DICE_GAME_OBJECT_HPP
