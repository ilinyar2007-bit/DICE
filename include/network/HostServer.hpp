#ifndef DICE_NETWORK_HOST_SERVER_HPP
#define DICE_NETWORK_HOST_SERVER_HPP

#include <atomic>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <SFML/Network.hpp>

#include "core/Action.hpp"
#include "core/ActionManager.hpp"
#include "core/Model.hpp"
#include "network/NetworkMessage.hpp"
#include "scripting/LuaScriptEngine.hpp"

namespace dice::network {

class HostServer {
public:
    HostServer(core::Model& model,
               core::ActionManager& action_manager,
               scripting::LuaScriptEngine& lua);
    ~HostServer();

    bool start(uint16_t port);
    void stop();
    bool isRunning() const {
        return isRunning_;
    }
    bool isGameStarted() const {
        return gameStarted_;
    }

    static std::string getLocalIp();
    uint16_t getPort() const {
        return port_;
    }
    std::vector<ClientInfo> getClients() const;
    size_t getClientCount() const {
        const std::lock_guard<std::mutex> lock(clientsMutex_);
        return clients_.size();
    }

    void startGame();
    void kickClient(const std::string& client_id);
    void sendChat(const std::string& text);

    void broadcastMoveObject(const std::string& object_id, float x, float y);
    void broadcastEvent(const std::string& object_id, const std::string& event_name);

    void update();

    void setOnClientJoined(std::function<void(const ClientInfo&)> handler);
    void setOnClientLeft(std::function<void(const std::string& clientId)> handler);
    void setOnClientReady(std::function<void(const std::string& clientId)> handler);
    void setOnGameStarted(std::function<void()> handler);
    void setOnChatReceived(std::function<void(const std::string&, const std::string&)> handler);

private:
    void serverLoop();
    void acceptNewClients();
    void receiveFromClient(sf::TcpSocket& socket, const std::string& client_id);
    void sendToClient(const std::string& client_id, const NetworkMessage& msg);
    void broadcast(const NetworkMessage& msg, const std::string& exclude_id = "");
    void removeClient(const std::string& client_id);
    void checkTimeouts();
    void broadcastSnapshot();

    void handleHandshake(const NetworkMessage& msg);
    void handlePlayerReady(const NetworkMessage& msg);
    void handleEvent(const NetworkMessage& msg);
    void handleMoveObject(const NetworkMessage& msg);
    void handleChat(const NetworkMessage& msg);
    void handlePing(const NetworkMessage& msg);

    bool isEventAllowedForClient(const std::string& event_name);

    static std::string generateId();

    core::Model& model_;
    core::ActionManager& actionManager_;
    scripting::LuaScriptEngine& lua_;

    sf::TcpListener listener_;
    std::unordered_map<std::string, std::unique_ptr<sf::TcpSocket>> clients_;
    std::unordered_map<std::string, ClientInfo> clientInfos_;

    uint16_t port_ = 0;
    std::atomic<bool> isRunning_{false};
    std::atomic<bool> gameStarted_{false};

    std::thread serverThread_;
    mutable std::mutex clientsMutex_;
    mutable std::mutex modelMutex_;

    std::chrono::steady_clock::time_point lastBroadcastTime_;
    std::chrono::steady_clock::time_point lastPingTime_;
    float broadcastInterval_ = 0.1f;
    float pingInterval_ = 5.0f;
    float timeoutInterval_ = 15.0f;

    std::unordered_set<std::string> allowedEvents_ = {"on_click", "on_drag_start", "on_drag_end"};

    std::function<void(const ClientInfo&)> onClientJoined_;
    std::function<void(const std::string&)> onClientLeft_;
    std::function<void(const std::string&)> onClientReady_;
    std::function<void()> onGameStarted_;
    std::function<void(const std::string&, const std::string&)> onChatReceived_;
};

} // namespace dice::network

#endif
