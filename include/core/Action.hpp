#ifndef DICE_ACTION_HPP
#define DICE_ACTION_HPP

#include <string>

#include <SFML/System/Vector2.hpp>

namespace dice::core {

class Model;

class MoveObjectAction {
public:
    MoveObjectAction() = default;
    MoveObjectAction(std::string object_id, sf::Vector2f new_pos);

    bool execute(Model& model);
    bool canExecute(const Model& model) const;

private:
    std::string objectId_;
    sf::Vector2f newPos_;
};

} // namespace dice::core

#endif
