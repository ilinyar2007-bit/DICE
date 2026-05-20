#ifndef DICE_APP_CONFIGLOADER_HPP
#define DICE_APP_CONFIGLOADER_HPP

#include <filesystem>

#include "app/AppConfig.hpp"

namespace dice {

class ConfigLoader {
public:
    static AppConfig load(const std::filesystem::path& path);
};

} // namespace dice

#endif // DICE_APP_CONFIGLOADER_HPP
