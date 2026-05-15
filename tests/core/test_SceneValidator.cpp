#include "core/SceneValidator.hpp"
#include <gtest/gtest.h>

using dice::core::MessageCode;
using dice::core::SceneValidator;

// ========== Helpers ==========

const std::filesystem::path test_path = "tests/core/test_file.json";

namespace {
nlohmann::json makeValidObject(const std::string& id = "obj_1",
                               const std::string& type = "GameObject") {
    return {{"id", id}, {"type", type}};
}

nlohmann::json makeValidScene(nlohmann::json objects = nlohmann::json::array()) {
    if (objects.empty()) {
        objects.push_back(makeValidObject());
    }
    return {{"objects", objects}};
}
} // namespace

// ========== Scene root checks ==========

TEST(SceneValidatorTest, ValidScenePassesValidation) {
    SceneValidator validator;
    EXPECT_TRUE(validator.validate(makeValidScene(), test_path));
    EXPECT_FALSE(validator.hasErrors());
    EXPECT_FALSE(validator.hasWarnings());
}

TEST(SceneValidatorTest, SceneRootIsNotObject) {
    SceneValidator validator;
    validator.validate(nlohmann::json::array(), test_path);
    EXPECT_TRUE(validator.hasErrors());
    EXPECT_EQ(validator.errors()[0].code_, MessageCode::E_SCENE_ROOT_IS_NOT_AN_OBJECT);
}

TEST(SceneValidatorTest, MissingObjectsArray) {
    SceneValidator validator;
    validator.validate(nlohmann::json::object(), test_path);
    EXPECT_TRUE(validator.hasErrors());
    EXPECT_EQ(validator.errors()[0].code_, MessageCode::E_MISSING_OBJECTS_ARRAY);
}

TEST(SceneValidatorTest, ObjectsIsNotArray) {
    SceneValidator validator;
    validator.validate({{"objects", "not_an_array"}}, test_path);
    EXPECT_TRUE(validator.hasErrors());
    EXPECT_EQ(validator.errors()[0].code_, MessageCode::E_OBJECTS_IS_NOT_AN_ARRAY);
}

// ========== Required fields ==========

TEST(SceneValidatorTest, MissingId) {
    SceneValidator validator;
    auto scene = makeValidScene({{{"type", "GameObject"}}});
    validator.validate(scene, test_path);

    EXPECT_TRUE(validator.hasErrors());

    bool found = false;
    for (const auto& e : validator.errors()) {
        if (e.code_ == MessageCode::E_MISSING_FIELD_ID) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST(SceneValidatorTest, IdIsNotString) {
    SceneValidator validator;
    auto scene = makeValidScene({{{"id", 123}, {"type", "GameObject"}}});
    validator.validate(scene, test_path);

    EXPECT_TRUE(validator.hasErrors());

    bool found = false;
    for (const auto& e : validator.errors()) {
        if (e.code_ == MessageCode::E_ID_IS_NOT_A_STRING) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST(SceneValidatorTest, MissingType) {
    SceneValidator validator;
    auto scene = makeValidScene({{{"id", "obj_1"}}});
    validator.validate(scene, test_path);
    EXPECT_TRUE(validator.hasErrors());
    EXPECT_EQ(validator.errors()[0].code_, MessageCode::E_MISSING_FIELD_TYPE);
}

TEST(SceneValidatorTest, TypeIsNotString) {
    SceneValidator validator;
    auto scene = makeValidScene({{{"id", "obj_1"}, {"type", 123}}});
    validator.validate(scene, test_path);
    EXPECT_TRUE(validator.hasErrors());
    EXPECT_EQ(validator.errors()[0].code_, MessageCode::E_TYPE_IS_NOT_A_STRING);
}

// ========== Duplicate IDs ==========

TEST(SceneValidatorTest, DuplicateId) {
    SceneValidator validator;
    auto scene = makeValidScene({makeValidObject("obj_1"), makeValidObject("obj_1")});
    validator.validate(scene, test_path);
    EXPECT_TRUE(validator.hasErrors());
    bool hasDuplicate = false;
    for (const auto& e : validator.errors()) {
        if (e.code_ == MessageCode::E_DUPLICATE_ID) {
            hasDuplicate = true;
        }
    }
    EXPECT_TRUE(hasDuplicate);
}

TEST(SceneValidatorTest, NoDuplicateIds) {
    SceneValidator validator;
    auto scene = makeValidScene({makeValidObject("obj_1"), makeValidObject("obj_2")});
    EXPECT_TRUE(validator.validate(scene, test_path));
}

// ========== Transform ==========

TEST(SceneValidatorTest, ValidPosition) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["position"] = {100.0, 200.0};
    EXPECT_TRUE(validator.validate(makeValidScene({obj}), test_path));
}

TEST(SceneValidatorTest, PositionIsNotArray) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["position"] = "100,200";
    validator.validate(makeValidScene({obj}), test_path);
    EXPECT_TRUE(validator.hasErrors());
    EXPECT_EQ(validator.errors()[0].code_, MessageCode::E_POSITION_IS_NOT_AN_ARRAY);
}

TEST(SceneValidatorTest, PositionDoesNotContain2Values) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["position"] = {100.0};
    validator.validate(makeValidScene({obj}), test_path);
    EXPECT_TRUE(validator.hasErrors());
    EXPECT_EQ(validator.errors()[0].code_, MessageCode::E_POSITION_DOES_NOT_CONTAIN_2_VALUES);
}

TEST(SceneValidatorTest, PositionValuesAreNotNumeric) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["position"] = {"x", "y"};
    validator.validate(makeValidScene({obj}), test_path);
    EXPECT_TRUE(validator.hasErrors());
    EXPECT_EQ(validator.errors()[0].code_, MessageCode::E_POSITION_VALUES_ARE_NOT_NUMERIC);
}

TEST(SceneValidatorTest, ValidScale) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["scale"] = {1.0, 1.0};
    EXPECT_TRUE(validator.validate(makeValidScene({obj}), test_path));
}

TEST(SceneValidatorTest, ScaleIsNotArray) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["scale"] = 1.0;
    validator.validate(makeValidScene({obj}), test_path);
    EXPECT_TRUE(validator.hasErrors());
    EXPECT_EQ(validator.errors()[0].code_, MessageCode::E_SCALE_IS_NOT_AN_ARRAY);
}

TEST(SceneValidatorTest, ScaleDoesNotContain2Values) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["scale"] = {1.0, 1.0, 1.0};
    validator.validate(makeValidScene({obj}), test_path);
    EXPECT_TRUE(validator.hasErrors());
    EXPECT_EQ(validator.errors()[0].code_, MessageCode::E_SCALE_DOES_NOT_CONTAIN_2_VALUES);
}

TEST(SceneValidatorTest, ScaleValuesAreNotNumeric) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["scale"] = {"a", "b"};
    validator.validate(makeValidScene({obj}), test_path);
    EXPECT_TRUE(validator.hasErrors());
    EXPECT_EQ(validator.errors()[0].code_, MessageCode::E_SCALE_VALUES_ARE_NOT_NUMERIC);
}

TEST(SceneValidatorTest, ValidRotation) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["rotation"] = 45.0;
    EXPECT_TRUE(validator.validate(makeValidScene({obj}), test_path));
}

TEST(SceneValidatorTest, RotationIsNotNumeric) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["rotation"] = "fast";
    validator.validate(makeValidScene({obj}), test_path);
    EXPECT_TRUE(validator.hasErrors());
    EXPECT_EQ(validator.errors()[0].code_, MessageCode::E_ROTATION_IS_NOT_NUMERIC);
}

// ========== Color ==========

TEST(SceneValidatorTest, ValidColor) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["color"] = {255, 0, 128, 255};
    EXPECT_TRUE(validator.validate(makeValidScene({obj}), test_path));
}

TEST(SceneValidatorTest, ColorIsNotArray) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["color"] = "red";
    validator.validate(makeValidScene({obj}), test_path);
    EXPECT_TRUE(validator.hasErrors());
    EXPECT_EQ(validator.errors()[0].code_, MessageCode::E_COLOR_IS_NOT_AN_ARRAY);
}

TEST(SceneValidatorTest, ColorDoesNotContain4Values) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["color"] = {255, 0, 128};
    validator.validate(makeValidScene({obj}), test_path);
    EXPECT_TRUE(validator.hasErrors());
    EXPECT_EQ(validator.errors()[0].code_, MessageCode::E_COLOR_DOES_NOT_CONTAIN_4_VALUES);
}

TEST(SceneValidatorTest, ColorValuesAreNotIntegers) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["color"] = {1.5, 0.0, 0.5, 1.0};
    validator.validate(makeValidScene({obj}), test_path);
    EXPECT_TRUE(validator.hasErrors());
    EXPECT_EQ(validator.errors()[0].code_, MessageCode::E_COLOR_VALUES_ARE_NOT_INTEGERS);
}

TEST(SceneValidatorTest, ColorValueBelowRange) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["color"] = {-1, 0, 0, 255};
    validator.validate(makeValidScene({obj}), test_path);
    EXPECT_TRUE(validator.hasErrors());
    EXPECT_EQ(validator.errors()[0].code_, MessageCode::E_COLOR_VALUES_ARE_NOT_IN_RANGE);
}

TEST(SceneValidatorTest, ColorValueAboveRange) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["color"] = {256, 0, 0, 255};
    validator.validate(makeValidScene({obj}), test_path);
    EXPECT_TRUE(validator.hasErrors());
    EXPECT_EQ(validator.errors()[0].code_, MessageCode::E_COLOR_VALUES_ARE_NOT_IN_RANGE);
}

TEST(SceneValidatorTest, ColorBoundaryValuesAreValid) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["color"] = {0, 0, 0, 255};
    EXPECT_TRUE(validator.validate(makeValidScene({obj}), test_path));
}

// ========== TextureFile ==========

TEST(SceneValidatorTest, TextureFileIsNotString) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["textureFile"] = 42;
    validator.validate(makeValidScene({obj}), test_path);
    EXPECT_TRUE(validator.hasErrors());
    EXPECT_EQ(validator.errors()[0].code_, MessageCode::E_TEXTURE_FILE_IS_NOT_A_STRING);
}

TEST(SceneValidatorTest, TextureFileNotFoundProducesWarning) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["textureFile"] = "nonexistent/texture.png";
    validator.validate(makeValidScene({obj}), test_path);
    EXPECT_FALSE(validator.hasErrors());
    EXPECT_TRUE(validator.hasWarnings());
    EXPECT_EQ(validator.warnings()[0].code_, MessageCode::W_TEXTURE_FILE_NOT_FOUND);
}

TEST(SceneValidatorTest, EmptyTextureFileSkipsExistenceCheck) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["textureFile"] = "";
    EXPECT_TRUE(validator.validate(makeValidScene({obj}), test_path));
    EXPECT_FALSE(validator.hasWarnings());
}

// ========== LuaScript ==========

TEST(SceneValidatorTest, LuaScriptIsNotString) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["luaScript"] = 99;
    validator.validate(makeValidScene({obj}), test_path);
    EXPECT_TRUE(validator.hasErrors());
    EXPECT_EQ(validator.errors()[0].code_, MessageCode::E_LUA_SCRIPT_IS_NOT_A_STRING);
}

TEST(SceneValidatorTest, LuaScriptNotFoundProducesWarning) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["luaScript"] = "nonexistent/script.lua";
    validator.validate(makeValidScene({obj}), test_path);
    EXPECT_FALSE(validator.hasErrors());
    EXPECT_TRUE(validator.hasWarnings());
    EXPECT_EQ(validator.warnings()[0].code_, MessageCode::W_LUA_SCRIPT_NOT_FOUND);
}

TEST(SceneValidatorTest, EmptyLuaScriptSkipsExistenceCheck) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["luaScript"] = "";
    EXPECT_TRUE(validator.validate(makeValidScene({obj}), test_path));
    EXPECT_FALSE(validator.hasWarnings());
}

// ========== Tags ==========

TEST(SceneValidatorTest, ValidTags) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["tags"] = {"movable", "player1"};
    EXPECT_TRUE(validator.validate(makeValidScene({obj}), test_path));
}

TEST(SceneValidatorTest, TagsIsNotArray) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["tags"] = "movable";
    validator.validate(makeValidScene({obj}), test_path);
    EXPECT_TRUE(validator.hasErrors());
    EXPECT_EQ(validator.errors()[0].code_, MessageCode::E_TAGS_IS_NOT_AN_ARRAY);
}

TEST(SceneValidatorTest, TagsValuesAreNotStrings) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["tags"] = {1, 2, 3};
    validator.validate(makeValidScene({obj}), test_path);
    EXPECT_TRUE(validator.hasErrors());
    EXPECT_EQ(validator.errors()[0].code_, MessageCode::E_TAGS_VALUES_ARE_NOT_STRINGS);
}

// ========== Properties ==========

TEST(SceneValidatorTest, ValidProperties) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["properties"] = {{"health", 100}, {"owner", "player1"}};
    EXPECT_TRUE(validator.validate(makeValidScene({obj}), test_path));
}

TEST(SceneValidatorTest, PropertiesIsNotObject) {
    SceneValidator validator;
    auto obj = makeValidObject();
    obj["properties"] = nlohmann::json::array({"a", "b"});
    validator.validate(makeValidScene({obj}), test_path);
    EXPECT_TRUE(validator.hasErrors());
    EXPECT_EQ(validator.errors()[0].code_, MessageCode::E_PROPERTIES_IS_NOT_AN_OBJECT);
}

// ========== State reset ==========

TEST(SceneValidatorTest, ValidatorClearsStateBetweenValidations) {
    SceneValidator validator;

    validator.validate({{"objects", "not_an_array"}}, test_path);
    EXPECT_TRUE(validator.hasErrors());

    EXPECT_TRUE(validator.validate(makeValidScene(), test_path));
    EXPECT_FALSE(validator.hasErrors());
}

// ========== Multiple errors ==========

TEST(SceneValidatorTest, MultipleErrorsCollected) {
    SceneValidator validator;
    auto scene = makeValidScene({
        {{"id", "obj_1"}, {"type", "GameObject"}, {"rotation", "fast"}, {"tags", "bad"}},
    });
    validator.validate(scene, test_path);
    EXPECT_GE(validator.errors().size(), 2U);
}