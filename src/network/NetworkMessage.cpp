#include "network/NetworkMessage.hpp"

#include <chrono>

namespace dice::network {

std::vector<uint8_t> NetworkMessage::serialize() const {
    nlohmann::json json;
    json["type"] = static_cast<uint8_t>(type);
    json["seq"] = sequenceId;
    json["ts"] = timestamp;
    json["from"] = fromId;
    json["data"] = data;

    std::string str = json.dump();
    return std::vector<uint8_t>(str.begin(), str.end());
}

NetworkMessage NetworkMessage::deserialize(const std::vector<uint8_t>& data) {
    NetworkMessage msg;
    try {
        std::string str(data.begin(), data.end());
        nlohmann::json json = nlohmann::json::parse(str);

        msg.type = static_cast<MessageType>(json.value("type", 0));
        msg.sequenceId = json.value("seq", 0);
        msg.timestamp = json.value("ts", 0);
        msg.fromId = json.value("from", "");
        msg.data = json.value("data", nlohmann::json::object());
    } catch (const std::exception& e) {
        msg.type = MessageType::Disconnect;
    }
    return msg;
}

NetworkMessage NetworkMessage::createHandshake(const std::string& playerName,
                                               const std::string& scriptsVersion) {
    NetworkMessage msg;
    msg.type = MessageType::Handshake;
    msg.data["playerName"] = playerName;
    msg.data["scriptsVersion"] = scriptsVersion;
    msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
    return msg;
}

NetworkMessage NetworkMessage::createHandshakeAck(const std::string& clientId, bool gameStarted) {
    NetworkMessage msg;
    msg.type = MessageType::HandshakeAck;
    msg.data["clientId"] = clientId;
    msg.data["gameStarted"] = gameStarted;
    return msg;
}

NetworkMessage NetworkMessage::createPlayerReady(const std::string& playerId) {
    NetworkMessage msg;
    msg.type = MessageType::PlayerReady;
    msg.fromId = playerId;
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

NetworkMessage NetworkMessage::createAction(const std::string& actionId,
                                            const std::string& functionName,
                                            const nlohmann::json& params) {
    NetworkMessage msg;
    msg.type = MessageType::Action;
    msg.data["actionId"] = actionId;
    msg.data["function"] = functionName;
    msg.data["params"] = params;
    return msg;
}

NetworkMessage NetworkMessage::createActionAck(const std::string& actionId) {
    NetworkMessage msg;
    msg.type = MessageType::ActionAck;
    msg.data["actionId"] = actionId;
    return msg;
}

NetworkMessage NetworkMessage::createActionReject(const std::string& actionId,
                                                  const std::string& reason) {
    NetworkMessage msg;
    msg.type = MessageType::ActionReject;
    msg.data["actionId"] = actionId;
    msg.data["reason"] = reason;
    return msg;
}

NetworkMessage NetworkMessage::createLuaCall(const std::string& functionName,
                                             const nlohmann::json& params) {
    NetworkMessage msg;
    msg.type = MessageType::LuaCall;
    msg.data["function"] = functionName;
    msg.data["params"] = params;
    return msg;
}

NetworkMessage NetworkMessage::createMoveObject(const std::string& objectId, float x, float y) {
    NetworkMessage msg;
    msg.type = MessageType::MoveObject;
    msg.data["objectId"] = objectId;
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

NetworkMessage NetworkMessage::createPlayerJoined(const std::string& playerId,
                                                  const std::string& playerName) {
    NetworkMessage msg;
    msg.type = MessageType::PlayerJoined;
    msg.fromId = playerId;
    msg.data["name"] = playerName;
    msg.data["status"] = static_cast<uint8_t>(PlayerStatus::Connected);
    return msg;
}

NetworkMessage NetworkMessage::createPlayerLeft(const std::string& playerId) {
    NetworkMessage msg;
    msg.type = MessageType::PlayerLeft;
    msg.fromId = playerId;
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
    return type != MessageType::Disconnect;
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
        case MessageType::Action:
            str += "Action";
            break;
        case MessageType::ActionAck:
            str += "ActionAck";
            break;
        case MessageType::ActionReject:
            str += "ActionReject";
            break;
        case MessageType::LuaCall:
            str += "LuaCall";
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