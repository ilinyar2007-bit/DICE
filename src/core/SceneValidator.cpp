#include "SceneValidator.hpp"

#include <filesystem>
#include <unordered_set>

#include <spdlog/spdlog.h>

namespace dice::core {

void SceneValidator::validate(const nlohmann::json& sceneJson) {
    errors_.clear();
    warnings_.clear();

    if (!checkSceneRoot(sceneJson)) {
        return false;
    }

    checkDuplicateIds(sceneJson);

    for (const auto& obj : sceneJson["objects"]) {
        checkObject(obj);
    }
}

void SceneValidator::addError(const std::optional<std::string>& objectId,
                              const std::string& message) {
    if (objectId.has_value()) {
        spdlog::error("Message from SceneValidator: <{}>, an object with id {} is involved",
                      message,
                      objectId.value());
    } else {
        spdlog::error("SceneValidator: '{}'", message);
    }
    errors_.push_back({objectId, message});
}

void SceneValidator::addWarning(const std::optional<std::string>& objectId,
                                const std::string& message) {
    if (objectId.has_value()) {
        spdlog::warn("Message from SceneValidator: <{}>, an object with id {} is involved",
                     message,
                     objectId.value());
    } else {
        spdlog::warn("SceneValidator: '{}'", message);
    }
    warnings_.push_back({objectId, message});
}

bool SceneValidator::checkSceneRoot(const nlohmann::json& sceneJson) {
    if (!sceneJson.is_object()) {
        addError(std::nullopt, "Scene root must be object");
        return false;
    }

    if (!sceneJson.contains("objects")) {
        addError(std::nullopt, "Missing field: objects");
        return false;
    }

    if (!sceneJson["objects"].is_array()) {
        addError(std::nullopt, "'objects' must be array");
        return false;
    }

    return true;
}

void SceneValidator::checkObject(const nlohmann::json& obj) {
    if (!obj.is_object()) {
        addError(std::nullopt, "Object entry must be object");
        return;
    }

    checkRequiredFields(obj);

    checkTransform(obj);
    checkColor(obj);

    checkTextureFile(obj);
    checkLuaScript(obj);

    checkTags(obj);
    checkProperties(obj);
}

void SceneValidator::checkRequiredFields(const nlohmann::json& obj) {
    const auto id = tryGetId(obj);

    if (!obj.contains("id")) {
        addError(id, "Missing field: id");
    } else if (!obj["id"].is_string()) {
        addError(id, "'id' must be string");
    }

    if (!obj.contains("type")) {
        addError(id, "Missing field: type");
    } else if (!obj["type"].is_string()) {
        addError(id, "'type' must be string");
    }
}

void SceneValidator::checkDuplicateIds(const nlohmann::json& sceneJson) {
    std::unordered_set<std::string> ids;

    for (const auto& obj : sceneJson["objects"]) {
        auto id = tryGetId(obj);

        if (!id.has_value()) {
            continue;
        }

        if (ids.contains(id.value())) {
            addError(id, "Duplicate object id");
        }

        ids.insert(id.value());
    }
}

void SceneValidator::checkTransform(const nlohmann::json& obj) {
    const auto id = tryGetId(obj);

    if (obj.contains("position")) {
        const auto& position = obj["position"];

        if (!position.is_array() || position.size() != 2) {
            addError(id, "'position' must contain 2 values");
        } else if (!isNumber(position[0]) || !isNumber(position[1])) {
            addError(id, "'position' values must be numeric");
        }
    }

    if (obj.contains("scale")) {
        const auto& scale = obj["scale"];

        if (!scale.is_array() || scale.size() != 2) {
            addError(id, "'scale' must contain 2 values");
        } else if (!isNumber(scale[0]) || !isNumber(scale[1])) {
            addError(id, "'scale' values must be numeric");
        }
    }

    if (obj.contains("rotation")) {
        if (!isNumber(obj["rotation"])) {
            addError(id, "'rotation' must be numeric");
        }
    }
}

void SceneValidator::checkColor(const nlohmann::json& obj) {
    const auto id = tryGetId(obj);

    if (!obj.contains("color")) {
        return;
    }

    const auto& color = obj["color"];

    if (!color.is_array() || color.size() != 4) {
        addError(id, "'color' must contain 4 values");
        return;
    }

    for (const auto& value : color) {
        if (!value.is_number_integer()) {
            addError(id, "'color' values must be integers");
            return;
        }

        const int channel = value.get<int>();

        if (channel < 0 || channel > 255) {
            addError(id, "'color' values must be in range 0..255");
            return;
        }
    }
}

void SceneValidator::checkTextureFile(const nlohmann::json& obj) {
    const auto id = tryGetId(obj);

    if (!obj.contains("textureFile")) {
        return;
    }

    if (!obj["textureFile"].is_string()) {
        addError(id, "'textureFile' must be string");
        return;
    }

    const auto path = obj["textureFile"].get<std::string>();

    if (!path.empty() && !std::filesystem::exists(path)) {
        addWarning(id, "Texture file not found: " + path);
    }
}

void SceneValidator::checkLuaScript(const nlohmann::json& obj) {
    const auto id = tryGetId(obj);

    if (!obj.contains("luaScript")) {
        return;
    }

    if (!obj["luaScript"].is_string()) {
        addError(id, "'luaScript' must be string");
        return;
    }

    const auto path = obj["luaScript"].get<std::string>();

    if (!path.empty() && !std::filesystem::exists(path)) {
        addWarning(id, "Lua script not found: " + path);
    }
}

void SceneValidator::checkTags(const nlohmann::json& obj) {
    const auto id = tryGetId(obj);

    if (!obj.contains("tags")) {
        return;
    }

    if (!obj["tags"].is_array()) {
        addError(id, "'tags' must be array");
        return;
    }

    for (const auto& tag : obj["tags"]) {
        if (!tag.is_string()) {
            addError(id, "'tags' values must be strings");
            return;
        }
    }
}

void SceneValidator::checkProperties(const nlohmann::json& obj) {
    const auto id = tryGetId(obj);

    if (!obj.contains("properties")) {
        return;
    }

    if (!obj["properties"].is_object()) {
        addError(id, "'properties' must be object");
    }
}

std::optional<std::string> SceneValidator::tryGetId(const nlohmann::json& obj) const {
    if (!obj.contains("id")) {
        return std::nullopt;
    }

    if (!obj["id"].is_string()) {
        return std::nullopt;
    }

    return obj["id"].get<std::string>();
}

bool SceneValidator::isNumber(const nlohmann::json& value) const {
    return value.is_number_integer() || value.is_number_unsigned() || value.is_number_float();
}


} // namespace dice::core