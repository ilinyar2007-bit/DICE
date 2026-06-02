#ifndef DICE_RESOURCE_MANAGER_HPP
#define DICE_RESOURCE_MANAGER_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>

#include <spdlog/spdlog.h>

namespace dice::core {

template <typename Resource> class ResourceManager {
public:
    ResourceManager() = default;
    ~ResourceManager() = default;

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&) = delete;
    ResourceManager& operator=(ResourceManager&&) = delete;

    // ========== Load by filename ==========
    std::shared_ptr<Resource> load(const std::string& id, const std::string& filename) {
        if (auto it = resources_.find(id); it != resources_.end()) {
            if (it->second.first == filename) {
                return it->second.second;
            }

            spdlog::error(
                "ResourceManager: failed to load file from '{}' by assigning existing id - '{}'",
                filename,
                id);

            return fallback_;
        }

        auto res = std::make_shared<Resource>();

        if (!res->loadFromFile(filename)) {
            spdlog::error("ResourceManager: failed to load from '{}'", filename);

            return fallback_;
        }

        resources_.emplace(id, std::make_pair(filename, res));
        return res;
    }

    // ========== Create resource object ==========
    std::shared_ptr<Resource> create(const std::string& id) {
        if (auto it = resources_.find(id); it != resources_.end()) {
            spdlog::error("ResourceManager: failed to create a new resource object by assigning "
                          "existing id - '{}'",
                          id);

            return fallback_;
        }

        auto res = std::make_shared<Resource>();
        resources_.emplace(id, std::make_pair("", res));
        return res;
    }

    // ========== Get resource by id ==========
    std::shared_ptr<Resource> get(const std::string& id) const {
        if (auto it = resources_.find(id); it != resources_.end()) {
            return it->second.second;
        }

        spdlog::warn("ResourceManager: resource '{}' not found", id);

        return fallback_;
    }

    // ========== Remove resource ==========
    void unload(const std::string& id) {
        resources_.erase(id);
    };

    // ========== Clear all resources ==========
    void clear() {
        resources_.clear();
    };

    bool isEmpty() const {
        return resources_.empty();
    }

    // ========== Set fallback resource ==========
    void setFallback(std::shared_ptr<Resource> fallback) {
        fallback_ = std::move(fallback);
    }

    // ========== Serialization ==========

    [[nodiscard]] nlohmann::json toJson() const {
        nlohmann::json json;
        json["resources"] = nlohmann::json::object();

        for (const auto& [id, pair] : resources_) {
            json["resources"][id] = pair.first;
        }

        return json;
    }

    void fromJson(const nlohmann::json& json) {
        resources_.clear();

        if (!json.contains("resources")) {
            return;
        }

        for (const auto& [id, value] : json["resources"].items()) {
            std::string filename;
            value.get_to(filename);

            load(id, filename);
        }
    }

private:
    // id : { filename, shared_ptr<Resource> }
    std::unordered_map<std::string, std::pair<std::string, std::shared_ptr<Resource>>> resources_{};
    std::shared_ptr<Resource> fallback_ = nullptr;
};

} // namespace dice::core

#endif // DICE_RESOURCE_MANAGER_HPP
