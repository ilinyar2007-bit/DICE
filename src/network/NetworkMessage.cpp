#include "network/NetworkMessage.hpp"

#include <chrono>

#include <spdlog/spdlog.h>

namespace dice::network {

std::vector<uint8_t> NetworkMessage::serialize() const {
    nlohmann::json json;
    json["type"] = static_cast<uint8_t>(type);
    json["seq"] = sequenceId;
    json["ts"] = timestamp;
    json["from"] = fromId;
    json["data"] = data;

    std::string jsonStr = json.dump();
    const auto length = static_cast<uint32_t>(jsonStr.size());

    std::vector<uint8_t> result;
    result.reserve(4 + length);

    result.push_back(static_cast<uint8_t>((length >> 24) & 0xFF));
    result.push_back(static_cast<uint8_t>((length >> 16) & 0xFF));
    result.push_back(static_cast<uint8_t>((length >> 8) & 0xFF));
    result.push_back(static_cast<uint8_t>(length & 0xFF));

    result.insert(result.end(), jsonStr.begin(), jsonStr.end());

    return result;
}

NetworkMessage NetworkMessage::deserialize(const std::vector<uint8_t>& data) {
    NetworkMessage msg;
    msg.type = MessageType::Invalid;

    try {
        const std::string jsonStr(data.begin(), data.end());
        const nlohmann::json json = nlohmann::json::parse(jsonStr);

        msg.type = static_cast<MessageType>(json.value("type", 0));
        msg.sequenceId = json.value("seq", 0);
        msg.timestamp = json.value("ts", 0);
        msg.fromId = json.value("from", "");
        msg.data = json.value("data", nlohmann::json::object());
    } catch (const nlohmann::json::parse_error& e) {
        spdlog::error("JSON parse error: {}", e.what());
    } catch (const std::exception& e) {
        spdlog::error("Deserialize error: {}", e.what());
    }

    return msg;
}

NetworkMessage NetworkMessage::createHandshake(const std::string& player_name) {
    NetworkMessage msg;
    msg.type = MessageType::Handshake;
    msg.data["playerName"] = player_name;
    msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
    return msg;
}

NetworkMessage NetworkMessage::createHandshakeAck(const std::string& client_id, bool game_started) {
    NetworkMessage msg;
    msg.type = MessageType::HandshakeAck;
    msg.data["clientId"] = client_id;
    msg.data["gameStarted"] = game_started;
    return msg;
}

NetworkMessage NetworkMessage::createPlayerReady(const std::string& player_id) {
    NetworkMessage msg;
    msg.type = MessageType::PlayerReady;
    msg.fromId = player_id;
    return msg;
}

NetworkMessage NetworkMessage::createStartGame() {
    NetworkMessage msg;
    msg.type = MessageType::StartGame;
    return msg;
}

NetworkMessage NetworkMessage::createSnapshot(const nlohmann::json& state) {
    NetworkMessage msg;
    msg.type = MessageType::Snapshot;
    msg.data["state"] = state;
    return msg;
}

NetworkMessage NetworkMessage::createEvent(const std::string& object_id,
                                           const std::string& event_name) {
    NetworkMessage msg;
    msg.type = MessageType::Event;
    msg.data["object_id"] = object_id;
    msg.data["event"] = event_name;
    return msg;
}

NetworkMessage NetworkMessage::createMoveObject(const std::string& object_id, float x, float y) {
    NetworkMessage msg;
    msg.type = MessageType::MoveObject;
    msg.data["objectId"] = object_id;
    msg.data["x"] = x;
    msg.data["y"] = y;
    return msg;
}

NetworkMessage NetworkMessage::createChat(const std::string& text) {
    NetworkMessage msg;
    msg.type = MessageType::Chat;
    msg.data["text"] = text;
    return msg;
}

NetworkMessage NetworkMessage::createPing() {
    NetworkMessage msg;
    msg.type = MessageType::Ping;
    msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now().time_since_epoch())
                        .count();
    return msg;
}

NetworkMessage NetworkMessage::createPong() {
    NetworkMessage msg;
    msg.type = MessageType::Pong;
    msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now().time_since_epoch())
                        .count();
    return msg;
}

NetworkMessage NetworkMessage::createPlayerJoined(const std::string& player_id,
                                                  const std::string& player_name) {
    NetworkMessage msg;
    msg.type = MessageType::PlayerJoined;
    msg.fromId = player_id;
    msg.data["name"] = player_name;
    msg.data["status"] = static_cast<uint8_t>(PlayerStatus::Connected);
    return msg;
}

NetworkMessage NetworkMessage::createPlayerLeft(const std::string& player_id) {
    NetworkMessage msg;
    msg.type = MessageType::PlayerLeft;
    msg.fromId = player_id;
    return msg;
}

NetworkMessage NetworkMessage::createDisconnect(const std::string& reason) {
    NetworkMessage msg;
    msg.type = MessageType::Disconnect;
    if (!reason.empty()) {
        msg.data["reason"] = reason;
    }
    return msg;
}

bool NetworkMessage::isValid() const {
    return type != MessageType::Disconnect && type != MessageType::Invalid;
}

std::string NetworkMessage::toString() const {
    std::string str = "[";
    switch (type) {
        case MessageType::Handshake:
            str += "Handshake";
            break;
        case MessageType::HandshakeAck:
            str += "HandshakeAck";
            break;
        case MessageType::Ping:
            str += "Ping";
            break;
        case MessageType::Pong:
            str += "Pong";
            break;
        case MessageType::Disconnect:
            str += "Disconnect";
            break;
        case MessageType::PlayerJoined:
            str += "PlayerJoined";
            break;
        case MessageType::PlayerLeft:
            str += "PlayerLeft";
            break;
        case MessageType::PlayerReady:
            str += "PlayerReady";
            break;
        case MessageType::StartGame:
            str += "StartGame";
            break;
        case MessageType::Snapshot:
            str += "Snapshot";
            break;
        case MessageType::Event:
            str += "Event";
            break;
        case MessageType::MoveObject:
            str += "MoveObject";
            break;
        case MessageType::Chat:
            str += "Chat";
            break;
        default:
            str += "Unknown";
    }
    str += "] from=" + fromId;
    return str;
}

} // namespace dice::network
