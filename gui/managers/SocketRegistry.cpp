/*
 * Copyright (C) 2026 Jared Burton
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

#include "managers/SocketRegistry.hpp"
#include "graphics/SocketWidget.hpp"
#include "graphics/GraphNode.hpp"
#include <spdlog/spdlog.h>

SocketRegistry* SocketRegistry::instance(){
    static SocketRegistry manager ;
    return &manager ;
}

SocketRegistry::SocketRegistry(QObject* parent): 
    QObject(parent)
{}

std::set<SocketWidget*> SocketRegistry::findSockets(const SocketSpec& spec) const {
    std::set<SocketWidget*> s ;
    for ( const auto& endpoint : spec.endpoints() ){
        SocketWidget* socket = findSocket(endpoint);
        if ( socket ) s.insert(socket);
    }
    return s ;
}

SocketWidget* SocketRegistry::findSocket(const ConnectionEndpoint& endpoint) const {
    auto it = endpoint2Socket_.find(endpoint);
    if ( it == endpoint2Socket_.end() ) return nullptr ;
    return it->second ;
}

SocketWidget* SocketRegistry::findSocketAt(const QPointF& scenePos) const {
    for ( SocketWidget* socket : registeredSockets_ ){
        QPointF localPos = socket->mapFromScene(scenePos);
        if ( socket->boundingRect().contains(localPos) ) return socket ;
    }
    return nullptr ;
}

void SocketRegistry::registerSocket(SocketWidget* socket){
    if ( !socket ){
        SPDLOG_WARN("Cannot register socket. Null pointer received");
        return ;
    }

    if ( registeredSockets_.contains(socket) ){
        SPDLOG_WARN("socket has already been registered");
        return ;
    }

    registeredSockets_.insert(socket);
}

void SocketRegistry::unregisterSocket(SocketWidget* socket){
    if ( !socket ) return ;

    auto it = socket2Endpoints_.find(socket);
    if ( it != socket2Endpoints_.end() ){
        const std::vector<ConnectionEndpoint> held(it->second.begin(), it->second.end());
        release(held);
    }

    registeredSockets_.erase(socket);
}

void SocketRegistry::registerNodeSockets(GraphNode* node){
    if ( !node ){
        SPDLOG_WARN("Cannot register node. Null pointer received");
        return ;
    }

    startBatch();
    for ( auto* socket : node->getSockets() ){
        registerSocket(socket);
    }
    endBatch();
}

void SocketRegistry::unregisterNodeSockets(GraphNode* node){
    if ( !node ) return ;

    startBatch();
    for ( auto* socket : node->getSockets() ){
        unregisterSocket(socket);
    }
    endBatch();
}

bool SocketRegistry::claim(SocketWidget* socket){
    if ( !socket ){
        SPDLOG_WARN("Cannot claim. Null pointer received");
        return false ;
    }

    if ( !registeredSockets_.contains(socket) ){
        SPDLOG_WARN("Cannot claim with an unregistered socket");
        return false ;
    }

    startBatch();
    for ( const auto& endpoint : socket->getSpec().endpoints() ){
        // if the endpoint is owned by another, clear out its record
        SocketWidget* previous = findSocket(endpoint);
        if ( previous ){
            auto& endpoints = socket2Endpoints_.at(previous);
            endpoints.erase(endpoint);
            if ( endpoints.empty() ){
                socket2Endpoints_.erase(previous);
                previous->setHasClaims(false); 
            }
        }
         
        endpoint2Socket_[endpoint] = socket ;
        socket2Endpoints_[socket].insert(endpoint);
    }
    socket->setHasClaims(true);

    batchDirty_ = true ;
    endBatch();

    return true ;
}

void SocketRegistry::release(std::span<const ConnectionEndpoint> endpoints){
    if ( endpoints.empty() ) return ;

    startBatch();
    for ( const auto& endpoint : endpoints ){
        if ( !endpoint2Socket_.contains(endpoint) ) continue ;
        
        SocketWidget* owner = endpoint2Socket_.at(endpoint);
        endpoint2Socket_.erase(endpoint);

        if ( !owner ) continue ;
        if ( !socket2Endpoints_.contains(owner) ) continue ;

        auto endpoints = socket2Endpoints_.at(owner);
        endpoints.erase(endpoint);
        if ( endpoints.empty() ){
            socket2Endpoints_.erase(owner);
            owner->setHasClaims(false);
        }
            
        batchDirty_ = true ;
    }
    endBatch();
}

bool SocketRegistry::isClaimed(const ConnectionEndpoint& endpoint) const {
    return endpoint2Socket_.contains(endpoint);
}

void SocketRegistry::startBatch(){
    ++batchDepth_ ;
}

void SocketRegistry::endBatch(){
    if ( batchDepth_ == 0 ){
        SPDLOG_WARN("endBatch called without matching startBatch");
        return ;
    }

    // if socketMapping changes trigger reactive claims,
    // batchDirty_ gets reset during the emitted signal
    // while loop keeps it in the loop until no sockets
    // are changed
    while ( batchDirty_ ){
        batchDirty_ = false ;
        emit socketMappingChanged();
    }
}

bool SocketRegistry::isBatching() const {
    return batchDepth_ > 0 ;
}

