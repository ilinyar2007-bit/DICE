// TODO: validator for catching scene errors

#include "core/Model.hpp"

#include <algorithm>

#include <spdlog/spdlog.h>

namespace dice::core {

Model::Model(std::shared_ptr<IObjectFactory> factory) : factory_(std::move(factory)) {
    if (!factory_) {
        factory_ = std::make_shared<GameObjectFactory>();
    }
}

void Model::setFactory(std::shared_ptr<IObjectFactory> factory) {
    factory_ = std::move(factory);
    if (!factory_) {
        factory_ = std::make_shared<GameObjectFactory>();
    }
}

std::shared_ptr<GameObject> Model::getObject(const std::string& id) const {
    auto it = objects_.find(id);
    return (it == objects_.end()) ? nullptr : it->second;
}

void Model::clear() {
    roots_.clear();
    objects_.clear();
}

void Model::addRootObject(const std::shared_ptr<GameObject>& object) {
    if (!object || objects_.contains(object->getId())) {
        return;
    }
    roots_.push_back(object);
    registerRecursive(object);
    spdlog::debug("Model: added root object '{}'", object->getId());
}

bool Model::attachTo(const std::string& parent_id, const std::shared_ptr<GameObject>& object) {
    if (!object || objects_.contains(object->getId())) {
        return false;
    }

    auto parent = getObject(parent_id);
    if (!parent) {
        return false;
    }

    parent->addChild(object);
    registerRecursive(object);
    return true;
}

bool Model::removeObject(const std::string& id) {
    auto obj = getObject(id);
    if (!obj) {
        return false;
    }

    if (auto* parent = obj->getParent(); parent != nullptr) {
        parent->removeChild(id);
    } else {
        roots_.erase(std::remove_if(roots_.begin(),
                                    roots_.end(),
                                    [&](const std::shared_ptr<GameObject>& r) {
                                        return r && r->getId() == id;
                                    }),
                     roots_.end());
    }

    unregisterRecursive(obj);
    spdlog::debug("Model: removed object '{}'", id);
    return true;
}

void Model::registerRecursive(const std::shared_ptr<GameObject>& obj) { // NOLINT(misc-no-recursion)
    objects_[obj->getId()] = obj;
    for (const auto& child : obj->getChildren()) {
        registerRecursive(child);
    }
}

// NOLINTNEXTLINE(misc-no-recursion)
void Model::unregisterRecursive(const std::shared_ptr<GameObject>& obj) {
    objects_.erase(obj->getId());
    for (const auto& child : obj->getChildren()) {
        unregisterRecursive(child);
    }
}

void Model::forEachDepthFirstImpl( // NOLINT(misc-no-recursion)
    const std::shared_ptr<GameObject>& node,
    const std::function<void(const std::shared_ptr<GameObject>&)>& fn) {
    fn(node);
    for (const auto& ch : node->getChildren()) {
        forEachDepthFirstImpl(ch, fn);
    }
}

void Model::forEachDepthFirst(
    const std::function<void(const std::shared_ptr<GameObject>&)>& fn) const {
    for (const auto& r : roots_) {
        forEachDepthFirstImpl(r, fn);
    }
}

nlohmann::json Model::toJson() const {
    nlohmann::json j;
    j["objects"] = nlohmann::json::array();
    for (const auto& root : roots_) {
        j["objects"].push_back(root->toJson());
    }
    return j;
}

std::shared_ptr<GameObject>
Model::makeFromJsonNode(const nlohmann::json& node_json) { // NOLINT(misc-no-recursion)
    const std::string typeStr = node_json.value("type", "GameObject");
    const std::string id = node_json.value("id", "");
    const std::string name = node_json.value("name", "");

    auto obj = factory_->create(objectTypeFromString(typeStr), id, name);

    obj->fromJson(node_json);

    if (node_json.contains("children")) {
        for (const auto& childJson : node_json["children"]) {
            auto child = makeFromJsonNode(childJson);
            obj->addChild(child);
        }
    }

    return obj;
}

void Model::fromJson(const nlohmann::json& j) {
    clear();
    if (!j.contains("objects")) {
        return;
    }

    for (const auto& node : j.at("objects")) {
        auto root = makeFromJsonNode(node);
        addRootObject(root);
    }
    spdlog::info("Model: loaded {} root object(s) from JSON", roots_.size());
}

} // namespace dice::core
