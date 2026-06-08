#ifndef DICE_DICE_HPP
#define DICE_DICE_HPP

#include "core/GameObject.hpp"

namespace dice::components {

class Dice : public core::GameObject {
public:
    Dice(const std::string& id, const std::string& name);

    // ================= State =================

    void setValue(int value) {
        value_ = value;
    }
    int getValue() const {
        return value_;
    }

    void setAssetId(const std::string& asset_id) {
        asset_id_ = asset_id;
    }
    const std::string& getAssetId() const {
        return asset_id_;
    }

    // ================= Serialization =================

    nlohmann::json toJson() const override;
    void fromJson(const nlohmann::json& json) override;

private:
    int value_ = 0;
    std::string asset_id_;
};

} // namespace dice::components

#endif
