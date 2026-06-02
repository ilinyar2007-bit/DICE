#ifndef DICE_APP_APPCONFIG_HPP
#define DICE_APP_APPCONFIG_HPP

#include <cstdint>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

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
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AppConfig,
                                   windowWidth,
                                   windowHeight,
                                   title,
                                   framerateLimit,
                                   clearR,
                                   clearG,
                                   clearB,
                                   startScene,
                                   globalScript,
                                   fonts,
                                   showFPS,
                                   showObjectCount,
                                   showControls,
                                   resizable)

} // namespace dice

#endif // DICE_APP_APPCONFIG_HPP
