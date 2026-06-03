#include "network/GameClient.hpp"

#include <random>

#include <spdlog/spdlog.h>

namespace dice::network {

GameClient::GameClient() = default;

GameClient::~GameClient() {
    disconnect();
}

bool GameClient::connect(const std::string& hostIp, uint16_t port, const std::string& playerName) {
    if (isConnected_) {
        spdlog::warn("Already connected");
        return false;
    }

    spdlog::info("Connecting to {}:{} as {}", hostIp, port, playerName);

    sf::Socket::Status status = socket_.connect(hostIp, port);
    if (status != sf::Socket::Done) {
        spdlog::error("Failed to connect: {}", static_cast<int>(status));
        return false;
    }

    socket_.setBlocking(false);
    serverIp_ = hostIp;
    serverPort_ = port;
    isConnected_ = true;

    auto handshake = NetworkMessage::createHandshake(playerName, SCRIPTS_VERSION);
    send(handshake);

    running_ = true;
    receiveThread_ = std::thread(&GameClient::receiveLoop, this);

    spdlog::info("Connected to server");
    return true;
}

void GameClient::disconnect() {
    if (!isConnected_) {
        return;
    }
    if (running_) {
        auto disconnect = NetworkMessage::createDisconnect();
        send(disconnect);
    }

    running_ = false;
    isConnected_ = false;
    gameStarted_ = false;

    if (receiveThread_.joinable()) {
        receiveThread_.join();
    }

    socket_.disconnect();

    spdlog::info("Disconnected from server");

    if (onDisconnected_) {
        onDisconnected_();
    }
}

void GameClient::receiveLoop() {
    std::vector<uint8_t> buffer(65536);

    while (running_ && isConnected_) {
        std::size_t received;
        sf::Socket::Status status = socket_.receive(buffer.data(), buffer.size(), received);

        if (status == sf::Socket::Done) {
            buffer.resize(received);
            auto msg = NetworkMessage::deserialize(buffer);
            handleMessage(msg);
        } else if (status == sf::Socket::Disconnected) {
            spdlog::warn("Disconnected from server");
            isConnected_ = false;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void GameClient::handleMessage(const NetworkMessage& msg) {
    spdlog::debug("Received: {}", msg.toString());

    switch (msg.type) {
        case MessageType::HandshakeAck:
            clientId_ = msg.data.value("clientId", "");
            gameStarted_ = msg.data.value("gameStarted", false);
            spdlog::info("Handshake acknowledged. Client ID: {}", clientId_);
            if (onConnected_) {
                onConnected_(clientId_);
            }
            break;

        case MessageType::PlayerJoined: {
            ClientInfo info;
            info.id = msg.fromId;
            info.name = msg.data.value("name", "Unknown");
            info.status = static_cast<PlayerStatus>(msg.data.value("status", 0));
            spdlog::info("Player joined: {}", info.name);
            if (onPlayerJoined_) {
                onPlayerJoined_(info);
            }
            break;
        }

        case MessageType::PlayerLeft:
            spdlog::info("Player left: {}", msg.fromId);
            if (onPlayerLeft_) {
                onPlayerLeft_(msg.fromId);
            }
            break;

        case MessageType::PlayerReady:
            spdlog::info("Player ready: {}", msg.fromId);
            if (onPlayerReady_) {
                onPlayerReady_(msg.fromId);
            }
            break;

        case MessageType::StartGame:
            gameStarted_ = true;
            spdlog::info("Game started!");
            if (onGameStarted_) {
                onGameStarted_();
            }
            break;

        case MessageType::Snapshot:
            if (msg.data.contains("state")) {
                applySnapshot(msg.data["state"]);
            }
            break;

        case MessageType::Event: {
            std::string objectId = msg.data.value("object_id", "");
            std::string eventName = msg.data.value("event", "");
            applyEvent(objectId, eventName);
            break;
        }

        case MessageType::MoveObject:
            if (model_) {
                std::string objectId = msg.data.value("objectId", "");
                float x = msg.data.value("x", 0.0f);
                float y = msg.data.value("y", 0.0f);
                applyMoveObject(objectId, x, y);
            }
            break;

        case MessageType::Chat:
            if (onChatReceived_) {
                std::string text = msg.data.value("text", "");
                onChatReceived_(msg.fromId, text);
            }
            break;

        case MessageType::Disconnect:
            spdlog::info("Disconnected by server: {}", msg.data.value("reason", "No reason"));
            disconnect();
            break;

        case MessageType::Ping: {
            auto pong = NetworkMessage::createPong();
            send(pong);
            break;
        }

        default:
            break;
    }
}

void GameClient::sendEvent(const std::string& objectId, const std::string& eventName) {
    if (!gameStarted_) {
        spdlog::warn("Cannot send event - game not started");
        return;
    }

    auto msg = NetworkMessage::createEvent(objectId, eventName);
    send(msg);
    spdlog::debug("Sent event: {} on {}", eventName, objectId);
}

void GameClient::applyEvent(const std::string& objectId, const std::string& eventName) {
    if (!lua_ || !model_) {
        return;
    }
    auto obj = model_->getObject(objectId);
    if (!obj) {
        spdlog::warn("Cannot apply event - object not found: {}", objectId);
        return;
    }

    spdlog::debug("Applying event: {} on {}", eventName, objectId);

    lua_->fireEvent(eventName, obj.get());
}

void GameClient::applyMoveObject(const std::string& objectId, float x, float y) {
    if (!model_) {
        return;
    }
    spdlog::debug("Applying move: {} to ({}, {})", objectId, x, y);

    core::MoveObjectAction action(objectId, sf::Vector2f(x, y));
    if (action.canExecute(*model_)) {
        action.execute(*model_);
    }
}

void GameClient::applySnapshot(const nlohmann::json& state) {
    if (!model_ || !actionManager_) {
        return;
    }
    spdlog::debug("Applying snapshot");

    actionManager_->saveSnapshot(*model_);
    model_->fromJson(state);
}

void GameClient::sendMoveObject(const std::string& objectId, float x, float y) {
    if (!gameStarted_) {
        spdlog::warn("Cannot send move - game not started");
        return;
    }

    auto msg = NetworkMessage::createMoveObject(objectId, x, y);
    send(msg);
    spdlog::debug("Sent move: {} to ({}, {})", objectId, x, y);
}

void GameClient::sendReady() {
    auto ready = NetworkMessage::createPlayerReady(clientId_);
    send(ready);
    spdlog::info("Sent ready status");
}

void GameClient::sendChat(const std::string& text) {
    auto chat = NetworkMessage::createChat(text);
    send(chat);
}

void GameClient::send(const NetworkMessage& msg) {
    if (!isConnected_) {
        return;
    }
    auto data = msg.serialize();
    socket_.send(data.data(), data.size());
}

void GameClient::update() {
    static auto lastPingTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();

    if (std::chrono::duration<float>(now - lastPingTime).count() > 5.0f) {
        auto ping = NetworkMessage::createPing();
        send(ping);
        lastPingTime = now;
    }
}

void GameClient::setOnConnected(std::function<void(const std::string&)> handler) {
    onConnected_ = handler;
}

void GameClient::setOnDisconnected(std::function<void()> handler) {
    onDisconnected_ = handler;
}

void GameClient::setOnPlayerJoined(std::function<void(const ClientInfo&)> handler) {
    onPlayerJoined_ = handler;
}

void GameClient::setOnPlayerLeft(std::function<void(const std::string&)> handler) {
    onPlayerLeft_ = handler;
}

void GameClient::setOnPlayerReady(std::function<void(const std::string&)> handler) {
    onPlayerReady_ = handler;
}

void GameClient::setOnGameStarted(std::function<void()> handler) {
    onGameStarted_ = handler;
}

void GameClient::setOnChatReceived(
    std::function<void(const std::string&, const std::string&)> handler) {
    onChatReceived_ = handler;
}

} // namespace dice::network