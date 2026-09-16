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
    if ( endpoint2Socket_.contains(endpoint) ) return endpoint2Socket_.at(endpoint);
    return nullptr ;
}

SocketWidget* SocketRegistry::findSocketAt(const QPointF& scenePos) const {
    for ( SocketWidget* socket : registered_ ){
        QPointF localPos = socket->mapFromScene(scenePos);
        if ( socket->boundingRect().contains(localPos) && socket->isVisible() ) return socket ;
    }
    return nullptr ;
}

void SocketRegistry::registerSocket(SocketWidget* socket, SocketPriority priority){
    if ( !socket ){
        SPDLOG_WARN("Cannot register socket. Null pointer received");
        return ;
    }

    if ( registered_.contains(socket) ){
        SPDLOG_WARN("socket has already been registered");
        return ;
    }

    registered_.insert(socket);
    registeredByPriority_[priority].insert(socket);

    if ( evaluateSocketClaims(socket) ) claim(socket);
}

void SocketRegistry::unregisterSocket(SocketWidget* socket){
    if ( !socket ) return ;

    registered_.erase(socket);
    registeredByPriority_[socket->priority()].erase(socket);

    release(socket);
}

void SocketRegistry::startBatch(){
    ++batchDepth_ ;
}

void SocketRegistry::endBatch(){
    if ( batchDepth_ == 0 ){
        SPDLOG_WARN("endBatch called without matching startBatch");
        return ;
    }

    --batchDepth_ ;
    runClaimLoop();
}

bool SocketRegistry::isBatching() const {
    return batchDepth_ > 0 ;
}

bool SocketRegistry::isClaimed(const ConnectionEndpoint& endpoint) const {
    return endpoint2Socket_.contains(endpoint);
}

bool SocketRegistry::evaluateSocketClaims(SocketWidget* socket) const {
    if ( !socket ) return false ;

    for ( const auto& endpoint : socket->getSpec().endpoints() ){
        // at least one endpoint unclaimed 
        if ( !isClaimed(endpoint) ) return true ;
        
        SocketWidget* claimer = endpoint2Socket_.at(endpoint);
        if ( claimer == socket ) continue ;

        // or if current socket with claim has lower priority
        if ( socket->priority() > claimer->priority() ) return true ;
    }
    return false ;
}

bool SocketRegistry::claim(SocketWidget* socket){
    if ( !socket ){
        SPDLOG_WARN("Cannot claim. Null pointer received");
        return false ;
    }

    startBatch();
    for ( const auto& endpoint : socket->getSpec().endpoints() ){
        // if the endpoint is owned by another, clear out its record
        SocketWidget* previous = findSocket(endpoint);
        if ( previous == socket ) continue ;
        if ( previous ) release(previous);
          
        endpoint2Socket_[endpoint] = socket ;
        socket2Endpoints_[socket].insert(endpoint);
        dirty_ = true ;
        socket->setHasClaims(true);
    }
    endBatch();

    return true ;
}

void SocketRegistry::release(SocketWidget* socket){
    if ( !socket2Endpoints_.contains(socket) ){
        SPDLOG_WARN("SocketRegistry release called on socket with no claims");
        return ;
    }

    while ( socket2Endpoints_.at(socket).size() > 0 ){
        auto it = socket2Endpoints_.at(socket).begin();

        endpoint2Socket_.erase(*it);
        socket2Endpoints_.at(socket).erase(it);    
    }
    socket2Endpoints_.erase(socket);

    socket->setHasClaims(false);
    dirty_ = true ;
    runClaimLoop();
}

void SocketRegistry::runClaimLoop(){
    if ( isBatching() ) return ;

    // loop through tiers to process claims
    size_t iterations = 0 ;
    auto tier = registeredByPriority_.begin();
    while ( tier != registeredByPriority_.end() ){
        if ( ++iterations > maxIterations ){
            SPDLOG_WARN("claim resolution exceeded {} iterations. Breaking out.", maxIterations);
            break ;
        }

        dirty_ = false ;
        for ( auto* socket : tier->second ){
            if ( evaluateSocketClaims(socket) ) claim(socket);
        }

        // if tier resulted in claim changes, start back at the top
        if ( dirty_ ) tier = registeredByPriority_.begin();
        else ++tier ;
    }

#ifdef DEBUG_BUILD
    validate();
#endif 

    emit socketMappingChanged();
}

#ifdef DEBUG_BUILD
void SocketRegistry::validate() const {
    for ( SocketWidget* socket : registered_ ){
        bool hasClaim = false ;
        bool hasAllClaims = true ;
        for ( const auto& endpoint : socket->getSpec().endpoints() ){
            // a socket's endpoint should always be claimed by somebody
            if ( !isClaimed(endpoint) ){
                SPDLOG_WARN(
                    "SocketRegistry validation: endpoint {} is currently unclaimed after batch resolution.",
                    endpoint.toString()
                );
                hasAllClaims = false ;
                break ;
            }
            
            SocketWidget* claimer = endpoint2Socket_.at(endpoint);

            // competing socket with claim should never have lower priority
            if (  claimer != socket  ){
                if ( socket->priority() > claimer->priority() ){
                    SPDLOG_WARN(
                        "SocketRegistry validation: Endpoint {} is currently claimed by Socket with endpoints {}, "
                        "but Socket with Endpoints {} has higher priority ({} vs {})",
                        endpoint.toString(), claimer->getSpec().toString(), socket->getSpec().toString(), 
                        socket->priority(), claimer->priority()
                    );
                }
                hasAllClaims = false ;
                break ;
            }

            hasClaim = true ;
        }

        // if socket claims one endpoint, it's expected to claim all endpoints
        if ( hasClaim && !hasAllClaims ){
            SPDLOG_WARN(
                "SocketRegistry validation: Socket with endpoints {} has at least one claim, "
                "but does not claim all of its endpoints.", socket->getSpec().toString()
            );
        }
        if ( !hasAllClaims ) continue ;

        // socket->endpoint map should have entry for socket with claim
        if ( !socket2Endpoints_.contains(socket) ){
            SPDLOG_WARN(
                "SocketRegistry validation: Socket with endpoints {} has claims in endpoint2Socket, "
                "but the socket2Endpoint map is not properly populated", socket->getSpec().toString()
            );
            continue ;
        }

        // all claims are validated at this point
        auto endpoints = socket2Endpoints_.at(socket);
        if ( endpoints.size() != socket->getSpec().numEndpoints() ){
            SPDLOG_WARN(
                "SocketRegistry validation: Socket with endpoints {} "
                "does not have the right number of claims in socket2Endpoints."
                , socket->getSpec().toString()
            );
            continue ;
        }

        // all endpoints in socket->endpoints should match endpoint->socket
        for ( const auto& endpoint : endpoints ){
            if ( endpoint2Socket_.at(endpoint) != socket ){
                SPDLOG_WARN(
                    "SocketRegistry validation: endpoint2Socket map does not match "
                    "socket2Endpoint with endpoint {}", endpoint.toString()
                );
            }
        }
    }
}
#endif // DEBUG_BUILD