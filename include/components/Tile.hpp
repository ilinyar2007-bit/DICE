#ifndef DICE_TILE_HPP
#define DICE_TILE_HPP

#include <string>
#include <vector>

#include "core/GameObject.hpp"

namespace dice::components {

class Tile : public core::GameObject {
public:
    Tile(const std::string& id, const std::string& name);

    // ================= Grid position =================

    void setCol(int col) {
        col_ = col;
    }
    int getCol() const {
        return col_;
    }

    void setRow(int row) {
        row_ = row;
    }
    int getRow() const {
        return row_;
    }

    // ================= Occupant =================

    void setOccupant(const std::string& id) {
        occupantId_ = id;
    }
    void clearOccupant() {
        occupantId_.clear();
    }
    const std::string& getOccupantId() const {
        return occupantId_;
    }
    bool isOccupied() const {
        return !occupantId_.empty();
    }

    // ================= Type filter =================

    void setAcceptedTypes(std::vector<std::string> types) {
        acceptedTypes_ = std::move(types);
    }
    const std::vector<std::string>& getAcceptedTypes() const {
        return acceptedTypes_;
    }
    bool accepts(const std::string& type) const;

    // ================= Serialization =================

    nlohmann::json toJson() const override;
    void fromJson(const nlohmann::json& json) override;

private:
    int col_ = 0;
    int row_ = 0;
    std::string occupantId_;
    std::vector<std::string> acceptedTypes_;
};

} // namespace dice::components

#endif
