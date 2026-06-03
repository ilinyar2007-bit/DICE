#ifndef DICE_NETWORK_NETWORK_MANAGER_HPP
#define DICE_NETWORK_NETWORK_MANAGER_HPP

#include <functional>
#include <memory>

#include "core/ActionManager.hpp"
#include "core/Model.hpp"
#include "network/GameClient.hpp"
#include "network/HostServer.hpp"
#include "scripting/LuaScriptEngine.hpp"

namespace dice::network {

enum class NetworkRole { SinglePlayer, Host, Client };

class NetworkManager {
public:
    NetworkManager(core::Model& model,
                   core::ActionManager& actionManager,
                   scripting::LuaScriptEngine& lua);
    ~NetworkManager();

    bool startHost(uint16_t port);
    bool joinGame(const std::string& hostIp, uint16_t port, const std::string& playerName);
    void leaveGame();

    NetworkRole getRole() const {
        return role_;
    }
    bool isConnected() const;
    bool isGameStarted() const;
    bool isHost() const {
        return role_ == NetworkRole::Host;
    }

    void sendEvent(const std::string& objectId,
                   const std::string& eventName,
                   std::function<void(bool)> callback = nullptr);
    void sendMoveObject(const std::string& objectId, float x, float y);
    void sendReady();
    void sendChat(const std::string& text);

    void startGame();
    void kickPlayer(const std::string& playerId);

    std::vector<ClientInfo> getPlayers() const;

    void update();

    void setOnPlayerJoined(std::function<void(const ClientInfo&)> handler);
    void setOnPlayerLeft(std::function<void(const std::string& playerId)> handler);
    void setOnPlayerReady(std::function<void(const std::string& playerId)> handler);
    void setOnGameStarted(std::function<void()> handler);
    void setOnChatReceived(
        std::function<void(const std::string& fromId, const std::string& text)> handler);

private:
    void registerLuaBindings();

    core::Model& model_;
    core::ActionManager& actionManager_;
    scripting::LuaScriptEngine& lua_;

    std::unique_ptr<HostServer> hostServer_;
    std::unique_ptr<GameClient> gameClient_;
    NetworkRole role_ = NetworkRole::SinglePlayer;

    std::function<void(const ClientInfo&)> onPlayerJoined_;
    std::function<void(const std::string&)> onPlayerLeft_;
    std::function<void(const std::string&)> onPlayerReady_;
    std::function<void()> onGameStarted_;
    std::function<void(const std::string&, const std::string&)> onChatReceived_;
};

} // namespace dice::network

#endif
