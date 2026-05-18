#include <components/Card.hpp>
#include <spdlog/spdlog.h>

namespace dice::components {

Card::Card(const std::string& id, const std::string& name)
    : GameObject(id, name), player_num_(0), front_texture_(nullptr), back_texture_(nullptr),
      face_up_(false) {
    setType("Card");
}

// ================= Textures =================

void Card::setFrontTexture(const sf::Texture* texture) {
    front_texture_ = texture;

    if (face_up_ && front_texture_ != nullptr) {
        setTexture(front_texture_);
    }
}

void Card::setBackTexture(const sf::Texture* texture) {
    back_texture_ = texture;

    if (!face_up_ && back_texture_ != nullptr) {
        setTexture(back_texture_);
    }
}

// ================= State =================

void Card::setFaceUp(bool face_up) {
    face_up_ = face_up;
    spdlog::debug("Card '{}': face {}", getId(), face_up ? "up" : "down");

    if (face_up_) {
        if (front_texture_ != nullptr) {
            setTexture(front_texture_);
        }
    } else {
        if (back_texture_ != nullptr) {
            setTexture(back_texture_);
        }
    }
}

// ================= Serialization =================

nlohmann::json Card::toJson() const {
    nlohmann::json json = GameObject::toJson();

    json["player"] = player_num_;
    json["face_up"] = face_up_;

    return json;
}

void Card::fromJson(const nlohmann::json& json) {
    GameObject::fromJson(json);

    if (json.contains("player")) {
        player_num_ = json["player"];
    }

    if (json.contains("face_up")) {
        setFaceUp(json["face_up"]);
    }
}

} // namespace dice::components