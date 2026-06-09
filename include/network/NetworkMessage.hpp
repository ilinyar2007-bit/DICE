#ifndef DICE_NETWORK_NETWORK_MESSAGE_HPP
#define DICE_NETWORK_NETWORK_MESSAGE_HPP

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
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
    Chat,
    Invalid = 255
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

class MessageBuffer {
public:
    void append(const std::vector<uint8_t>& data) {
        buffer_.insert(buffer_.end(), data.begin(), data.end());
    }

    void append(const uint8_t* data, size_t size) {
        buffer_.insert(buffer_.end(), data, data + size);
    }

    std::optional<NetworkMessage> extract() {
        if (buffer_.size() < 4) {
            return std::nullopt;
        }

        uint32_t msgLength =
            (static_cast<uint32_t>(buffer_[0]) << 24) | (static_cast<uint32_t>(buffer_[1]) << 16) |
            (static_cast<uint32_t>(buffer_[2]) << 8) | static_cast<uint32_t>(buffer_[3]);

        if (buffer_.size() < 4 + msgLength) {
            return std::nullopt;
        }

        std::string jsonStr(buffer_.begin() + 4, buffer_.begin() + 4 + msgLength);
        buffer_.erase(buffer_.begin(), buffer_.begin() + 4 + msgLength);

        std::vector<uint8_t> jsonData(jsonStr.begin(), jsonStr.end());
        return NetworkMessage::deserialize(jsonData);
    }

    void clear() {
        buffer_.clear();
    }
    bool empty() const {
        return buffer_.empty();
    }
    size_t size() const {
        return buffer_.size();
    }

private:
    std::vector<uint8_t> buffer_;
};

struct NetworkMessage {
    MessageType type = MessageType::Invalid;
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
