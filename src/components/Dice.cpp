#include "components/Dice.hpp"

namespace dice::components {

Dice::Dice(const std::string& id, const std::string& name)
    : GameObject(id, name), value_(0), asset_id_("") {
    setType("Dice");
}

nlohmann::json Dice::toJson() const {
    nlohmann::json json = GameObject::toJson();
    json["value"] = value_;
    if (!asset_id_.empty()) {
        json["assetId"] = asset_id_;
    }
    return json;
}

void Dice::fromJson(const nlohmann::json& json) {
    GameObject::fromJson(json);

    if (json.contains("value")) {
        value_ = json["value"].get<int>();
    }

    if (json.contains("assetId")) {
        asset_id_ = json["assetId"].get<std::string>();
    }
}

} // namespace dice::components
