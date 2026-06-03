#ifndef DICE_NETWORK_GAME_CLIENT_HPP
#define DICE_NETWORK_GAME_CLIENT_HPP

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

#include <SFML/Network.hpp>

#include "core/Action.hpp"
#include "core/ActionManager.hpp"
#include "core/Model.hpp"
#include "network/NetworkMessage.hpp"
#include "scripting/LuaScriptEngine.hpp"

namespace dice::network {

class GameClient {
public:
    GameClient();
    ~GameClient();

    bool connect(const std::string& hostIp, uint16_t port, const std::string& playerName);
    void disconnect();
    bool isConnected() const {
        return isConnected_;
    }
    bool isGameStarted() const {
        return gameStarted_;
    }

    std::string getClientId() const {
        return clientId_;
    }

    void sendEvent(const std::string& objectId, const std::string& eventName);
    void sendMoveObject(const std::string& objectId, float x, float y);
    void sendReady();
    void sendChat(const std::string& text);

    void update();

    void setModel(core::Model* model) {
        model_ = model;
    }
    void setActionManager(core::ActionManager* actionManager) {
        actionManager_ = actionManager;
    }
    void setLuaEngine(scripting::LuaScriptEngine* lua) {
        lua_ = lua;
    }

    void setOnConnected(std::function<void(const std::string& clientId)> handler);
    void setOnDisconnected(std::function<void()> handler);
    void setOnPlayerJoined(std::function<void(const ClientInfo&)> handler);
    void setOnPlayerLeft(std::function<void(const std::string& playerId)> handler);
    void setOnPlayerReady(std::function<void(const std::string& playerId)> handler);
    void setOnGameStarted(std::function<void()> handler);
    void setOnChatReceived(
        std::function<void(const std::string& fromId, const std::string& text)> handler);

private:
    void receiveLoop();
    void handleMessage(const NetworkMessage& msg);
    void send(const NetworkMessage& msg);

    void applyEvent(const std::string& objectId, const std::string& eventName);
    void applyMoveObject(const std::string& objectId, float x, float y);
    void applySnapshot(const nlohmann::json& state);

    sf::TcpSocket socket_;
    std::string clientId_;
    std::string serverIp_;
    uint16_t serverPort_ = 0;

    std::atomic<bool> isConnected_{false};
    std::atomic<bool> gameStarted_{false};

    std::thread receiveThread_;
    std::atomic<bool> running_{false};

    core::Model* model_ = nullptr;
    core::ActionManager* actionManager_ = nullptr;
    scripting::LuaScriptEngine* lua_ = nullptr;

    std::function<void(const std::string&)> onConnected_;
    std::function<void()> onDisconnected_;
    std::function<void(const ClientInfo&)> onPlayerJoined_;
    std::function<void(const std::string&)> onPlayerLeft_;
    std::function<void(const std::string&)> onPlayerReady_;
    std::function<void()> onGameStarted_;
    std::function<void(const std::string&, const std::string&)> onChatReceived_;
};

} // namespace dice::network

#endif
