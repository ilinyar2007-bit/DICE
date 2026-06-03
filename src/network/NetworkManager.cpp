#include "network/NetworkManager.hpp"

#include <spdlog/spdlog.h>

namespace dice::network {

NetworkManager::NetworkManager(core::Model& model,
                               core::ActionManager& actionManager,
                               scripting::LuaScriptEngine& lua)
    : model_(model), actionManager_(actionManager), lua_(lua) {
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

bool NetworkManager::joinGame(const std::string& hostIp,
                              uint16_t port,
                              const std::string& playerName) {
    if (role_ != NetworkRole::SinglePlayer) {
        spdlog::warn("Cannot join game - already in game");
        return false;
    }

    gameClient_ = std::make_unique<GameClient>();

    gameClient_->setModel(&model_);
    gameClient_->setActionManager(&actionManager_);
    gameClient_->setLuaEngine(&lua_);

    gameClient_->setOnConnected(
        [this](const std::string& clientId) { spdlog::info("Connected as {}", clientId); });

    gameClient_->setOnDisconnected([this]() {
        role_ = NetworkRole::SinglePlayer;
        gameClient_.reset();
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

    if (!gameClient_->connect(hostIp, port, playerName)) {
        gameClient_.reset();
        return false;
    }

    role_ = NetworkRole::Client;
    spdlog::info("Joined game as client");
    return true;
}

void NetworkManager::leaveGame() {
    if (hostServer_) {
        hostServer_->stop();
        hostServer_.reset();
    }

    if (gameClient_) {
        gameClient_->disconnect();
        gameClient_.reset();
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

void NetworkManager::sendEvent(const std::string& objectId,
                               const std::string& eventName,
                               std::function<void(bool)> callback) {
    if (hostServer_) {
        auto obj = model_.getObject(objectId);
        if (obj) {
            actionManager_.saveSnapshot(model_);
            lua_.fireEvent(eventName, obj.get());
            hostServer_->broadcast(NetworkMessage::createEvent(objectId, eventName));
        } else if (gameClient_) {
            gameClient_->sendEvent(objectId, eventName, callback);
        } else {
            auto obj = model_.getObject(objectId);
            if (obj) {
                actionManager_.saveSnapshot(model_);
                lua_.fireEvent(eventName, obj.get());
            }
        }
    }
}

void NetworkManager::sendMoveObject(const std::string& objectId, float x, float y) {
    if (hostServer_) {
        hostServer_->handleMoveObject(NetworkMessage::createMoveObject(objectId, x, y));
    } else if (gameClient_) {
        gameClient_->sendMoveObject(objectId, x, y);
    } else {
        core::MoveObjectAction action(objectId, sf::Vector2f(x, y));
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

void NetworkManager::kickPlayer(const std::string& playerId) {
    if (hostServer_) {
        hostServer_->kickClient(playerId);
    }
}

std::vector<ClientInfo> NetworkManager::getPlayers() const {
    if (hostServer_) {
        return hostServer_->getClients();
    }
    return {};
}

void NetworkManager::update() {
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
        sendEvent(id, event, nullptr);
    });
    lua_.registerFunction(
        "send_move", [this](const std::string& id, float x, float y) { sendMoveObject(id, x, y); });
}

void NetworkManager::setOnPlayerJoined(std::function<void(const ClientInfo&)> handler) {
    onPlayerJoined_ = handler;
}

void NetworkManager::setOnPlayerLeft(std::function<void(const std::string&)> handler) {
    onPlayerLeft_ = handler;
}

void NetworkManager::setOnPlayerReady(std::function<void(const std::string&)> handler) {
    onPlayerReady_ = handler;
}

void NetworkManager::setOnGameStarted(std::function<void()> handler) {
    onGameStarted_ = handler;
}

void NetworkManager::setOnChatReceived(
    std::function<void(const std::string&, const std::string&)> handler) {
    onChatReceived_ = handler;
}

} // namespace dice::network
