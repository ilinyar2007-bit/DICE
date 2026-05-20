#include "app/ConfigLoader.hpp"

#include <fstream>

#include <spdlog/spdlog.h>

namespace dice {

AppConfig ConfigLoader::load(const std::filesystem::path& path) {
    AppConfig cfg;
    if (!std::filesystem::exists(path)) {
        spdlog::warn("Config file not found: {}, using defaults", path.string());
        return cfg;
    }

    try {
        std::ifstream file(path);
        nlohmann::json j;
        file >> j;
        cfg = j.get<AppConfig>();
        spdlog::info("Config loaded from {}", path.string());
    } catch (const std::exception& e) {
        spdlog::error("Failed to load config: {}", e.what());
    }

    return cfg;
}

} // namespace dice
