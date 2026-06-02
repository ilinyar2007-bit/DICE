#include "core/GameObject.hpp"

#include <algorithm>
#include <random>

#include <spdlog/spdlog.h>

namespace dice::core {

GameObject::GameObject()
    : name_("Unnamed"), type_("generic"), zOrder_(0), parent_(nullptr), active_(true),
      visible_(true), draggable_(false) {}

GameObject::GameObject(std::string id, std::string name)
    : id_(std::move(id)), name_(std::move(name)), type_("generic"), zOrder_(0), parent_(nullptr),
      active_(true), visible_(true), draggable_(false) {}

// ========== Metadata ==========

void GameObject::addTag(const std::string& tag) {
    if (!hasTag(tag)) {
        tags_.push_back(tag);
        spdlog::debug("Added tag '{}' to object '{}'", tag, id_);
    }
}

void GameObject::removeTag(const std::string& tag) {
    auto it = std::find(tags_.begin(), tags_.end(), tag);
    if (it != tags_.end()) {
        tags_.erase(it);
        spdlog::debug("Removed tag '{}' from object '{}'", tag, id_);
    }
}

bool GameObject::hasTag(const std::string& tag) const {
    return std::find(tags_.begin(), tags_.end(), tag) != tags_.end();
}

// ========== Visual representation ==========

void GameObject::setTexture(const sf::Texture* texture) {
    if (texture != nullptr) {
        sprite_.setTexture(*texture, true);
        auto bounds = sprite_.getLocalBounds();
        sprite_.setOrigin(bounds.width / 2.F, bounds.height / 2.F);
        spdlog::debug("Set texture for object '{}'", id_);
    }
}

void GameObject::setColor(const sf::Color& color) {
    sprite_.setColor(color);
}

// ========== Boundaries and collisions ==========

sf::FloatRect GameObject::getGlobalBounds() const {
    return getTransform().transformRect(sprite_.getGlobalBounds());
}

sf::FloatRect GameObject::getLocalBounds() const {
    return sprite_.getLocalBounds();
}

bool GameObject::contains(const sf::Vector2f& point) const {
    return getGlobalBounds().contains(point);
}

bool GameObject::intersects(const GameObject& other) const {
    return getGlobalBounds().intersects(other.getGlobalBounds());
}

// ========== Hierarchy of objects ==========

void GameObject::addChild(std::shared_ptr<GameObject> child) {
    if (!child) {
        spdlog::warn("Attempted to add null child to object '{}'", id_);
        return;
    }

    auto it = std::find_if(children_.begin(), children_.end(), [&child](const auto& c) {
        return c->getId() == child->getId();
    });

    if (it == children_.end()) {
        child->setParent(this);
        children_.push_back(child);
        spdlog::debug("Added child '{}' to object '{}'", child->getId(), id_);
    } else {
        spdlog::warn("Child '{}' already exists in object '{}'", child->getId(), id_);
    }
}

void GameObject::removeChild(const std::string& child_id) {
    auto it = std::find_if(children_.begin(), children_.end(), [&child_id](const auto& c) {
        return c->getId() == child_id;
    });

    if (it != children_.end()) {
        (*it)->setParent(nullptr);
        children_.erase(it);
        spdlog::debug("Removed child '{}' from object '{}'", child_id, id_);
    } else {
        spdlog::warn("Child '{}' not found in object '{}'", child_id, id_);
    }
}

void GameObject::shuffleChildren() {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::shuffle(children_.begin(), children_.end(), rng);
    spdlog::debug("Shuffled children of object '{}'", id_);
}

std::shared_ptr<GameObject> GameObject::getChild(const std::string& child_id) {
    auto it = std::find_if(children_.begin(), children_.end(), [&child_id](const auto& c) {
        return c->getId() == child_id;
    });

    if (it != children_.end()) {
        return *it;
    }

    return nullptr;
}

// ========== Updating and rendering ==========

void GameObject::update(float delta_time) { // NOLINT(misc-no-recursion)
    if (!active_) {
        return;
    }

    for (auto& child : children_) {
        if (child && child->isActive()) {
            child->update(delta_time);
        }
    }
}

void GameObject::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!visible_) {
        return;
    }

    states.transform *= getTransform();

    target.draw(sprite_, states);

    for (const auto& child : children_) {
        if (child && child->isVisible()) {
            target.draw(*child, states);
        }
    }
}

// ========== Serialization ==========

nlohmann::json GameObject::toJson() const { // NOLINT(misc-no-recursion)
    nlohmann::json json;

    json["id"] = id_;
    json["name"] = name_;
    json["type"] = type_;
    json["description"] = description_;
    json["tags"] = tags_;

    auto pos = getPosition();
    json["position"] = {pos.x, pos.y};
    json["rotation"] = getRotation();
    auto scale = getScale();
    json["scale"] = {scale.x, scale.y};

    auto color = sprite_.getColor();
    json["color"] = {color.r, color.g, color.b, color.a};
    json["zOrder"] = zOrder_;

    json["active"] = active_;
    json["visible"] = visible_;
    json["draggable"] = draggable_;

    json["properties"] = properties_;

    if (!luaScript_.empty()) {
        json["luaScript"] = luaScript_;
    }

    if (!triggerBindings_.empty()) {
        json["triggers"] = nlohmann::json::object();
        for (const auto& [event, name] : triggerBindings_) {
            json["triggers"][event] = name;
        }
    }

    if (!textureFile_.empty()) {
        json["textureFile"] = textureFile_;
    }

    if (!children_.empty()) {
        nlohmann::json childrenJson = nlohmann::json::array();
        for (const auto& child : children_) {
            if (child) {
                childrenJson.push_back(child->toJson());
            }
        }
        json["children"] = childrenJson;
    }

    return json;
}

void GameObject::fromJson(const nlohmann::json& json) {
    if (json.contains("id")) {
        id_ = json["id"];
    }
    if (json.contains("name")) {
        name_ = json["name"];
    }
    if (json.contains("type")) {
        type_ = json["type"];
    }
    if (json.contains("description")) {
        description_ = json["description"];
    }

    if (json.contains("tags")) {
        tags_ = json["tags"].get<std::vector<std::string>>();
    }

    if (json.contains("position")) {
        auto pos = json["position"];
        setPosition(pos[0], pos[1]);
    }
    if (json.contains("rotation")) {
        setRotation(json["rotation"]);
    }
    if (json.contains("scale")) {
        auto scale = json["scale"];
        setScale(scale[0], scale[1]);
    }

    if (json.contains("color")) {
        auto color = json["color"];
        setColor(sf::Color(color[0], color[1], color[2], color[3]));
    }
    if (json.contains("zOrder")) {
        zOrder_ = json["zOrder"];
    }

    if (json.contains("active")) {
        active_ = json["active"];
    }
    if (json.contains("visible")) {
        visible_ = json["visible"];
    }
    if (json.contains("draggable")) {
        draggable_ = json["draggable"];
    }

    if (json.contains("properties")) {
        properties_ = json["properties"].get<std::unordered_map<std::string, nlohmann::json>>();
    }

    if (json.contains("luaScript")) {
        luaScript_ = json["luaScript"];
    }

    triggerBindings_.clear();
    if (json.contains("triggers") && json["triggers"].is_object()) {
        for (const auto& [event, triggerName] : json["triggers"].items()) {
            if (triggerName.is_string()) {
                triggerBindings_[event] = triggerName.get<std::string>();
            }
        }
    }

    if (json.contains("presets") && json["presets"].is_array()) {
        presets_ = json["presets"].get<std::vector<std::string>>();
    }

    if (json.contains("textureFile")) {
        textureFile_ = json["textureFile"];
    }

    spdlog::debug("Loaded GameObject '{}' from JSON", id_);
}

} // namespace dice::core
