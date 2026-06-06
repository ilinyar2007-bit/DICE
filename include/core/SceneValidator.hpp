#ifndef DICE_SCENE_VALIDATOR_HPP
#define DICE_SCENE_VALIDATOR_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace dice::core {

enum class MessageCode : std::uint8_t {

    E_SCENE_ROOT_IS_NOT_AN_OBJECT,
    E_MISSING_OBJECTS_ARRAY,
    E_OBJECTS_IS_NOT_AN_ARRAY,

    E_OBJECT_ENTRY_IS_NOT_AN_OBJECT,

    E_MISSING_FIELD_ID,
    E_ID_IS_NOT_A_STRING,

    E_MISSING_FIELD_TYPE,
    E_TYPE_IS_NOT_A_STRING,

    E_DUPLICATE_ID,

    E_POSITION_IS_NOT_AN_ARRAY,
    E_POSITION_DOES_NOT_CONTAIN_2_VALUES,
    E_POSITION_VALUES_ARE_NOT_NUMERIC,

    E_SCALE_IS_NOT_AN_ARRAY,
    E_SCALE_DOES_NOT_CONTAIN_2_VALUES,
    E_SCALE_VALUES_ARE_NOT_NUMERIC,

    E_ROTATION_IS_NOT_NUMERIC,

    E_COLOR_IS_NOT_AN_ARRAY,
    E_COLOR_DOES_NOT_CONTAIN_4_VALUES,
    E_COLOR_VALUES_ARE_NOT_INTEGERS,
    E_COLOR_VALUES_ARE_NOT_IN_RANGE,

    E_TEXTURE_FILE_IS_NOT_A_STRING,
    W_TEXTURE_FILE_NOT_FOUND,

    E_LUA_SCRIPT_IS_NOT_A_STRING,
    W_LUA_SCRIPT_NOT_FOUND,

    E_TAGS_IS_NOT_AN_ARRAY,
    E_TAGS_VALUES_ARE_NOT_STRINGS,

    E_PROPERTIES_IS_NOT_AN_OBJECT,

    E_SCRIPTS_IS_NOT_AN_ARRAY,
    E_SCRIPTS_ENTRY_IS_NOT_A_STRING,
    W_SCRIPT_FILE_NOT_FOUND,
    E_TRIGGERS_IS_NOT_AN_OBJECT,
    E_TRIGGER_VALUE_IS_NOT_A_STRING,
    E_OBJECT_COUNT_LIMIT_EXCEEDED,

};

struct ValidationMessage {
    MessageCode code_;
    std::string message_;
    nlohmann::json context_object_;
};

class SceneValidator {
public:
    explicit SceneValidator(std::size_t max_objects = 1000);

    bool validate(const nlohmann::json& scene_json, const std::filesystem::path& scene_file_path);

    [[nodiscard]] const std::vector<ValidationMessage>& errors() const {
        return errors_;
    }

    [[nodiscard]] const std::vector<ValidationMessage>& warnings() const {
        return warnings_;
    }

    [[nodiscard]] bool hasErrors() const {
        return !errors_.empty();
    }

    [[nodiscard]] bool hasWarnings() const {
        return !warnings_.empty();
    }

private:
    std::size_t maxObjectCount_;
    std::vector<ValidationMessage> errors_;
    std::vector<ValidationMessage> warnings_;
    std::filesystem::path scene_directory_;

    void clear() {
        warnings_.clear();
        errors_.clear();
    }

    void addError(const MessageCode& code,
                  const std::string& message,
                  const nlohmann::json& context_object);

    void addWarning(const MessageCode& code,
                    const std::string& message,
                    const nlohmann::json& context_object);

    void checkSceneRoot(const nlohmann::json& scene_json);

    void checkObjectCount(const nlohmann::json& scene_json);
    void checkObject(const nlohmann::json& obj);

    void checkRequiredFields(const nlohmann::json& obj);
    void checkDuplicateIds(const nlohmann::json& scene_json);

    void checkTransform(const nlohmann::json& obj);
    void checkColor(const nlohmann::json& obj);

    void checkTextureFile(const nlohmann::json& obj);
    void checkLuaScript(const nlohmann::json& obj);

    void checkTags(const nlohmann::json& obj);
    void checkProperties(const nlohmann::json& obj);

    void checkScripts(const nlohmann::json& scene_json);
    void checkTriggers(const nlohmann::json& obj);

    [[nodiscard]] static std::optional<std::string> tryGetId(const nlohmann::json& obj) {
        if (!obj.contains("id")) {
            return std::nullopt;
        }

        if (!obj["id"].is_string()) {
            return std::nullopt;
        }

        return obj["id"].get<std::string>();
    }

    [[nodiscard]] static bool isNumber(const nlohmann::json& value) {
        return value.is_number_integer() || value.is_number_unsigned() || value.is_number_float();
    }
};

} // namespace dice::core

#endif // DICE_SCENE_VALIDATOR_HPP
