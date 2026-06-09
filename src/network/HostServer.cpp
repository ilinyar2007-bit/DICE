#include "network/HostServer.hpp"

#include <random>

#include "network/SocketUtils.hpp"
#include <spdlog/spdlog.h>

namespace dice::network {

HostServer::HostServer(core::Model& model,
                       core::ActionManager& action_manager,
                       scripting::LuaScriptEngine& lua)
    : model_(model), actionManager_(action_manager), lua_(lua) {}

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
    lastPingTime_ = std::chrono::steady_clock::now();
    lastBroadcastTime_ = std::chrono::steady_clock::now();

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
    {
        const std::lock_guard<std::mutex> lock(clientsMutex_);
        for (auto& [id, ctx] : clients_) {
            if (ctx->socket) {
                ctx->socket->disconnect();
            }
        }
        clients_.clear();
    }
    if (serverThread_.joinable()) {
        serverThread_.join();
    }

    spdlog::info("Server stopped");
}

void HostServer::serverLoop() {
    while (isRunning_) {
        acceptNewClients();

        std::vector<std::string> clientsCopy;
        {
            const std::lock_guard<std::mutex> lock(clientsMutex_);
            for (const auto& [id, ctx] : clients_) {
                clientsCopy.push_back(id);
            }
        }

        for (const auto& id : clientsCopy) {
            receiveFromClient(id);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void HostServer::acceptNewClients() {
    auto client = std::make_unique<sf::TcpSocket>();

    if (listener_.accept(*client) == sf::Socket::Done) {
        client->setBlocking(false);

        const std::string clientId = generateId();

        auto ctx = std::make_shared<ClientContext>();
        ctx->socket = std::move(client);
        ctx->info.id = clientId;
        ctx->info.name = "Player_" + std::to_string(clients_.size() + 1);
        ctx->info.ip = ctx->socket->getRemoteAddress().toString();
        ctx->info.port = ctx->socket->getRemotePort();
        ctx->info.status = PlayerStatus::Connecting;
        ctx->info.lastPing = std::chrono::steady_clock::now();

        {
            const std::lock_guard<std::mutex> lock(clientsMutex_);
            clients_[ctx->info.id] = ctx;
        }

        spdlog::info("New client connected: {} ({})", ctx->info.name, ctx->info.ip);

        if (onClientJoined_) {
            onClientJoined_(ctx->info);
        }
    }
}

void HostServer::receiveFromClient(const std::string& client_id) {
    std::shared_ptr<ClientContext> ctx;
    {
        const std::lock_guard<std::mutex> lock(clientsMutex_);
        auto it = clients_.find(client_id);
        if (it == clients_.end()) {
            return;
        }
        ctx = it->second;
    }

    std::vector<uint8_t> chunk(65536);
    std::size_t received = 0;

    if (ctx->socket->receive(chunk.data(), chunk.size(), received) == sf::Socket::Done) {
        chunk.resize(received);
        ctx->receiveBuffer.append(chunk);

        while (auto msg = ctx->receiveBuffer.extract()) {
            msg->fromId = client_id;

            spdlog::debug("Received: {}", msg->toString());

            {
                const std::lock_guard<std::mutex> lock(clientsMutex_);
                if (auto it = clients_.find(client_id); it != clients_.end()) {
                    it->second->info.lastPing = std::chrono::steady_clock::now();
                }
            }

            switch (msg->type) {
                case MessageType::Handshake:
                    handleHandshake(*msg);
                    break;
                case MessageType::PlayerReady:
                    handlePlayerReady(*msg);
                    break;
                case MessageType::Event:
                    handleEvent(*msg);
                    break;
                case MessageType::MoveObject:
                    handleMoveObject(*msg);
                    break;
                case MessageType::Chat:
                    handleChat(*msg);
                    break;
                case MessageType::Ping:
                    handlePing(*msg);
                    break;
                case MessageType::Disconnect:
                    removeClient(client_id);
                    break;
                default:
                    spdlog::warn(
                        "Unknown message type from {}: {}", client_id, static_cast<int>(msg->type));
                    break;
            }
        }
    }
}

void HostServer::handleHandshake(const NetworkMessage& msg) {
    std::string playerName = msg.data.value("playerName", "Unknown");

    {
        const std::lock_guard<std::mutex> lock(clientsMutex_);
        auto it = clients_.find(msg.fromId);
        if (it != clients_.end()) {
            it->second->info.name = playerName;
            it->second->info.status = PlayerStatus::Connected;
        }
    }

    spdlog::info("Player {} connected as {}", msg.fromId, playerName);

    auto ack = NetworkMessage::createHandshakeAck(msg.fromId, gameStarted_);
    sendToClient(msg.fromId, ack);

    auto joined = NetworkMessage::createPlayerJoined(msg.fromId, playerName);
    broadcast(joined, msg.fromId);

    if (gameStarted_) {
        nlohmann::json state;
        {
            const std::lock_guard<std::mutex> modelLock(modelMutex_);
            state = model_.toJson();
        }
        auto snapshot = NetworkMessage::createSnapshot(state);
        sendToClient(msg.fromId, snapshot);
    }
}

void HostServer::handlePlayerReady(const NetworkMessage& msg) {
    {
        const std::lock_guard<std::mutex> lock(clientsMutex_);
        auto it = clients_.find(msg.fromId);
        if (it != clients_.end()) {
            it->second->info.status = PlayerStatus::Ready;
            spdlog::info("Player {} is ready", it->second->info.name);
        }
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

    if (!isEventAllowedForClient(eventName)) {
        spdlog::warn("Client {} tried to call forbidden event: {}", msg.fromId, eventName);
        return;
    }

    spdlog::info("Event from {}: {} on {}", msg.fromId, eventName, objectId);

    {
        const std::lock_guard<std::mutex> modelLock(modelMutex_);
        auto obj = model_.getObject(objectId);
        if (!obj) {
            spdlog::warn("Event target object not found: {}", objectId);
            return;
        }
        actionManager_.saveSnapshot(model_);
        lua_.fireEvent(eventName, obj.get());
    }

    broadcast(msg);
}

void HostServer::handleMoveObject(const NetworkMessage& msg) {
    if (!gameStarted_) {
        spdlog::warn("MoveObject received but game not started");
        return;
    }

    std::string objectId = msg.data.value("objectId", "");
    float x = msg.data.value("x", 0.0F);
    float y = msg.data.value("y", 0.0F);

    spdlog::info("MoveObject from {}: {} to ({}, {})", msg.fromId, objectId, x, y);

    core::MoveObjectAction action(objectId, sf::Vector2f(x, y));

    bool executed = false;
    {
        const std::lock_guard<std::mutex> modelLock(modelMutex_);
        if (action.canExecute(model_)) {
            actionManager_.saveSnapshot(model_);
            action.execute(model_);
            executed = true;
        }
    }

    if (executed) {
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

    const std::lock_guard<std::mutex> lock(clientsMutex_);
    auto it = clients_.find(msg.fromId);
    if (it != clients_.end()) {
        it->second->info.lastPing = std::chrono::steady_clock::now();
    }
}

bool HostServer::isEventAllowedForClient(const std::string& event_name) {
    return allowedEvents_.contains(event_name);
}

void HostServer::sendToClient(const std::string& client_id, const NetworkMessage& msg) {
    std::shared_ptr<ClientContext> ctx;
    {
        const std::lock_guard<std::mutex> lock(clientsMutex_);
        auto it = clients_.find(client_id);
        if (it == clients_.end()) {
            return;
        }
        ctx = it->second;
    }

    auto data = msg.serialize();
    auto status = sendAll(*ctx->socket, data);
    if (status != sf::Socket::Done) {
        spdlog::warn(
            "Failed to send to client {}, status: {}", client_id, static_cast<int>(status));
    }
}

void HostServer::broadcast(const NetworkMessage& msg, const std::string& exclude_id) {
    auto data = msg.serialize();

    const std::lock_guard<std::mutex> lock(clientsMutex_);
    for (const auto& [id, ctx] : clients_) {
        if (id == exclude_id) {
            continue;
        }
        auto status = sendAll(*ctx->socket, data);
        if (status != sf::Socket::Done) {
            spdlog::warn(
                "Broadcast failed for client {}, status: {}", id, static_cast<int>(status));
        }
    }
}

void HostServer::removeClient(const std::string& client_id) {
    std::shared_ptr<ClientContext> ctx;
    {
        const std::lock_guard<std::mutex> lock(clientsMutex_);
        auto it = clients_.find(client_id);
        if (it != clients_.end()) {
            ctx = it->second;
            clients_.erase(it);
        }
    }

    if (ctx) {
        spdlog::info("Client disconnected: {}", ctx->info.name);

        auto left = NetworkMessage::createPlayerLeft(client_id);
        broadcast(left);

        if (onClientLeft_) {
            onClientLeft_(client_id);
        }
    }
}

void HostServer::update() {
    auto now = std::chrono::steady_clock::now();
    const float elapsed = std::chrono::duration<float>(now - lastPingTime_).count();

    if (elapsed >= pingInterval_) {
        auto ping = NetworkMessage::createPing();
        broadcast(ping);
        lastPingTime_ = now;
    }

    checkTimeouts();

    if (gameStarted_) {
        const float snapshotElapsed =
            std::chrono::duration<float>(now - lastBroadcastTime_).count();
        if (snapshotElapsed >= broadcastInterval_) {
            broadcastSnapshot();
            lastBroadcastTime_ = now;
        }
    }
}

void HostServer::checkTimeouts() {
    auto now = std::chrono::steady_clock::now();
    std::vector<std::string> timedOut;

    {
        const std::lock_guard<std::mutex> lock(clientsMutex_);
        for (const auto& [id, ctx] : clients_) {
            const float elapsed = std::chrono::duration<float>(now - ctx->info.lastPing).count();
            if (elapsed > timeoutInterval_) {
                timedOut.push_back(id);
            }
        }
    }

    for (const auto& id : timedOut) {
        spdlog::warn("Client {} timed out", id);
        removeClient(id);
    }
}

void HostServer::broadcastEvent(const std::string& object_id, const std::string& event_name) {
    broadcast(NetworkMessage::createEvent(object_id, event_name));
}

void HostServer::broadcastMoveObject(const std::string& object_id, float x, float y) {
    broadcast(NetworkMessage::createMoveObject(object_id, x, y));
}

void HostServer::broadcastSnapshot() {
    nlohmann::json state;
    {
        const std::lock_guard<std::mutex> modelLock(modelMutex_);
        state = model_.toJson();
    }
    broadcast(NetworkMessage::createSnapshot(state));
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

void HostServer::kickClient(const std::string& client_id) {
    auto kick = NetworkMessage::createDisconnect("Kicked by host");
    sendToClient(client_id, kick);
    removeClient(client_id);
}

void HostServer::sendChat(const std::string& text) {
    auto chat = NetworkMessage::createChat(text);
    broadcast(chat);
}

std::string HostServer::getLocalIp() {
    return sf::IpAddress::getLocalAddress().toString();
}

std::vector<ClientInfo> HostServer::getClients() const {
    const std::lock_guard<std::mutex> lock(clientsMutex_);
    std::vector<ClientInfo> result;
    result.reserve(clients_.size());
    for (const auto& [id, ctx] : clients_) {
        result.push_back(ctx->info);
    }
    return result;
}

std::string HostServer::generateId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::mutex genMutex;
    static std::uniform_int_distribution<> dis(0, 15);

    const std::lock_guard<std::mutex> lock(genMutex);
    const std::string hex = "0123456789abcdef";
    std::string id = "client_";
    for (int i = 0; i < 16; ++i) {
        id += hex[dis(gen)];
    }
    return id;
}

void HostServer::setOnClientJoined(std::function<void(const ClientInfo&)> handler) {
    onClientJoined_ = std::move(handler);
}

void HostServer::setOnClientLeft(std::function<void(const std::string&)> handler) {
    onClientLeft_ = std::move(handler);
}

void HostServer::setOnClientReady(std::function<void(const std::string&)> handler) {
    onClientReady_ = std::move(handler);
}

void HostServer::setOnGameStarted(std::function<void()> handler) {
    onGameStarted_ = std::move(handler);
}

void HostServer::setOnChatReceived(
    std::function<void(const std::string&, const std::string&)> handler) {
    onChatReceived_ = std::move(handler);
}

} // namespace dice::network
