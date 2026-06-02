#ifndef DICE_DEFAULT_FACTORY_HPP
#define DICE_DEFAULT_FACTORY_HPP

#include <memory>

#include "components/Card.hpp"
#include "components/Chip.hpp"
#include "core/IObjectFactory.hpp"

namespace dice::scene {

class DefaultObjectFactory : public dice::core::IObjectFactory {
public:
    std::shared_ptr<dice::core::GameObject>
    create(dice::core::ObjectType type, const std::string& id, const std::string& name) override {
        switch (type) {
            case dice::core::ObjectType::CHIP:
                return std::make_shared<dice::components::Chip>(id, name);
            case dice::core::ObjectType::CARD:
                return std::make_shared<dice::components::Card>(id, name);
            default:
                return std::make_shared<dice::core::GameObject>(id, name);
        }
    }
};

inline std::shared_ptr<dice::core::IObjectFactory> makeDefaultFactory() {
    return std::make_shared<DefaultObjectFactory>();
}

} // namespace dice::scene

#endif // DICE_DEFAULT_FACTORY_HPP
