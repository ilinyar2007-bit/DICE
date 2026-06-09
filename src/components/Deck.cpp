#include "components/Deck.hpp"

#include "components/Card.hpp"
#include <spdlog/spdlog.h>

namespace dice::components {

Deck::Deck(const std::string& id, const std::string& name) : GameObject(id, name) {
    setType("Deck");
}

void Deck::setFaceDown(bool face_down) {
    faceDown_ = face_down;
    for (const auto& child : getChildren()) {
        if (auto* card = dynamic_cast<Card*>(child.get())) {
            card->setFaceUp(!face_down);
        }
    }
    spdlog::debug("Deck '{}': faceDown={}", getId(), face_down);
}

nlohmann::json Deck::toJson() const {
    nlohmann::json json = GameObject::toJson();
    json["faceDown"] = faceDown_;
    return json;
}

void Deck::fromJson(const nlohmann::json& json) {
    GameObject::fromJson(json);
    if (json.contains("faceDown")) {
        faceDown_ = json["faceDown"].get<bool>();
    }
}

} // namespace dice::components
