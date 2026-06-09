#include "network/NetworkManager.hpp"

#include <spdlog/spdlog.h>

namespace dice::network {

NetworkManager::NetworkManager(core::Model& model,
                               core::ActionManager& action_manager,
                               scripting::LuaScriptEngine& lua)
    : model_(model), actionManager_(action_manager), lua_(lua) {
    registerLuaBindings();
}

NetworkManager::~NetworkManager() {
    leaveGame();
}

bool NetworkManager::startHost(uint16_t port) {
    if (role_ != NetworkRole::SinglePlayer) {
        spdlog::warn("Cannot start host - already in game");
        return false;
    }

    hostServer_ = std::make_unique<HostServer>(model_, actionManager_, lua_);

    hostServer_->setOnClientJoined([this](const ClientInfo& info) {
        if (onPlayerJoined_) {
            onPlayerJoined_(info);
        }
    });

    hostServer_->setOnClientLeft([this](const std::string& id) {
        if (onPlayerLeft_) {
            onPlayerLeft_(id);
        }
    });

    hostServer_->setOnClientReady([this](const std::string& id) {
        if (onPlayerReady_) {
            onPlayerReady_(id);
        }
    });

    hostServer_->setOnGameStarted([this]() {
        if (onGameStarted_) {
            onGameStarted_();
        }
    });

    hostServer_->setOnChatReceived([this](const std::string& from, const std::string& text) {
        if (onChatReceived_) {
            onChatReceived_(from, text);
        }
    });

    if (!hostServer_->start(port)) {
        hostServer_.reset();
        return false;
    }

    role_ = NetworkRole::Host;
    spdlog::info("Started as host on port {}", port);
    return true;
}

bool NetworkManager::joinGame(const std::string& host_ip,
                              uint16_t port,
                              const std::string& player_name) {
    if (role_ != NetworkRole::SinglePlayer) {
        spdlog::warn("Cannot join game - already in game");
        return false;
    }

    gameClient_ = std::make_unique<GameClient>();

    gameClient_->setModel(&model_);
    gameClient_->setActionManager(&actionManager_);
    gameClient_->setLuaEngine(&lua_);

    gameClient_->setOnConnected(
        [this](const std::string& client_id) { spdlog::info("Connected as {}", client_id); });

    gameClient_->setOnDisconnected([this]() {
        role_ = NetworkRole::SinglePlayer;
        pendingClientCleanup_ = true;
        if (onPlayerLeft_) {
            onPlayerLeft_("");
        }
    });

    gameClient_->setOnPlayerJoined([this](const ClientInfo& info) {
        if (onPlayerJoined_) {
            onPlayerJoined_(info);
        }
    });

    gameClient_->setOnPlayerLeft([this](const std::string& id) {
        if (onPlayerLeft_) {
            onPlayerLeft_(id);
        }
    });

    gameClient_->setOnPlayerReady([this](const std::string& id) {
        if (onPlayerReady_) {
            onPlayerReady_(id);
        }
    });

    gameClient_->setOnGameStarted([this]() {
        if (onGameStarted_) {
            onGameStarted_();
        }
    });

    gameClient_->setOnChatReceived([this](const std::string& from, const std::string& text) {
        if (onChatReceived_) {
            onChatReceived_(from, text);
        }
    });

    if (!gameClient_->connect(host_ip, port, player_name)) {
        gameClient_.reset();
        return false;
    }

    role_ = NetworkRole::Client;
    spdlog::info("Joined game as client");
    return true;
}

void NetworkManager::cleanupGameClient() {
    if (pendingClientCleanup_ && gameClient_) {
        gameClient_.reset();
        pendingClientCleanup_ = false;
    }
}

void NetworkManager::leaveGame() {
    if (hostServer_) {
        hostServer_->stop();
        hostServer_.reset();
    }

    if (gameClient_) {
        gameClient_->disconnect();
        gameClient_.reset();
        pendingClientCleanup_ = false;
    }

    role_ = NetworkRole::SinglePlayer;
    spdlog::info("Left game");
}

bool NetworkManager::isConnected() const {
    if (hostServer_) {
        return hostServer_->isRunning();
    }
    if (gameClient_) {
        return gameClient_->isConnected();
    }
    return false;
}

bool NetworkManager::isGameStarted() const {
    if (hostServer_) {
        return hostServer_->isGameStarted();
    }
    if (gameClient_) {
        return gameClient_->isGameStarted();
    }
    return false;
}

void NetworkManager::sendEvent(const std::string& object_id, const std::string& event_name) {
    if (hostServer_) {
        auto obj = model_.getObject(object_id);
        if (obj) {
            actionManager_.saveSnapshot(model_);
            lua_.fireEvent(event_name, obj.get());
            hostServer_->broadcastEvent(object_id, event_name);
        }
    } else if (gameClient_) {
        gameClient_->sendEvent(object_id, event_name);
    } else {
        auto obj = model_.getObject(object_id);
        if (obj) {
            actionManager_.saveSnapshot(model_);
            lua_.fireEvent(event_name, obj.get());
        }
    }
}

void NetworkManager::sendMoveObject(const std::string& object_id, float x, float y) {
    if (hostServer_) {
        core::MoveObjectAction action(object_id, sf::Vector2f(x, y));
        if (action.canExecute(model_)) {
            actionManager_.saveSnapshot(model_);
            action.execute(model_);
            hostServer_->broadcastMoveObject(object_id, x, y);
        }
    } else if (gameClient_) {
        gameClient_->sendMoveObject(object_id, x, y);
    } else {
        core::MoveObjectAction action(object_id, sf::Vector2f(x, y));
        if (action.canExecute(model_)) {
            actionManager_.saveSnapshot(model_);
            action.execute(model_);
        }
    }
}

void NetworkManager::sendReady() {
    if (gameClient_) {
        gameClient_->sendReady();
    }
}

void NetworkManager::sendChat(const std::string& text) {
    if (hostServer_) {
        hostServer_->sendChat(text);
    } else if (gameClient_) {
        gameClient_->sendChat(text);
    }
}

void NetworkManager::startGame() {
    if (hostServer_) {
        hostServer_->startGame();
    }
}

void NetworkManager::kickPlayer(const std::string& player_id) {
    if (hostServer_) {
        hostServer_->kickClient(player_id);
    }
}

std::vector<ClientInfo> NetworkManager::getPlayers() const {
    if (hostServer_) {
        return hostServer_->getClients();
    }
    return {};
}

void NetworkManager::update() {
    cleanupGameClient();
    if (hostServer_) {
        hostServer_->update();
    }
    if (gameClient_) {
        gameClient_->update();
    }
}

void NetworkManager::registerLuaBindings() {
    lua_.registerFunction("is_host", [this]() { return isHost(); });
    lua_.registerFunction("is_client", [this]() { return !isHost() && isConnected(); });
    lua_.registerFunction("send_event", [this](const std::string& id, const std::string& event) {
        sendEvent(id, event);
    });
    lua_.registerFunction(
        "send_move", [this](const std::string& id, float x, float y) { sendMoveObject(id, x, y); });
}

void NetworkManager::setOnPlayerJoined(std::function<void(const ClientInfo&)> handler) {
    onPlayerJoined_ = std::move(handler);
}

void NetworkManager::setOnPlayerLeft(std::function<void(const std::string&)> handler) {
    onPlayerLeft_ = std::move(handler);
}

void NetworkManager::setOnPlayerReady(std::function<void(const std::string&)> handler) {
    onPlayerReady_ = std::move(handler);
}

void NetworkManager::setOnGameStarted(std::function<void()> handler) {
    onGameStarted_ = std::move(handler);
}

void NetworkManager::setOnChatReceived(
    std::function<void(const std::string&, const std::string&)> handler) {
    onChatReceived_ = std::move(handler);
}

} // namespace dice::network
