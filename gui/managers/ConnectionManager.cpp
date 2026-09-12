/*
 * Copyright (C) 2025 Jared Burton
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "managers/ConnectionManager.hpp"
#include "requests/ConnectionRequest.hpp"
#include "api/ControlApiClient.hpp"

#include <QGraphicsItem>
#include <algorithm>
#include <spdlog/spdlog.h>


ConnectionManager* ConnectionManager::instance(){
    static ConnectionManager manager ;
    return &manager ;
}

ConnectionManager::ConnectionManager(QObject* parent): 
    QObject(parent)
{
    connect(
        ControlApiClient::instance(), &ControlApiClient::dataReceived, 
        this, &ConnectionManager::onControlMessageReceived
    );
}

bool ConnectionManager::hasExternalConnections(const SocketSpec& spec) const {
    auto ids = spec.componentIds();

    auto isExternal = [&](const ConnectionEndpoint& endpoint){
        if ( !endpoint.componentId().has_value() ) return true ;
        return !ids.contains(endpoint.componentId().value());
    };

    for ( const auto& c : connections_ ){
        for ( const auto& e : spec.endpoints() ){
            if ( c.inbound() == e  && isExternal(c.outbound()) ) return true ;
            if ( c.outbound() == e && isExternal(c.inbound())  ) return true ;
        }
    }
    return false ;
}

bool ConnectionManager::hasModulationConnections(
    const ConnectionEndpoint& endpoint
) const {
    if ( endpoint.socket() != SocketType::ModulationInbound ){
        SPDLOG_WARN("cannot search modulation connections on a socket that doesn't match");
        return false ;
    }

    for ( const auto& c : connections_ ){
        if ( c.partialMatch(endpoint) && !c.modulatingDepth() ) return true ;
    }
    return false ;
}

bool ConnectionManager::hasModulationDepthConnections(
    const ConnectionEndpoint& endpoint
) const {
    if ( endpoint.socket() != SocketType::ModulationInbound ){
        SPDLOG_WARN("cannot search modulation connections on a socket that doesn't match");
        return false ;
    }

    for ( const auto& c : connections_ ){
        if ( c.partialMatch(endpoint) && c.modulatingDepth() ) return true ;
    }
    return false ;
}

Connections ConnectionManager::getConnectionsMatchingEndpoint(
    const ConnectionEndpoint& endpoint
) const {
    Connections requests ;
    for ( const auto& c : connections_ ){
        if ( c.partialMatch(endpoint) ) requests.push_back(c);
    }
    return requests ;
}

size_t ConnectionManager::getNumConnectionsMatchingEndpoint(
    const ConnectionEndpoint& endpoint
) const {
    size_t count = 0 ;
    for ( const auto& c : connections_ ){
        if ( c.partialMatch(endpoint) ) ++count ;
    }
    return count ;
}   

Connections ConnectionManager::getConnectionsMatchingEndpoints(
    const ConnectionEndpoint& outbound, 
    const ConnectionEndpoint& inbound
) const {
    Connections requests ;

    if ( outbound.socket().isInbound() ){
        SPDLOG_WARN("match function recieved inbound endpoint as outbound argument.");
        return requests ;
    }

    if ( !inbound.socket().isInbound() ){
        SPDLOG_WARN("match function recieved outbound endpoint as inbound argument.");
        return requests ;
    }

    for ( const auto& c : connections_ ){
        if ( c.inbound() == inbound && c.outbound() == outbound ) requests.push_back(c);
    }
    return requests ;
}

size_t ConnectionManager::getNumConnectionsMatchingEndpoints(
    const ConnectionEndpoint& outbound,
    const ConnectionEndpoint& inbound
) const {
    size_t count = 0 ;

    if ( outbound.socket().isInbound() ){
        SPDLOG_WARN("match function recieved inbound endpoint as outbound argument.");
        return 0 ;
    }

    if ( !inbound.socket().isInbound() ){
        SPDLOG_WARN("match function recieved outbound endpoint as inbound argument.");
        return 0 ;
    }

    for ( const auto& c : connections_ ){
        if ( c.inbound() == inbound && c.outbound() == outbound ) ++count ;
    }
    return count ;
}   

Connections ConnectionManager::getConnectionsMatchingSpec(
    const SocketSpec& spec
) const {
    Connections requests ;
    for ( const auto& c : connections_ ){
        bool match = false ;
        for ( const auto& endpoint : spec.endpoints() ){
            if ( c.partialMatch(endpoint) ){
                match = true ;
                break ;
            }
        }
        if ( match ) requests.push_back(c);
    }
    return requests ;
}

Connections ConnectionManager::getConnectionsMatchingSpecs(
    const SocketSpec& outbound, 
    const SocketSpec& inbound
) const {
    Connections requests ;

    if ( outbound.type().isInbound() ){
        SPDLOG_WARN("match function recieved inbound SocketSpec as outbound argument.");
        return requests ;
    }

    if ( !inbound.type().isInbound() ){
        SPDLOG_WARN("match function recieved outbound SocketSpec as inbound argument.");
        return requests ;
    }

    for ( const auto& c : connections_ ){
        auto matches = [&]() {
            for (const auto& out : outbound.endpoints())
                for (const auto& in : inbound.endpoints())
                    if (c.inbound() == in && c.outbound() == out)
                        return true ;
            return false;
        };
        if ( matches() ) requests.push_back(c);
    }
    return requests ;
}

size_t ConnectionManager::getNumConnectionsMatchingSpec(
    const SocketSpec& spec
) const {
    size_t count = 0 ;
    for ( const auto& endpoint : spec.endpoints() ){
        for ( const auto& c : connections_ ){
            if ( c.partialMatch(endpoint) ) ++count ;
        }
    }
    return count ;
}

size_t ConnectionManager::getNumConnectionsMatchingSpecs(
    const SocketSpec& outbound, 
    const SocketSpec& inbound 
) const {
    if ( outbound.type().isInbound() ){
        SPDLOG_WARN("match function recieved inbound SocketSpec as outbound argument.");
        return 0 ;
    }

    if ( !inbound.type().isInbound() ){
        SPDLOG_WARN("match function recieved outbound SocketSpec as inbound argument.");
        return 0 ;
    }

    size_t count = 0 ;
    for ( const auto& c : connections_ ){
        auto matches = [&]() {
            for (const auto& out : outbound.endpoints())
                for (const auto& in : inbound.endpoints())
                    if (c.inbound() == in && c.outbound() == out)
                        return true ;
            return false;
        };
        if ( matches() ) ++count ;
    }
    return count ;
}

void ConnectionManager::requestConnectionEvent(const ConnectionRequest& req){
    if ( !req.valid() ){
        SPDLOG_WARN("Invalid connection request created. Cancelling connection. {}", json(req).dump());
        return ;
    }

    auto obj = req ;
    ControlApiClient::instance()->sendMessage(obj);
}

void ConnectionManager::requestConnectionEvent(
        const ConnectionEndpoint& outbound, 
        const ConnectionEndpoint& inbound, 
        bool remove, bool depth
    ){
    ConnectionRequest req(outbound, inbound, remove, depth);
    requestConnectionEvent(req);
}

void ConnectionManager::requestConnectionEvent(
    const SocketSpec& outbound, 
    const SocketSpec& inbound, 
    bool remove, bool depth
){
    if ( outbound.type().isInbound() ){
        SPDLOG_WARN("recieved inbound SocketSpec as outbound argument. Rejecting request.");
        return ;
    }

    if ( !inbound.type().isInbound() ){
        SPDLOG_WARN("recieved outbound SocketSpec as inbound argument. Rejecting request.");
        return ;
    }

    for ( const auto& out : outbound.endpoints() ){
        for ( const auto& in : inbound.endpoints() ){
            requestConnectionEvent(out, in, remove, depth);
        }
    }
}

bool ConnectionManager::connectionExists(const ConnectionRequest& request) const {
    auto it = std::find(connections_.begin(), connections_.end(), request);
    return it != connections_.end() ;
}

void ConnectionManager::onControlMessageReceived(const json& msg){
    QString action = QString::fromStdString(msg.at("action")) ;
    bool success = msg.at("status") == "success" ;
    if ( ! success ) return ;

    // all requests we care about are connection requests
    std::optional<ConnectionRequest> req ;
    try {
        req = msg ;
    } catch (std::exception& e){
        return ;
    }
    
    if ( req->remove() ){
        removeConnection(*req);
        return ;
    }

    addConnection(*req);
}

void ConnectionManager::addConnection(const ConnectionRequest& req){
    if ( connectionExists(req) ){
        SPDLOG_WARN(
            "requested connection is already present in connection manager."
            "This suggests this client is out of sync."
        );
        return ;
    }
    connections_.push_back(req);
    emit connectionAdded(req);
}

void ConnectionManager::removeConnection(const ConnectionRequest& req){
    if ( !connectionExists(req) ){
        SPDLOG_WARN(
            "requested connection removal does not exist in "
            "the connection manager. This suggests this client is out of sync"
        );
        return ;
    }
    connections_.erase(std::remove(
        connections_.begin(), connections_.end(), req),
        connections_.end()
    );
    emit connectionRemoved(req);
}
