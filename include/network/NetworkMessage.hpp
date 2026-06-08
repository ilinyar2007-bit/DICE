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
    Event,
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

    std::string toString() const {
        return name + " (" + ip + ":" + std::to_string(port) + ")";
    }
};

struct NetworkMessage {
    MessageType type = MessageType::Disconnect;
    uint32_t sequenceId = 0;
    uint64_t timestamp = 0;
    std::string fromId;
    nlohmann::json data;

    std::vector<uint8_t> serialize() const;
    static NetworkMessage deserialize(const std::vector<uint8_t>& data);

    static NetworkMessage createHandshake(const std::string& player_name);
    static NetworkMessage createHandshakeAck(const std::string& client_id, bool game_started);
    static NetworkMessage createPlayerReady(const std::string& player_id);
    static NetworkMessage createStartGame();
    static NetworkMessage createSnapshot(const nlohmann::json& state);
    static NetworkMessage createEvent(const std::string& object_id, const std::string& event_name);
    static NetworkMessage createMoveObject(const std::string& object_id, float x, float y);
    static NetworkMessage createChat(const std::string& text);
    static NetworkMessage createPing();
    static NetworkMessage createPong();
    static NetworkMessage createPlayerJoined(const std::string& player_id,
                                             const std::string& player_name);
    static NetworkMessage createPlayerLeft(const std::string& player_id);
    static NetworkMessage createDisconnect(const std::string& reason = "");

    bool isValid() const;
    std::string toString() const;
};

} // namespace dice::network

#endif
