#include <algorithm>
#include <array>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "core/GameObject.hpp"
#include "core/Model.hpp"
#include "core/ResourceManager.hpp"
#include "scene/DefaultFactory.hpp"
#include "scripting/LuaScriptEngine.hpp"
#include "ui/View.hpp"

using dice::core::GameObject;
using dice::core::Model;
using dice::core::ResourceManager;
using dice::scene::makeDefaultFactory;
using dice::scripting::LuaScriptEngine;
using dice::view::View;
using dice::view::ViewConfig;

// ─── Procedural asset generation ──────────────────────────────────────────────

namespace {

sf::Image makeBoard(int w, int h) {
    sf::Image img;
    img.create(w, h, sf::Color(40, 130, 40));
    for (int x = 0; x < w; x++)
        for (int y = 0; y < h; y++)
            if (x % 50 == 0 || y % 50 == 0)
                img.setPixel(x, y, sf::Color(30, 100, 30));
    for (int x = 0; x < w; x++) {
        img.setPixel(x, 0,     sf::Color(20, 70, 20));
        img.setPixel(x, h - 1, sf::Color(20, 70, 20));
    }
    for (int y = 0; y < h; y++) {
        img.setPixel(0,     y, sf::Color(20, 70, 20));
        img.setPixel(w - 1, y, sf::Color(20, 70, 20));
    }
    return img;
}

sf::Image makeButton(int w, int h) {
    sf::Image img;
    img.create(w, h, sf::Color(220, 220, 220));
    for (int x = 0; x < w; x++) {
        img.setPixel(x, 0,     sf::Color(140, 140, 140));
        img.setPixel(x, h - 1, sf::Color(140, 140, 140));
    }
    for (int y = 0; y < h; y++) {
        img.setPixel(0,     y, sf::Color(140, 140, 140));
        img.setPixel(w - 1, y, sf::Color(140, 140, 140));
    }
    return img;
}

void generateAssets() {
    namespace fs = std::filesystem;
    fs::create_directories("assets");
    auto save = [](const sf::Image& img, const std::string& path) {
        if (!std::filesystem::exists(path))
            img.saveToFile(path);
    };
    save(makeBoard(100, 100),   "assets/board.png");
    save(makeButton(100, 100),  "assets/button.png");
}

} // namespace

// ======== Game state ========

struct GameState {
    int  currentPlayer = 1;
    int  scores[2]     = {0, 0};
    int  diceRoll[2]   = {0, 0}; 
    bool hasRolled     = false;
    bool gameOver      = false;
    int  winner        = 0;
    const int targetScore = 21;
};

// ======== UI helpers ========

static void drawCenteredText(sf::RenderWindow& win, const sf::Font& font,
                             const std::string& str, float cx, float cy,
                             unsigned sz, sf::Color fill, float outline = 1.5f) {
    sf::Text t;
    t.setFont(font);
    t.setString(sf::String::fromUtf8(str.begin(), str.end()));
    t.setCharacterSize(sz);
    t.setFillColor(fill);
    t.setOutlineColor(sf::Color::Black);
    t.setOutlineThickness(outline);
    const auto lb = t.getLocalBounds();
    t.setOrigin(lb.left + lb.width / 2.f, lb.top + lb.height / 2.f);
    t.setPosition(cx, cy);
    win.draw(t);
}

static void drawLeftText(sf::RenderWindow& win, const sf::Font& font,
                         const std::string& str, float x, float y,
                         unsigned sz, sf::Color fill, float outline = 1.5f) {
    sf::Text t;
    t.setFont(font);
    t.setString(sf::String::fromUtf8(str.begin(), str.end()));
    t.setCharacterSize(sz);
    t.setFillColor(fill);
    t.setOutlineColor(sf::Color::Black);
    t.setOutlineThickness(outline);
    t.setPosition(x, y);
    win.draw(t);
}

// ======== Entry point ========

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    spdlog::set_level(spdlog::level::info);
    spdlog::info("DICE — turn-based dice game starting");

    generateAssets();

    sf::RenderWindow window(sf::VideoMode(1280, 720), "DICE — игра в кости");
    window.setFramerateLimit(60);

    // Fallback texture
    auto baseTex = std::make_shared<sf::Texture>();
    {
        sf::Image img;
        img.create(100, 100, sf::Color::White);
        baseTex->loadFromImage(img);
    }

    ResourceManager<sf::Texture> textures;
    textures.setFallback(baseTex);

    // ======== View ========
    View view(window);
    ViewConfig vcfg;
    vcfg.backgroundColor = sf::Color(20, 20, 30);
    vcfg.showFPS         = false;
    vcfg.showObjectCount = false;
    vcfg.showControls    = false;
    view.setConfig(vcfg);

    // ======== Game state ========
    GameState gs;

    // ======== Lua engine ========
    LuaScriptEngine luaEngine;

    luaEngine.registerFunction("cpp_roll_dice", [&gs](int player) -> int {
        if (player < 1 || player > 2) return 0;
        if (gs.gameOver || gs.hasRolled || gs.currentPlayer != player) {
            spdlog::info("[Dice] Roll ignored (turn=P{} player=P{} rolled={})",
                         gs.currentPlayer, player, gs.hasRolled);
            return 0;
        }
        const int roll = (std::rand() % 6) + 1;
        gs.diceRoll[player - 1]  = roll;
        gs.scores[player - 1]   += roll;
        gs.hasRolled              = true;
        spdlog::info("[Dice] Player {} rolled {} — total: {}", player, roll, gs.scores[player - 1]);
        if (gs.scores[player - 1] >= gs.targetScore) {
            gs.gameOver = true;
            gs.winner   = player;
            spdlog::info("[Game] Player {} wins!", player);
        }
        return roll;
    });

    luaEngine.registerFunction("cpp_end_turn", [&gs]() {
        if (gs.gameOver || !gs.hasRolled) return;
        gs.currentPlayer = (gs.currentPlayer == 1) ? 2 : 1;
        gs.hasRolled     = false;
        spdlog::info("[Game] Turn passed to Player {}", gs.currentPlayer);
    });

    // ======== Load scene ========
    Model model(makeDefaultFactory());
    {
        std::ifstream file("scenes/demo.json");
        if (!file.is_open()) {
            spdlog::error("Cannot open scenes/demo.json — run from project root!");
            return 1;
        }
        model.fromJson(nlohmann::json::parse(file));
        spdlog::info("Scene loaded");
    }

    // Assign textures and attach Lua scripts
    std::unordered_set<std::string> loadedIds;
    model.forEachDepthFirst([&](const std::shared_ptr<GameObject>& obj) {
        const std::string& tf = obj->getTextureFile();
        if (!tf.empty()) {
            if (!loadedIds.count(tf)) { textures.load(tf, tf); loadedIds.insert(tf); }
            obj->setTexture(textures.get(tf).get());
        } else {
            obj->setTexture(baseTex.get());
        }
        if (!obj->getLuaScript().empty())
            luaEngine.attachScript(*obj);
    });

    // Load all 6 faces for each die
    std::array<std::shared_ptr<sf::Texture>, 6> whiteFaces, redFaces;
    for (int i = 1; i <= 6; i++) {
        const std::string wp = "assets/dieWhite_border" + std::to_string(i) + ".png";
        const std::string rp = "assets/dieRed_border"   + std::to_string(i) + ".png";
        whiteFaces[i - 1] = textures.load("white_" + std::to_string(i), wp);
        redFaces  [i - 1] = textures.load("red_"   + std::to_string(i), rp);
    }

    // Field bounds
    sf::FloatRect fieldBounds{0.f, 0.f, 1280.f, 720.f};
    if (auto board = model.getObject("board"))
        fieldBounds = board->getGlobalBounds();

    // ======== Font ========
    sf::Font font;
    const bool fontOk = font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");

    // ======== Drag state ========
    std::shared_ptr<GameObject> draggedObj;
    sf::Vector2f dragOffset;
    bool  wasDragging = false;
    float chipHalfW   = 0.f;
    float chipHalfH   = 0.f;

    sf::Clock clock;

    while (window.isOpen()) {
        const float deltaTime = clock.restart().asSeconds();

        std::vector<std::shared_ptr<GameObject>> allObjects;
        model.forEachDepthFirst([&](const std::shared_ptr<GameObject>& obj) {
            allObjects.push_back(obj);
        });

        // ======== Update die textures and tinting ========
        auto updateDie = [&](const std::string& id, int player,
                             std::array<std::shared_ptr<sf::Texture>, 6>& faces) {
            auto d = model.getObject(id);
            if (!d) return;
            const int face = std::max(0, gs.diceRoll[player - 1] - 1);
            if (faces[face]) d->setTexture(faces[face].get());
            const bool active = (gs.currentPlayer == player) && !gs.hasRolled && !gs.gameOver;
            d->setColor(active ? sf::Color::White : sf::Color(160, 150, 130, 180));
        };
        updateDie("dice_1", 1, whiteFaces);
        updateDie("dice_2", 2, redFaces);

        if (auto btn = model.getObject("end_turn_btn")) {
            if (gs.gameOver)        btn->setColor(sf::Color(80,  80,  80,  180));
            else if (gs.hasRolled)  btn->setColor(sf::Color(80,  220, 100, 255));
            else                    btn->setColor(sf::Color(150, 150, 150, 200));
        }

        // ======== Events ========
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
            if (event.type == sf::Event::KeyPressed &&
                event.key.code == sf::Keyboard::Escape) window.close();

            if (event.type == sf::Event::MouseButtonPressed &&
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
                event.mouseButton.button == sf::Mouse::Left) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
                const auto wp = view.screenToWorld({event.mouseButton.x, event.mouseButton.y});
                auto picked = view.pickObject(wp, allObjects);
                if (picked && picked->isDraggable()) {
                    draggedObj  = picked;
                    dragOffset  = picked->getPosition() - wp;
                    wasDragging = false;
                    const auto b = picked->getGlobalBounds();
                    chipHalfW = b.width  / 2.f;
                    chipHalfH = b.height / 2.f;
                } else if (picked) {
                    luaEngine.fireEvent("on_click", picked.get());
                }
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
            if (event.type == sf::Event::MouseMoved && draggedObj) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
                const auto wp = view.screenToWorld({event.mouseMove.x, event.mouseMove.y});
                sf::Vector2f newPos = wp + dragOffset;
                newPos.x = std::clamp(newPos.x, fieldBounds.left + chipHalfW,
                                      fieldBounds.left + fieldBounds.width  - chipHalfW);
                newPos.y = std::clamp(newPos.y, fieldBounds.top  + chipHalfH,
                                      fieldBounds.top  + fieldBounds.height - chipHalfH);
                draggedObj->setPosition(newPos.x, newPos.y);
                luaEngine.fireEvent("on_move", draggedObj.get());
                wasDragging = true;
            }

            if (event.type == sf::Event::MouseButtonReleased &&
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
                event.mouseButton.button == sf::Mouse::Left) {
                if (draggedObj && !wasDragging)
                    luaEngine.fireEvent("on_click", draggedObj.get());
                draggedObj = nullptr;
            }

            view.handleEvent(event);
        }

        view.update(deltaTime);
        view.render(allObjects);

        // ======== HUD ========
        if (fontOk) {
            const sf::Color p1Col(220, 80,  80);
            const sf::Color p2Col(80,  80,  220);

            // Scores
            drawLeftText(window, font,
                "Игрок 1: " + std::to_string(gs.scores[0]) + " / " + std::to_string(gs.targetScore),
                20, 18, 26, p1Col);

            {
                const std::string s2 = "Игрок 2: " + std::to_string(gs.scores[1]) +
                                       " / " + std::to_string(gs.targetScore);
                sf::Text t2;
                t2.setFont(font);
                t2.setString(sf::String::fromUtf8(s2.begin(), s2.end()));
                t2.setCharacterSize(26);
                t2.setFillColor(p2Col);
                t2.setOutlineColor(sf::Color::Black);
                t2.setOutlineThickness(1.5f);
                const auto lb = t2.getLocalBounds();
                t2.setPosition(1280.f - lb.width - 20.f, 18.f);
                window.draw(t2);
            }

            // Current player
            if (!gs.gameOver) {
                drawCenteredText(window, font,
                    ">>> Ход Игрока " + std::to_string(gs.currentPlayer) + " <<<",
                    640, 22, 26,
                    gs.currentPlayer == 1 ? p1Col : p2Col, 2.f);
            }

            // Button label
            drawCenteredText(window, font, "Закончить ход", 640, 660, 22,
                (gs.hasRolled && !gs.gameOver) ? sf::Color::White : sf::Color(160, 160, 160));

            // Hint
            drawLeftText(window, font,
                "Перемещай кубик | Кликни на своём кубике чтобы бросить | Закончить ход",
                20, 700, 15, sf::Color(130, 130, 130), 0.f);

            // Win screen
            if (gs.gameOver) {
                sf::RectangleShape overlay({1280.f, 720.f});
                overlay.setFillColor(sf::Color(0, 0, 0, 190));
                window.draw(overlay);
                const sf::Color wCol = (gs.winner == 1) ? p1Col : p2Col;
                drawCenteredText(window, font,
                    "Игрок " + std::to_string(gs.winner) + " ПОБЕДИЛ!",
                    640, 300, 72, wCol, 3.f);
                drawCenteredText(window, font,
                    "Счёт: " + std::to_string(gs.scores[gs.winner - 1]) + " очков",
                    640, 390, 36, sf::Color::White, 2.f);
                drawCenteredText(window, font,
                    "Нажмите ESC для выхода", 640, 450, 24, sf::Color(180, 180, 180));
            }
        }

        window.display();
    }

    spdlog::info("Application finished");
    return 0;
}
