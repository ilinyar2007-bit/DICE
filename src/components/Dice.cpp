#include "components/Dice.hpp"

#include <random>

#include <spdlog/spdlog.h>

namespace dice::components {

Dice::Dice(const std::string& id, const std::string& name) : GameObject(id, name) {
    setType("Dice");
}

const std::string& Dice::getFaceTexturePath(int value) const {
    static const std::string kEmpty;
    if (value >= 1 && value <= static_cast<int>(faceTextures_.size())) {
        return faceTextures_[static_cast<std::size_t>(value - 1)];
    }
    return kEmpty;
}

int Dice::roll() {
    if (faceCount_ < 1) {
        spdlog::warn("Dice '{}': faceCount is {}, clamping to 1", getId(), faceCount_);
        faceCount_ = 1;
    }
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(1, faceCount_);
    value_ = dist(rng);
    spdlog::debug("Dice '{}': rolled {}", getId(), value_);
    return value_;
}

nlohmann::json Dice::toJson() const {
    nlohmann::json json = GameObject::toJson();
    json["faceCount"] = faceCount_;
    json["value"] = value_;
    if (!faceTextures_.empty()) {
        json["faceTextures"] = faceTextures_;
    }
    return json;
}

void Dice::fromJson(const nlohmann::json& json) {
    GameObject::fromJson(json);
    if (json.contains("faceCount")) {
        faceCount_ = json["faceCount"].get<int>();
    }
    if (json.contains("value")) {
        value_ = json["value"].get<int>();
    }
    if (json.contains("faceTextures")) {
        faceTextures_ = json["faceTextures"].get<std::vector<std::string>>();
    }
}

} // namespace dice::components
