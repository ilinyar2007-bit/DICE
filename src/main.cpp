#include "app/Application.hpp"
#include <spdlog/spdlog.h>

int main() {
    spdlog::set_level(spdlog::level::info);

    try {
        dice::Application app;
        app.run();
    } catch (const std::exception& e) {
        spdlog::critical("Unhandled exception: {}", e.what());
        return 1;
    }

    return 0;
}
