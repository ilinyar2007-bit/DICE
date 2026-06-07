#include "app/Application.hpp"
#include <spdlog/spdlog.h>

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::info);

    std::string startScene;
    if (argc > 1) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        startScene = argv[1];
    }

    try {
        dice::Application app;
        app.run(startScene);
    } catch (const std::exception& e) {
        spdlog::critical("Unhandled exception: {}", e.what());
        return 1;
    }

    return 0;
}
