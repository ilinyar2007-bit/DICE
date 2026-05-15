#include "core/SceneValidator.hpp"

#include <filesystem>
#include <unordered_set>

#include <spdlog/spdlog.h>

namespace dice::core {

bool SceneValidator::validate(const nlohmann::json& scene_json,
                              const std::filesystem::path& scene_file_path) {
    clear();

    scene_directory_ = scene_file_path.parent_path();

    checkSceneRoot(scene_json);

    if (hasErrors()) {
        return false;
    }

    checkDuplicateIds(scene_json);

    for (const auto& obj : scene_json["objects"]) {
        checkObject(obj);
    }

    return !hasErrors();
}

void SceneValidator::addError(const MessageCode& code,
                              const std::string& message,
                              const nlohmann::json& context_object) {
    if (context_object.contains("id") && context_object["id"].is_string()) {
        spdlog::error("Message from SceneValidator: <{}>, an object with id {} is involved",
                      message,
                      context_object["id"].get<std::string>());
    } else {
        spdlog::error("SceneValidator: '{}'", message);
    }
    errors_.push_back({code, message, context_object});
}

void SceneValidator::addWarning(const MessageCode& code,
                                const std::string& message,
                                const nlohmann::json& context_object) {
    if (context_object.contains("id") && context_object["id"].is_string()) {
        spdlog::warn("Message from SceneValidator: <{}>, an object with id {} is involved",
                     message,
                     context_object["id"].get<std::string>());
    } else {
        spdlog::warn("SceneValidator: '{}'", message);
    }
    warnings_.push_back({code, message, context_object});
}

void SceneValidator::checkSceneRoot(const nlohmann::json& scene_json) {
    if (!scene_json.is_object()) {
        addError(
            MessageCode::E_SCENE_ROOT_IS_NOT_AN_OBJECT, "Scene root must be object", scene_json);
        return;
    }

    if (!scene_json.contains("objects")) {
        addError(MessageCode::E_MISSING_OBJECTS_ARRAY, "Missing field: objects", scene_json);
        return;
    }

    if (!scene_json["objects"].is_array()) {
        addError(MessageCode::E_OBJECTS_IS_NOT_AN_ARRAY, "'objects' must be array", scene_json);
        return;
    }
}

void SceneValidator::checkObject(const nlohmann::json& obj) {
    if (!obj.is_object()) {
        addError(MessageCode::E_OBJECT_ENTRY_IS_NOT_AN_OBJECT, "Object entry must be object", obj);
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
    if (!obj.contains("id")) {
        addError(MessageCode::E_MISSING_FIELD_ID, "Missing field: id", obj);
    } else if (!obj["id"].is_string()) {
        addError(MessageCode::E_ID_IS_NOT_A_STRING, "'id' must be string", obj);
    }

    if (!obj.contains("type")) {
        addError(MessageCode::E_MISSING_FIELD_TYPE, "Missing field: type", obj);
    } else if (!obj["type"].is_string()) {
        addError(MessageCode::E_TYPE_IS_NOT_A_STRING, "'type' must be string", obj);
    }
}

void SceneValidator::checkDuplicateIds(const nlohmann::json& scene_json) {
    std::unordered_set<std::string> ids;

    for (const auto& obj : scene_json["objects"]) {
        auto id = tryGetId(obj);

        if (!id.has_value()) {
            continue;
        }

        if (ids.contains(id.value())) {
            addError(MessageCode::E_DUPLICATE_ID, "Duplicate object id", obj);
        }

        ids.insert(id.value());
    }
}

void SceneValidator::checkTransform(const nlohmann::json& obj) {
    if (obj.contains("position")) {
        const auto& position = obj["position"];

        if (!position.is_array()) {
            addError(MessageCode::E_POSITION_IS_NOT_AN_ARRAY, "'position' must be array", obj);
        } else if (position.size() != 2) {
            addError(MessageCode::E_POSITION_DOES_NOT_CONTAIN_2_VALUES,
                     "'position' must contain 2 values",
                     obj);
        } else if (!isNumber(position[0]) || !isNumber(position[1])) {
            addError(MessageCode::E_POSITION_VALUES_ARE_NOT_NUMERIC,
                     "'position' values must be numeric",
                     obj);
        }
    }

    if (obj.contains("scale")) {
        const auto& scale = obj["scale"];

        if (!scale.is_array()) {
            addError(MessageCode::E_SCALE_IS_NOT_AN_ARRAY, "'scale' must be array", obj);
        } else if (scale.size() != 2) {
            addError(MessageCode::E_SCALE_DOES_NOT_CONTAIN_2_VALUES,
                     "'scale' must contain 2 values",
                     obj);
        } else if (!isNumber(scale[0]) || !isNumber(scale[1])) {
            addError(
                MessageCode::E_SCALE_VALUES_ARE_NOT_NUMERIC, "'scale' values must be numeric", obj);
        }
    }

    if (obj.contains("rotation")) {
        if (!isNumber(obj["rotation"])) {
            addError(MessageCode::E_ROTATION_IS_NOT_NUMERIC, "'rotation' must be numeric", obj);
        }
    }
}

void SceneValidator::checkColor(const nlohmann::json& obj) {
    if (!obj.contains("color")) {
        return;
    }

    const auto& color = obj["color"];

    if (!color.is_array()) {
        addError(MessageCode::E_COLOR_IS_NOT_AN_ARRAY, "'color' must be array", obj);
        return;
    } else if (color.size() != 4) {
        addError(
            MessageCode::E_COLOR_DOES_NOT_CONTAIN_4_VALUES, "'color' must contain 4 values", obj);
        return;
    }

    for (const auto& value : color) {
        if (!value.is_number_integer()) {
            addError(MessageCode::E_COLOR_VALUES_ARE_NOT_INTEGERS,
                     "'color' values must be integers",
                     obj);
            return;
        }

        const int channel = value.get<int>();

        if (channel < 0 || channel > 255) {
            addError(MessageCode::E_COLOR_VALUES_ARE_NOT_IN_RANGE,
                     "'color' values must be in range 0..255",
                     obj);
            return;
        }
    }
}

void SceneValidator::checkTextureFile(const nlohmann::json& obj) {
    if (!obj.contains("textureFile")) {
        return;
    }

    if (!obj["textureFile"].is_string()) {
        addError(MessageCode::E_TEXTURE_FILE_IS_NOT_A_STRING, "'textureFile' must be string", obj);
        return;
    }

    const auto path = obj["textureFile"].get<std::string>();

    if (!path.empty() && !std::filesystem::exists(scene_directory_ / path)) {
        addWarning(MessageCode::W_TEXTURE_FILE_NOT_FOUND,
                   "Texture file not found: " + (scene_directory_ / path).string(),
                   obj);
    }
}

void SceneValidator::checkLuaScript(const nlohmann::json& obj) {
    if (!obj.contains("luaScript")) {
        return;
    }

    if (!obj["luaScript"].is_string()) {
        addError(MessageCode::E_LUA_SCRIPT_IS_NOT_A_STRING, "'luaScript' must be string", obj);
        return;
    }

    const auto path = obj["luaScript"].get<std::string>();

    if (!path.empty() && !std::filesystem::exists(scene_directory_ / path)) {
        addWarning(MessageCode::W_LUA_SCRIPT_NOT_FOUND,
                   "Lua script file not found: " + (scene_directory_ / path).string(),
                   obj);
    }
}

void SceneValidator::checkTags(const nlohmann::json& obj) {
    if (!obj.contains("tags")) {
        return;
    }

    if (!obj["tags"].is_array()) {
        addError(MessageCode::E_TAGS_IS_NOT_AN_ARRAY, "'tags' must be array", obj);
        return;
    }

    for (const auto& tag : obj["tags"]) {
        if (!tag.is_string()) {
            addError(
                MessageCode::E_TAGS_VALUES_ARE_NOT_STRINGS, "'tags' values must be strings", obj);
            return;
        }
    }
}

void SceneValidator::checkProperties(const nlohmann::json& obj) {
    if (!obj.contains("properties")) {
        return;
    }

    if (!obj["properties"].is_object()) {
        addError(MessageCode::E_PROPERTIES_IS_NOT_AN_OBJECT, "'properties' must be object", obj);
    }
}

} // namespace dice::core