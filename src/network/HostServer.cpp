#include "network/HostServer.hpp"

#include <random>

#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

namespace dice::network {

HostServer::HostServer(core::Model& model,
                       core::ActionManager& actionManager,
                       scripting::LuaScriptEngine& lua)
    : model_(model), actionManager_(actionManager), lua_(lua) {}

HostServer::~HostServer() {
    stop();
}

bool HostServer::start(uint16_t port) {
    if (isRunning_) {
        spdlog::warn("Server already running");
        return false;
    }

    port_ = port;

    if (listener_.listen(port_) != sf::Socket::Done) {
        spdlog::error("Failed to listen on port {}", port_);
        return false;
    }

    listener_.setBlocking(false);
    isRunning_ = true;
    gameStarted_ = false;

    serverThread_ = std::thread(&HostServer::serverLoop, this);

    spdlog::info("Server started on port {}", port_);
    spdlog::info("Local IP: {}", getLocalIp());

    return true;
}

void HostServer::stop() {
    if (!isRunning_) {
        return;
    }
    isRunning_ = false;
    listener_.close();

    std::lock_guard<std::mutex> lock(clientsMutex_);
    for (auto& [id, socket] : clients_) {
        socket->disconnect();
    }
    clients_.clear();
    clientInfos_.clear();

    if (serverThread_.joinable()) {
        serverThread_.join();
    }

    spdlog::info("Server stopped");
}

void HostServer::serverLoop() {
    while (isRunning_) {
        acceptNewClients();

        std::lock_guard<std::mutex> lock(clientsMutex_);
        for (auto& [id, socket] : clients_) {
            receiveFromClient(*socket, id);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void HostServer::acceptNewClients() {
    auto client = std::make_unique<sf::TcpSocket>();

    if (listener_.accept(*client) == sf::Socket::Done) {
        client->setBlocking(false);

        std::string clientId = generateId();

        std::lock_guard<std::mutex> lock(clientsMutex_);
        clients_[clientId] = std::move(client);

        ClientInfo info;
        info.id = clientId;
        info.name = "Player_" + std::to_string(clients_.size());
        info.ip = clients_[clientId]->getRemoteAddress().toString();
        info.port = clients_[clientId]->getRemotePort();
        info.status = PlayerStatus::Connecting;
        info.lastPing = std::chrono::steady_clock::now();
        info.scriptsVersion = "";
        clientInfos_[clientId] = info;

        spdlog::info("New client connected: {} ({})", info.name, info.ip);

        if (onClientJoined_) {
            onClientJoined_(info);
        }
    }
}

void HostServer::receiveFromClient(sf::TcpSocket& socket, const std::string& clientId) {
    std::vector<uint8_t> buffer(65536);
    std::size_t received;

    if (socket.receive(buffer.data(), buffer.size(), received) == sf::Socket::Done) {
        buffer.resize(received);
        NetworkMessage msg = NetworkMessage::deserialize(buffer);
        msg.fromId = clientId;

        spdlog::debug("Received: {}", msg.toString());

        {
            std::lock_guard<std::mutex> lock(clientsMutex_);
            if (clientInfos_.count(clientId)) {
                clientInfos_[clientId].lastPing = std::chrono::steady_clock::now();
            }
        }
        switch (msg.type) {
            case MessageType::Handshake:
                handleHandshake(msg);
                break;
            case MessageType::PlayerReady:
                handlePlayerReady(msg);
                break;
            case MessageType::Event:
                handleEvent(msg);
                break;
            case MessageType::MoveObject:
                handleMoveObject(msg);
                break;
            case MessageType::Chat:
                handleChat(msg);
                break;
            case MessageType::Ping:
                handlePing(msg);
                break;
            case MessageType::Disconnect:
                removeClient(clientId);
                break;
            default:
                break;
        }
    }
}

void HostServer::handleHandshake(const NetworkMessage& msg) {
    std::string playerName = msg.data.value("playerName", "Unknown");
    std::string clientVersion = msg.data.value("scriptsVersion", "");

    if (clientVersion != SCRIPTS_VERSION) {
        spdlog::error("Client {} has wrong scripts version: {} (server: {})",
                      msg.fromId,
                      clientVersion,
                      SCRIPTS_VERSION);

        auto reject =
            NetworkMessage::createDisconnect("Wrong game version. Please update your game!");
        sendToClient(msg.fromId, reject);
        removeClient(msg.fromId);
        return;
    }

    std::lock_guard<std::mutex> lock(clientsMutex_);
    if (clientInfos_.count(msg.fromId)) {
        clientInfos_[msg.fromId].name = playerName;
        clientInfos_[msg.fromId].status = PlayerStatus::Connected;
        clientInfos_[msg.fromId].scriptsVersion = clientVersion;
    }

    spdlog::info(
        "Player {} connected as {} (scripts version: {})", msg.fromId, playerName, clientVersion);

    auto ack = NetworkMessage::createHandshakeAck(msg.fromId, gameStarted_);
    sendToClient(msg.fromId, ack);

    for (const auto& [id, otherInfo] : clientInfos_) {
        if (id != msg.fromId) {
            auto joined = NetworkMessage::createPlayerJoined(id, otherInfo.name);
            sendToClient(msg.fromId, joined);
        }
    }

    auto joined = NetworkMessage::createPlayerJoined(msg.fromId, playerName);
    broadcast(joined, msg.fromId);

    if (gameStarted_) {
        auto snapshot = NetworkMessage::createSnapshot(model_.toJson());
        sendToClient(msg.fromId, snapshot);
    }
}

void HostServer::handlePlayerReady(const NetworkMessage& msg) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    if (clientInfos_.count(msg.fromId)) {
        clientInfos_[msg.fromId].status = PlayerStatus::Ready;
        spdlog::info("Player {} is ready", clientInfos_[msg.fromId].name);
    }

    auto readyMsg = NetworkMessage::createPlayerReady(msg.fromId);
    broadcast(readyMsg);

    if (onClientReady_) {
        onClientReady_(msg.fromId);
    }
}

void HostServer::handleEvent(const NetworkMessage& msg) {
    if (!gameStarted_) {
        spdlog::warn("Event received but game not started");
        return;
    }

    std::string objectId = msg.data.value("object_id", "");
    std::string eventName = msg.data.value("event", "");

    if (!isEventAllowedForClient(eventName, msg.fromId)) {
        spdlog::warn("Client {} tried to call forbidden event: {}", msg.fromId, eventName);
        return;
    }

    auto obj = model_.getObject(objectId);
    if (!obj) {
        spdlog::warn("Event target object not found: {}", objectId);
        return;
    }

    spdlog::info("Event from {}: {} on {}", msg.fromId, eventName, objectId);

    actionManager_.saveSnapshot(model_);

    lua_.fireEvent(eventName, obj.get());

    broadcast(msg);
}

void HostServer::handleMoveObject(const NetworkMessage& msg) {
    if (!gameStarted_) {
        spdlog::warn("MoveObject received but game not started");
        return;
    }

    std::string objectId = msg.data.value("objectId", "");
    float x = msg.data.value("x", 0.0f);
    float y = msg.data.value("y", 0.0f);

    spdlog::info("MoveObject from {}: {} to ({}, {})", msg.fromId, objectId, x, y);

    core::MoveObjectAction action(objectId, sf::Vector2f(x, y));

    if (action.canExecute(model_)) {
        actionManager_.saveSnapshot(model_);
        action.execute(model_);
        broadcast(msg);
    } else {
        spdlog::warn("MoveObject rejected: {} cannot be moved", objectId);
    }
}

void HostServer::handleChat(const NetworkMessage& msg) {
    std::string text = msg.data.value("text", "");
    spdlog::info("Chat from {}: {}", msg.fromId, text);

    broadcast(msg);

    if (onChatReceived_) {
        onChatReceived_(msg.fromId, text);
    }
}

void HostServer::handlePing(const NetworkMessage& msg) {
    auto pong = NetworkMessage::createPong();
    sendToClient(msg.fromId, pong);

    std::lock_guard<std::mutex> lock(clientsMutex_);
    if (clientInfos_.count(msg.fromId)) {
        clientInfos_[msg.fromId].lastPing = std::chrono::steady_clock::now();
    }
}

bool HostServer::isEventAllowedForClient(const std::string& eventName,
                                         const std::string& clientId) {
    return allowedEvents_.count(eventName) > 0;
}

void HostServer::sendToClient(const std::string& clientId, const NetworkMessage& msg) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    auto it = clients_.find(clientId);
    if (it != clients_.end()) {
        auto data = msg.serialize();
        it->second->send(data.data(), data.size());
    }
}

void HostServer::broadcast(const NetworkMessage& msg, const std::string& excludeId) {
    auto data = msg.serialize();
    std::lock_guard<std::mutex> lock(clientsMutex_);
    for (const auto& [id, socket] : clients_) {
        if (id != excludeId) {
            socket->send(data.data(), data.size());
        }
    }
}

void HostServer::removeClient(const std::string& clientId) {
    std::lock_guard<std::mutex> lock(clientsMutex_);

    if (clientInfos_.count(clientId)) {
        spdlog::info("Client disconnected: {}", clientInfos_[clientId].name);
    }

    clients_.erase(clientId);
    clientInfos_.erase(clientId);

    auto left = NetworkMessage::createPlayerLeft(clientId);
    broadcast(left);

    if (onClientLeft_) {
        onClientLeft_(clientId);
    }
}

void HostServer::update() {
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - lastPingTime_).count();

    if (elapsed >= pingInterval_) {
        auto ping = NetworkMessage::createPing();
        broadcast(ping);
        lastPingTime_ = now;
    }

    checkTimeouts();

    if (gameStarted_) {
        float snapshotElapsed = std::chrono::duration<float>(now - lastBroadcastTime_).count();
        if (snapshotElapsed >= broadcastInterval_) {
            broadcastSnapshot();
            lastBroadcastTime_ = now;
        }
    }
}

void HostServer::checkTimeouts() {
    auto now = std::chrono::steady_clock::now();
    std::vector<std::string> timedOut;

    std::lock_guard<std::mutex> lock(clientsMutex_);
    for (const auto& [id, info] : clientInfos_) {
        float elapsed = std::chrono::duration<float>(now - info.lastPing).count();
        if (elapsed > timeoutInterval_) {
            timedOut.push_back(id);
        }
    }

    for (const auto& id : timedOut) {
        spdlog::warn("Client {} timed out", id);
        removeClient(id);
    }
}

void HostServer::broadcastSnapshot() {
    auto snapshot = NetworkMessage::createSnapshot(model_.toJson());
    broadcast(snapshot);
}

void HostServer::startGame() {
    if (gameStarted_) {
        return;
    }
    gameStarted_ = true;

    auto startMsg = NetworkMessage::createStartGame();
    broadcast(startMsg);

    broadcastSnapshot();

    if (onGameStarted_) {
        onGameStarted_();
    }

    spdlog::info("Game started!");
}

void HostServer::kickClient(const std::string& clientId) {
    auto kick = NetworkMessage::createDisconnect("Kicked by host");
    sendToClient(clientId, kick);
    removeClient(clientId);
}

void HostServer::sendChat(const std::string& text) {
    auto chat = NetworkMessage::createChat(text);
    broadcast(chat);
}

std::string HostServer::getLocalIp() const {
    return sf::IpAddress::getLocalAddress().toString();
}

std::vector<ClientInfo> HostServer::getClients() const {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    std::vector<ClientInfo> result;
    for (const auto& [id, info] : clientInfos_) {
        result.push_back(info);
    }
    return result;
}

std::string HostServer::generateId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);

    const char* hex = "0123456789abcdef";
    std::string id = "client_";
    for (int i = 0; i < 16; ++i) {
        id += hex[dis(gen)];
    }
    return id;
}

void HostServer::setOnClientJoined(std::function<void(const ClientInfo&)> handler) {
    onClientJoined_ = handler;
}

void HostServer::setOnClientLeft(std::function<void(const std::string&)> handler) {
    onClientLeft_ = handler;
}

void HostServer::setOnClientReady(std::function<void(const std::string&)> handler) {
    onClientReady_ = handler;
}

void HostServer::setOnGameStarted(std::function<void()> handler) {
    onGameStarted_ = handler;
}

void HostServer::setOnChatReceived(
    std::function<void(const std::string&, const std::string&)> handler) {
    onChatReceived_ = handler;
}

} // namespace dice::network
