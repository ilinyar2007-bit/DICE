#ifndef DICE_APP_APPCONFIG_HPP
#define DICE_APP_APPCONFIG_HPP

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace dice {

struct FontEntry {
    std::string id;
    std::string path;
};

struct AppConfig {
    int windowWidth   = 1280;
    int windowHeight  = 720;
    std::string title = "DICE";
    int framerateLimit = 60;

    uint8_t clearR = 30;
    uint8_t clearG = 30;
    uint8_t clearB = 40;

    std::string startScene   = "scenes/demo.json";
    std::string globalScript = "";

    std::vector<FontEntry> fonts;

    bool showFPS         = true;
    bool showObjectCount = true;
    bool showControls    = true;

    static AppConfig loadFromFile(const std::filesystem::path& path) {
        AppConfig cfg;

        std::ifstream file(path);
        if (!file.is_open()) {
            spdlog::warn("AppConfig: cannot open '{}', using defaults", path.string());
            return cfg;
        }

        try {
            nlohmann::json j;
            file >> j;

            if (j.contains("windowWidth"))    cfg.windowWidth    = j["windowWidth"];
            if (j.contains("windowHeight"))   cfg.windowHeight   = j["windowHeight"];
            if (j.contains("title"))          cfg.title          = j["title"];
            if (j.contains("framerateLimit")) cfg.framerateLimit = j["framerateLimit"];
            if (j.contains("clearR"))         cfg.clearR         = j["clearR"];
            if (j.contains("clearG"))         cfg.clearG         = j["clearG"];
            if (j.contains("clearB"))         cfg.clearB         = j["clearB"];
            if (j.contains("startScene"))     cfg.startScene     = j["startScene"];
            if (j.contains("globalScript"))   cfg.globalScript   = j["globalScript"];
            if (j.contains("showFPS"))        cfg.showFPS        = j["showFPS"];
            if (j.contains("showObjectCount")) cfg.showObjectCount = j["showObjectCount"];
            if (j.contains("showControls"))   cfg.showControls   = j["showControls"];

            if (j.contains("fonts") && j["fonts"].is_array()) {
                for (const auto& entry : j["fonts"]) {
                    FontEntry fe;
                    if (entry.contains("id"))   fe.id   = entry["id"];
                    if (entry.contains("path")) fe.path = entry["path"];
                    cfg.fonts.push_back(std::move(fe));
                }
            }

            spdlog::info("AppConfig loaded from '{}'", path.string());
        } catch (const std::exception& e) {
            spdlog::error("AppConfig: failed to parse '{}': {}", path.string(), e.what());
        }

        return cfg;
    }
};

} // namespace dice

#endif // DICE_APP_APPCONFIG_HPP
