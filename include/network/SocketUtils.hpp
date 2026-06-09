#pragma once

#include <cstdint>
#include <vector>

#include <SFML/Network.hpp>

namespace dice::network {

inline sf::Socket::Status sendAll(sf::TcpSocket& socket, const std::vector<uint8_t>& data) {
    std::size_t offset = 0;
    while (offset < data.size()) {
        std::size_t sent = 0;
        sf::Socket::Status status = socket.send(data.data() + offset, data.size() - offset, sent);
        if (status == sf::Socket::Done || status == sf::Socket::Partial) {
            offset += sent;
        } else {
            return status;
        }
    }
    return sf::Socket::Done;
}

} // namespace dice::network
