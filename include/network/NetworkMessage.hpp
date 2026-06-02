#ifndef DICE_NETWORK_NETWORK_MESSAGE_HPP
#define DICE_NETWORK_NETWORK_MESSAGE_HPP

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace dice::network {

enum class MessageType : uint8_t {
    Handshake = 0,
    HandshakeAck,
    Ping,
    Pong,
    Disconnect,

    PlayerJoined,
    PlayerLeft,
    PlayerReady,
    StartGame,

    Snapshot,
    Action,
    ActionAck,
    ActionReject,
    LuaCall,
    MoveObject,
    Chat
};

enum class PlayerStatus : uint8_t { Connecting = 0, Connected, Ready, InGame, Disconnected };

struct ClientInfo {
    std::string id;
    std::string name;
    std::string ip;
    uint16_t port = 0;
    PlayerStatus status = PlayerStatus::Connecting;
    std::chrono::steady_clock::time_point lastPing;
    std::string scriptsVersion;

    std::string toString() const {
        return name + " (" + ip + ":" + std::to_string(port) + ")";
    }
};

struct PendingAction {
    std::string actionId;
    std::string functionName;
    nlohmann::json params;
    nlohmann::json predictedState;
    std::chrono::steady_clock::time_point timestamp;
    std::function<void(bool)> callback;
};

struct NetworkMessage {
    MessageType type = MessageType::Disconnect;
    uint32_t sequenceId = 0;
    uint64_t timestamp = 0;
    std::string fromId;
    nlohmann::json data;

    std::vector<uint8_t> serialize() const;
    static NetworkMessage deserialize(const std::vector<uint8_t>& data);

    static NetworkMessage createHandshake(const std::string& playerName,
                                          const std::string& scriptsVersion);
    static NetworkMessage createHandshakeAck(const std::string& clientId, bool gameStarted);
    static NetworkMessage createPlayerReady(const std::string& playerId);
    static NetworkMessage createStartGame();
    static NetworkMessage createSnapshot(const nlohmann::json& state);
    static NetworkMessage createAction(const std::string& actionId,
                                       const std::string& functionName,
                                       const nlohmann::json& params);
    static NetworkMessage createActionAck(const std::string& actionId);
    static NetworkMessage createActionReject(const std::string& actionId,
                                             const std::string& reason);
    static NetworkMessage createLuaCall(const std::string& functionName,
                                        const nlohmann::json& params);
    static NetworkMessage createMoveObject(const std::string& objectId, float x, float y);
    static NetworkMessage createChat(const std::string& text);
    static NetworkMessage createPing();
    static NetworkMessage createPong();
    static NetworkMessage createPlayerJoined(const std::string& playerId,
                                             const std::string& playerName);
    static NetworkMessage createPlayerLeft(const std::string& playerId);
    static NetworkMessage createDisconnect(const std::string& reason = "");

    bool isValid() const;
    std::string toString() const;
};

// constexpr const char* SCRIPTS_VERSION = "1.0.0";

} // namespace dice::network

#endif
