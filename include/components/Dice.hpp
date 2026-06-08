#ifndef DICE_COMPONENTS_DICE_HPP
#define DICE_COMPONENTS_DICE_HPP

#include <string>
#include <vector>

#include "core/GameObject.hpp"

namespace dice::components {

class Dice : public core::GameObject {
public:
    Dice(const std::string& id, const std::string& name);

    void setFaceCount(int count) {
        faceCount_ = count;
    }
    int getFaceCount() const {
        return faceCount_;
    }

    void setValue(int value) {
        value_ = value;
    }
    int getValue() const {
        return value_;
    }

    void setFaceTextures(std::vector<std::string> textures) {
        faceTextures_ = std::move(textures);
    }
    const std::vector<std::string>& getFaceTextures() const {
        return faceTextures_;
    }

    const std::string& getFaceTexturePath(int value) const;

    int roll();

    nlohmann::json toJson() const override;
    void fromJson(const nlohmann::json& json) override;

private:
    int faceCount_ = 6;
    int value_ = 1;
    std::vector<std::string> faceTextures_;
};

} // namespace dice::components

#endif // DICE_COMPONENTS_DICE_HPP
