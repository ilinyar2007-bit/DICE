#ifndef DICE_MODEL_HPP
#define DICE_MODEL_HPP

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "core/GameObject.hpp"
#include "core/IObjectFactory.hpp"

namespace dice::core {

class Model final {
public:
    Model() = default;
    explicit Model(std::shared_ptr<IObjectFactory> factory);

    void setFactory(std::shared_ptr<IObjectFactory> factory);

    const std::vector<std::shared_ptr<GameObject>>& roots() const noexcept {
        return roots_;
    }

    std::shared_ptr<GameObject> getObject(const std::string& id) const;

    void addRootObject(const std::shared_ptr<GameObject>& object);
    bool attachTo(const std::string& parent_id, const std::shared_ptr<GameObject>& object);
    bool removeObject(const std::string& id);

    void forEachDepthFirst(const std::function<void(const std::shared_ptr<GameObject>&)>& fn) const;

    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);

    void clear();

    using UnregisterCallback = std::function<void(const std::string&)>;
    void setUnregisterCallback(UnregisterCallback cb) {
        unregisterCallback_ = std::move(cb);
    }

private:
    std::vector<std::shared_ptr<GameObject>> roots_;
    std::unordered_map<std::string, std::shared_ptr<GameObject>> objects_;
    std::shared_ptr<IObjectFactory> factory_;
    UnregisterCallback unregisterCallback_;

    void registerRecursive(const std::shared_ptr<GameObject>& obj);
    void unregisterRecursive(const std::shared_ptr<GameObject>& obj);

    static void
    forEachDepthFirstImpl(const std::shared_ptr<GameObject>& node,
                          const std::function<void(const std::shared_ptr<GameObject>&)>& fn);

    std::shared_ptr<GameObject> makeFromJsonNode(const nlohmann::json& node_json);
};

} // namespace dice::core

#endif // DICE_MODEL_HPP
