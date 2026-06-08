#include "components/Tile.hpp"

#include <algorithm>

namespace dice::components {

Tile::Tile(const std::string& id, const std::string& name) : GameObject(id, name) {
    setType("Tile");
}

bool Tile::accepts(const std::string& type) const {
    if (acceptedTypes_.empty()) {
        return true;
    }
    return std::find(acceptedTypes_.begin(), acceptedTypes_.end(), type) != acceptedTypes_.end();
}

nlohmann::json Tile::toJson() const {
    nlohmann::json json = GameObject::toJson();
    json["col"] = col_;
    json["row"] = row_;
    if (!occupantId_.empty()) {
        json["occupantId"] = occupantId_;
    }
    if (!acceptedTypes_.empty()) {
        json["acceptedTypes"] = acceptedTypes_;
    }
    return json;
}

void Tile::fromJson(const nlohmann::json& json) {
    GameObject::fromJson(json);
    if (json.contains("col")) {
        col_ = json["col"].get<int>();
    }
    if (json.contains("row")) {
        row_ = json["row"].get<int>();
    }
    if (json.contains("occupantId")) {
        occupantId_ = json["occupantId"].get<std::string>();
    }
    if (json.contains("acceptedTypes")) {
        acceptedTypes_ = json["acceptedTypes"].get<std::vector<std::string>>();
    }
}

} // namespace dice::components
