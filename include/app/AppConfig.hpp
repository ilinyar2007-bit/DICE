#ifndef DICE_APP_APPCONFIG_HPP
#define DICE_APP_APPCONFIG_HPP

#include <cstdint>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <spdlog/spdlog.h>

namespace dice {

struct FontEntry {
    std::string id;
    std::string path;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FontEntry, id, path)

struct AppConfig {
    int windowWidth = 1280;
    int windowHeight = 720;
    std::string title = "DICE";
    int framerateLimit = 60;

    uint8_t clearR = 30;
    uint8_t clearG = 30;
    uint8_t clearB = 40;

    std::string startScene = "scenes/demo.json";
    std::string globalScript;

    std::vector<FontEntry> fonts;

    bool showFPS = true;
    bool showObjectCount = true;
    bool showControls = true;
    bool resizable = true;

    int luaMemoryLimitMb = 64;
    int maxSceneObjects = 1000;
};

inline void from_json(const nlohmann::json& j, AppConfig& cfg) {
    cfg.windowWidth = j.value("windowWidth", cfg.windowWidth);
    cfg.windowHeight = j.value("windowHeight", cfg.windowHeight);
    cfg.title = j.value("title", cfg.title);
    cfg.framerateLimit = j.value("framerateLimit", cfg.framerateLimit);
    cfg.clearR = j.value("clearR", cfg.clearR);
    cfg.clearG = j.value("clearG", cfg.clearG);
    cfg.clearB = j.value("clearB", cfg.clearB);
    cfg.startScene = j.value("startScene", cfg.startScene);
    cfg.globalScript = j.value("globalScript", cfg.globalScript);
    cfg.fonts = j.value("fonts", cfg.fonts);
    cfg.showFPS = j.value("showFPS", cfg.showFPS);
    cfg.showObjectCount = j.value("showObjectCount", cfg.showObjectCount);
    cfg.showControls = j.value("showControls", cfg.showControls);
    cfg.resizable = j.value("resizable", cfg.resizable);
    cfg.luaMemoryLimitMb = j.value("luaMemoryLimitMb", cfg.luaMemoryLimitMb);
    cfg.maxSceneObjects = j.value("maxSceneObjects", cfg.maxSceneObjects);

    if (cfg.luaMemoryLimitMb <= 0) {
        spdlog::warn("AppConfig: luaMemoryLimitMb={} invalid, using default 64",
                     cfg.luaMemoryLimitMb);
        cfg.luaMemoryLimitMb = 64;
    }
    if (cfg.maxSceneObjects <= 0) {
        spdlog::warn("AppConfig: maxSceneObjects={} invalid, using default 1000",
                     cfg.maxSceneObjects);
        cfg.maxSceneObjects = 1000;
    }
}

inline void to_json(nlohmann::json& j, const AppConfig& cfg) {
    j = nlohmann::json{
        {"windowWidth", cfg.windowWidth},
        {"windowHeight", cfg.windowHeight},
        {"title", cfg.title},
        {"framerateLimit", cfg.framerateLimit},
        {"clearR", cfg.clearR},
        {"clearG", cfg.clearG},
        {"clearB", cfg.clearB},
        {"startScene", cfg.startScene},
        {"globalScript", cfg.globalScript},
        {"fonts", cfg.fonts},
        {"showFPS", cfg.showFPS},
        {"showObjectCount", cfg.showObjectCount},
        {"showControls", cfg.showControls},
        {"resizable", cfg.resizable},
        {"luaMemoryLimitMb", cfg.luaMemoryLimitMb},
        {"maxSceneObjects", cfg.maxSceneObjects},
    };
}

} // namespace dice

#endif // DICE_APP_APPCONFIG_HPP
