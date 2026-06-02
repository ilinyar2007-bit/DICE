#include <memory>

#include <SFML/Graphics.hpp>

#include "components/Card.hpp"
#include "components/Chip.hpp"
#include "core/GameObject.hpp"
#include "ui/View.hpp"
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

using dice::components::Card;
using dice::components::Chip;
using dice::core::GameObject;
using dice::view::View;
using dice::view::ViewConfig;

// ========== Fixture ==========

class ViewTest : public ::testing::Test {
protected:
    void SetUp() override {
        spdlog::set_level(spdlog::level::off);
        window.create(sf::VideoMode(800, 600), "Test Window", sf::Style::Default);
        window.setVisible(false);
        view = std::make_unique<View>(window);

        ViewConfig config;
        config.showFPS = false;
        config.showObjectCount = false;
        config.showControls = false;
        view->setConfig(config);

        createTestTextures();
    }

    void TearDown() override {
        view.reset();
        if (window.isOpen()) {
            window.close();
        }
    }

    void createTestTextures() {
        testTexture.create(50, 50);
        sf::Image testImage;
        testImage.create(50, 50, sf::Color::White);
        testTexture.update(testImage);

        cardTexture.create(100, 140);
        sf::Image cardImage;
        cardImage.create(100, 140, sf::Color(220, 220, 255));
        cardTexture.update(cardImage);

        chipTexture.create(64, 64);
        sf::Image chipImage;
        chipImage.create(64, 64, sf::Color::Red);
        chipTexture.update(chipImage);
    }

    std::shared_ptr<GameObject> createTestObject(
        const std::string& id, const std::string& name, float x, float y, int z_order = 0) {
        auto obj = std::make_shared<GameObject>(id, name);
        obj->setTexture(&testTexture);
        obj->setPosition(x, y);
        obj->setZOrder(z_order);
        return obj;
    }
    sf::RenderWindow window;    // NOLINT
    std::unique_ptr<View> view; // NOLINT
    sf::Texture testTexture;    // NOLINT
    sf::Texture cardTexture;    // NOLINT
    sf::Texture chipTexture;    // NOLINT
};

// ========== Constructor Tests ==========

TEST_F(ViewTest, ConstructorInitializesCorrectly) {
    EXPECT_NE(view, nullptr);

    auto& config = view->getConfig();
    EXPECT_EQ(config.backgroundColor, sf::Color(50, 50, 50));
    EXPECT_EQ(config.fontAssetId, "default_font");
}

// ========== Update tests ==========

TEST_F(ViewTest, UpdateDoesNotCrash) {
    EXPECT_NO_THROW(view->update(0.016F));
    EXPECT_NO_THROW(view->update(0.033F));
    EXPECT_NO_THROW(view->update(0.0F));
}

// ========== Coordinate transformation tests ==========

TEST_F(ViewTest, ScreenToWorldConversion) {
    const sf::Vector2i screenPoint(400, 300);
    const sf::Vector2f worldPoint = view->screenToWorld(screenPoint);

    EXPECT_FLOAT_EQ(worldPoint.x, 400.0F);
    EXPECT_FLOAT_EQ(worldPoint.y, 300.0F);
}

TEST_F(ViewTest, WorldToScreenConversion) {
    const sf::Vector2f worldPoint(400.0F, 300.0F);
    const sf::Vector2i screenPoint = view->worldToScreen(worldPoint);

    EXPECT_EQ(screenPoint.x, 400);
    EXPECT_EQ(screenPoint.y, 300);
}

TEST_F(ViewTest, CoordinateConversionsAreInverse) {
    const sf::Vector2i originalScreen(123, 456);

    const sf::Vector2f world = view->screenToWorld(originalScreen);
    const sf::Vector2i backToScreen = view->worldToScreen(world);

    EXPECT_EQ(backToScreen.x, originalScreen.x);
    EXPECT_EQ(backToScreen.y, originalScreen.y);
}

// ========== pickObject Tests ==========

TEST_F(ViewTest, PickObjectReturnsCorrectObject) {
    auto obj1 = createTestObject("obj1", "Object 1", 100, 100);
    auto obj2 = createTestObject("obj2", "Object 2", 200, 200);

    const std::vector<std::shared_ptr<GameObject>> objects = {obj1, obj2};

    auto picked = view->pickObject({124, 124}, objects);
    EXPECT_EQ(picked, obj1);

    picked = view->pickObject({224, 224}, objects);
    EXPECT_EQ(picked, obj2);

    picked = view->pickObject({0, 0}, objects);
    EXPECT_EQ(picked, nullptr);
}

TEST_F(ViewTest, PickObjectRespectsVisibility) {
    auto obj1 = createTestObject("obj1", "Object 1", 100, 100);
    obj1->setVisible(false);
    auto obj2 = createTestObject("obj2", "Object 2", 100, 100);
    const std::vector<std::shared_ptr<GameObject>> objects = {obj1, obj2};

    auto picked = view->pickObject({124, 124}, objects);
    EXPECT_EQ(picked, obj2);
}

TEST_F(ViewTest, PickObjectRespectsActive) {
    auto obj1 = createTestObject("obj1", "Object 1", 100, 100);
    obj1->setActive(false);
    auto obj2 = createTestObject("obj2", "Object 2", 100, 100);
    const std::vector<std::shared_ptr<GameObject>> objects = {obj1, obj2};

    auto picked = view->pickObject({124, 124}, objects);
    EXPECT_EQ(picked, obj2);
}

TEST_F(ViewTest, PickObjectRespectsZOrder) {
    auto objLow = createTestObject("low", "Low", 100, 100, 10);
    auto objHigh = createTestObject("high", "High", 100, 100, 20);
    const std::vector<std::shared_ptr<GameObject>> objects = {objLow, objHigh};

    auto picked = view->pickObject({124, 124}, objects);
    EXPECT_EQ(picked, objHigh);
}

TEST_F(ViewTest, PickObjectHandlesNullptrs) {
    auto obj1 = createTestObject("obj1", "Object 1", 100, 100);

    std::vector<std::shared_ptr<GameObject>> objects;
    objects.push_back(nullptr);
    objects.push_back(obj1);
    objects.push_back(nullptr);

    auto picked = view->pickObject({124, 124}, objects);
    EXPECT_EQ(picked, obj1);
}

TEST_F(ViewTest, PickObjectWithEmptyList) {
    const std::vector<std::shared_ptr<GameObject>> empty;

    auto picked = view->pickObject({100, 100}, empty);
    EXPECT_EQ(picked, nullptr);
}

// ========== Sorting tests ==========

TEST_F(ViewTest, SortObjectsByZOrderWorks) {
    auto objHigh = createTestObject("high", "High", 0, 0, 30);
    auto objMid = createTestObject("mid", "Mid", 0, 0, 20);
    auto objLow = createTestObject("low", "Low", 0, 0, 10);

    const std::vector<std::shared_ptr<GameObject>> objects = {objHigh, objMid, objLow};

    auto picked = view->pickObject({24, 24}, objects);
    EXPECT_EQ(picked, objHigh);
}

TEST_F(ViewTest, SortRemovesNullptrs) {
    auto obj1 = createTestObject("obj1", "Object 1", 100, 100);
    auto obj2 = createTestObject("obj2", "Object 2", 200, 200);

    std::vector<std::shared_ptr<GameObject>> objects;
    objects.push_back(nullptr);
    objects.push_back(obj1);
    objects.push_back(nullptr);
    objects.push_back(obj2);
    objects.push_back(nullptr);

    auto picked1 = view->pickObject({124, 124}, objects);
    EXPECT_EQ(picked1, obj1);

    auto picked2 = view->pickObject({224, 224}, objects);
    EXPECT_EQ(picked2, obj2);
}

// ========== Event handling tests ==========

TEST_F(ViewTest, HandleResizeEvent) {
    window.setSize(sf::Vector2u(1024, 768));

    const sf::Event resizeEvent{.type = sf::Event::Resized, .size = {1024, 768}};

    EXPECT_NO_THROW(view->handleEvent(resizeEvent));

    const sf::Vector2i screenPoint(1024, 768);
    const sf::Vector2f worldPoint = view->screenToWorld(screenPoint);

    EXPECT_FLOAT_EQ(worldPoint.x, 1024.0F);
    EXPECT_FLOAT_EQ(worldPoint.y, 768.0F);
}

TEST_F(ViewTest, HandleOtherEvents) {
    const sf::Event keyEvent{.type = sf::Event::KeyPressed, .key = {.code = sf::Keyboard::Space}};

    EXPECT_NO_THROW(view->handleEvent(keyEvent));

    sf::Event mouseEvent{};
    mouseEvent.type = sf::Event::MouseButtonPressed;
    EXPECT_NO_THROW(view->handleEvent(mouseEvent));
}

// ========== Configuration tests ==========

TEST_F(ViewTest, CanSetAndGetConfig) {
    ViewConfig newConfig;
    newConfig.backgroundColor = sf::Color::Red;
    newConfig.showFPS = true;
    newConfig.showObjectCount = false;
    newConfig.showControls = true;
    newConfig.textColor = sf::Color::Green;
    newConfig.fontAssetId = "custom_font";

    view->setConfig(newConfig);

    auto& retrieved = view->getConfig();
    EXPECT_EQ(retrieved.backgroundColor, sf::Color::Red);
    EXPECT_EQ(retrieved.showFPS, true);
    EXPECT_EQ(retrieved.showObjectCount, false);
    EXPECT_EQ(retrieved.showControls, true);
    EXPECT_EQ(retrieved.textColor, sf::Color::Green);
    EXPECT_EQ(retrieved.fontAssetId, "custom_font");
}

// ========== Rendering tests ==========

TEST_F(ViewTest, RenderWithEmptyList) {
    const std::vector<std::shared_ptr<GameObject>> empty;

    EXPECT_NO_THROW(view->render(empty));
}

TEST_F(ViewTest, RenderWithSingleObject) {
    auto obj = createTestObject("obj", "Object", 100, 100);
    const std::vector<std::shared_ptr<GameObject>> objects = {obj};

    EXPECT_NO_THROW(view->render(objects));
}

TEST_F(ViewTest, RenderWithMultipleObjects) {
    std::vector<std::shared_ptr<GameObject>> objects;
    objects.reserve(10);
    for (int i = 0; i < 10; i++) {
        objects.push_back(createTestObject("obj" + std::to_string(i),
                                           "Object " + std::to_string(i),
                                           static_cast<float>(i * 50),
                                           static_cast<float>(i * 50),
                                           i));
    }

    EXPECT_NO_THROW(view->render(objects));
}

TEST_F(ViewTest, RenderWithNullptrs) {
    auto obj1 = createTestObject("obj1", "Object 1", 100, 100);
    auto obj2 = createTestObject("obj2", "Object 2", 200, 200);

    std::vector<std::shared_ptr<GameObject>> objects;
    objects.push_back(nullptr);
    objects.push_back(obj1);
    objects.push_back(nullptr);
    objects.push_back(obj2);
    objects.push_back(nullptr);

    EXPECT_NO_THROW(view->render(objects));
}

TEST_F(ViewTest, RenderWithInvisibleObjects) {
    auto visibleObj = createTestObject("visible", "Visible", 100, 100);
    auto invisibleObj = createTestObject("invisible", "Invisible", 200, 200);
    invisibleObj->setVisible(false);
    const std::vector<std::shared_ptr<GameObject>> objects = {visibleObj, invisibleObj};

    EXPECT_NO_THROW(view->render(objects));
}

// ========== Tests with real components ==========

TEST_F(ViewTest, WorksWithRealCard) {
    auto card = std::make_shared<Card>("card1", "Test Card");
    card->setFrontTexture(&cardTexture);
    card->setBackTexture(&cardTexture);
    card->setFaceUp(true);
    card->setPosition(100, 100);

    const std::vector<std::shared_ptr<GameObject>> objects = {card};

    EXPECT_NO_THROW(view->render(objects));

    auto picked = view->pickObject({149, 150}, objects);
    EXPECT_EQ(picked, card);
}

TEST_F(ViewTest, WorksWithRealChip) {
    auto chip = std::make_shared<Chip>("chip1", "Test Chip");
    chip->setTexture(&chipTexture);
    chip->setRadius(32.F);
    chip->setPosition(100, 100);
    const std::vector<std::shared_ptr<GameObject>> objects = {chip};

    EXPECT_NO_THROW(view->render(objects));

    auto picked = view->pickObject({131, 131}, objects);
    EXPECT_EQ(picked, chip);
}

// ========== HUD Tests ==========

TEST_F(ViewTest, HUDWithAllFeaturesEnabled) {
    auto& config = view->getConfig();
    config.showFPS = true;
    config.showObjectCount = true;
    config.showControls = true;

    auto obj = createTestObject("obj", "Object", 100, 100);
    const std::vector<std::shared_ptr<GameObject>> objects = {obj};

    EXPECT_NO_THROW(view->render(objects));
}

TEST_F(ViewTest, HUDWithSomeFeaturesEnabled) {
    auto& config = view->getConfig();
    config.showFPS = true;
    config.showObjectCount = false;
    config.showControls = true;

    auto obj = createTestObject("obj", "Object", 100, 100);
    const std::vector<std::shared_ptr<GameObject>> objects = {obj};

    EXPECT_NO_THROW(view->render(objects));
}

TEST_F(ViewTest, HUDWithAllFeaturesDisabled) {
    auto& config = view->getConfig();
    config.showFPS = false;
    config.showObjectCount = false;
    config.showControls = false;

    auto obj = createTestObject("obj", "Object", 100, 100);
    const std::vector<std::shared_ptr<GameObject>> objects = {obj};

    EXPECT_NO_THROW(view->render(objects));
}

// ========== Boundary case tests ==========

TEST_F(ViewTest, HandlesLargeNumberOfObjects) {
    std::vector<std::shared_ptr<GameObject>> manyObjects;
    for (int i = 0; i < 1000; ++i) {
        auto obj = createTestObject("obj" + std::to_string(i),
                                    "Object " + std::to_string(i),
                                    static_cast<float>(i % 10 * 50),
                                    static_cast<float>(i) / 10 * 50,
                                    i % 100);
        manyObjects.push_back(obj);
    }

    EXPECT_NO_THROW(view->render(manyObjects));

    auto picked = view->pickObject({25, 25}, manyObjects);
    EXPECT_NE(picked, nullptr);
}

TEST_F(ViewTest, HandlesWindowResizeDuringRender) {
    auto obj = createTestObject("obj", "Object", 100, 100);
    const std::vector<std::shared_ptr<GameObject>> objects = {obj};

    window.setSize(sf::Vector2u(1024, 768));
    const sf::Event resizeEvent{.type = sf::Event::Resized, .size = {1024, 768}};
    view->handleEvent(resizeEvent);

    EXPECT_NO_THROW(view->render(objects));
}

TEST_F(ViewTest, HandlesMultipleRenderCalls) {
    auto obj = createTestObject("obj", "Object", 100, 100);
    const std::vector<std::shared_ptr<GameObject>> objects = {obj};

    for (int i = 0; i < 10; ++i) {
        EXPECT_NO_THROW(view->render(objects));
    }
}

TEST_F(ViewTest, HandlesUpdateAndRenderInterleaved) {
    auto obj = createTestObject("obj", "Object", 100, 100);
    const std::vector<std::shared_ptr<GameObject>> objects = {obj};

    for (int i = 0; i < 10; ++i) {
        view->update(0.016F);
        EXPECT_NO_THROW(view->render(objects));
    }
}

// ========== Font Tests ==========

// TODO
