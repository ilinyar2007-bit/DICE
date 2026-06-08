#ifndef DICE_DECK_HPP
#define DICE_DECK_HPP

#include "core/GameObject.hpp"

namespace dice::components {

class Deck : public core::GameObject {
public:
    Deck(const std::string& id, const std::string& name);

    // ================= State =================

    int count() const {
        return static_cast<int>(getChildren().size());
    }
    bool isEmpty() const {
        return getChildren().empty();
    }

    void setFaceDown(bool face_down);
    bool isFaceDown() const {
        return faceDown_;
    }

    // ================= Serialization =================

    nlohmann::json toJson() const override;
    void fromJson(const nlohmann::json& json) override;

private:
    bool faceDown_ = false;
};

} // namespace dice::components

#endif
