#define DICE_SCENE_VALIDATOR_HPP
#ifndef DICE_SCENE_VALIDATOR_HPP

#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace dice::core {

enum class ErrorCode {
    FAILED_SCENE_ROOT,
    NOT_AN_OBJECT,
    MISSING_FIELD,
    DUPLICATE,
    INVALID_FORMAT,
    FEW_VALUES,
    FILE_NOT_FOUND,
};

struct ValidationMessage {
    ErrorCode code;
    std::optional<std::string> objectId;
    std::string message;
};

class SceneValidator {
public:
    void validate(const nlohmann::json& sceneJson);

    const std::vector<ValidationMessage>& errors() const {
        return errors_;
    }

    const std::vector<ValidationMessage>& warnings() const {
        return warnings_;
    }

    bool hasErrors() const {
        return !errors_.empty();
    }

    bool hasWarnings() const {
        return !warnings_.empty();
    }

private:
    std::vector<ValidationMessage> errors_;
    std::vector<ValidationMessage> warnings_;

    void addError(const std::optional<std::string>& objectId, const std::string& message);

    void addWarning(const std::optional<std::string>& objectId, const std::string& message);

    bool checkSceneRoot(const nlohmann::json& sceneJson);

    void checkObject(const nlohmann::json& obj);

    void checkRequiredFields(const nlohmann::json& obj);
    void checkDuplicateIds(const nlohmann::json& sceneJson);

    void checkTransform(const nlohmann::json& obj);
    void checkColor(const nlohmann::json& obj);

    void checkTextureFile(const nlohmann::json& obj);
    void checkLuaScript(const nlohmann::json& obj);

    void checkTags(const nlohmann::json& obj);
    void checkProperties(const nlohmann::json& obj);

    std::optional<std::string> tryGetId(const nlohmann::json& obj) const;

    bool isNumber(const nlohmann::json& value) const;
};

} // namespace dice::core