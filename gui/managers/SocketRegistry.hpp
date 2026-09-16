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

#ifndef SOCKET_REGISTRY_HPP_
#define SOCKET_REGISTRY_HPP_

#include <QObject>
#include <unordered_map>
#include <unordered_set>

#include "types/ConnectionEndpoint.hpp"
#include "util/SocketSpec.hpp"

// forward declarations
class SocketWidget ;
class GraphNode ;
using SocketPriority = size_t ;
class SocketRegistry : public QObject {
    Q_OBJECT

private:

    std::unordered_set<SocketWidget*> registered_ ;
    std::map<size_t, std::unordered_set<SocketWidget*>, std::greater<>> registeredByPriority_ ;

    std::unordered_map<ConnectionEndpoint, SocketWidget*, EndpointHash> endpoint2Socket_ ;
    std::unordered_map<SocketWidget*, std::unordered_set<ConnectionEndpoint, EndpointHash>> socket2Endpoints_ ;

    size_t batchDepth_ = 0 ;
    bool dirty_ = false ;

    explicit SocketRegistry(QObject* parent = nullptr);

public:
    static SocketRegistry* instance();

    SocketRegistry(const SocketRegistry&) = delete ;
    SocketRegistry& operator=(const SocketRegistry&) = delete ;
    SocketRegistry(SocketRegistry&&) = delete ;
    SocketRegistry& operator=(SocketRegistry&&) = delete ;

    // Socket Lookups

    // for each endpoint in spec, find the corresponding registered sockets
    std::set<SocketWidget*> findSockets(const SocketSpec& spec) const ;

    // get sockets claiming the specified endpoint
    SocketWidget* findSocket(const ConnectionEndpoint& endpoint) const ;

    // get visible socket at a particular location on the scene
    SocketWidget* findSocketAt(const QPointF& scenePos) const ;

    void registerSocket(SocketWidget* socket, SocketPriority priority);
    void unregisterSocket(SocketWidget* socket);

    // batching -- hold signals until sets of events (e.g., grouping, deserialization, etc)
    // are fully complete
    void startBatch();
    void endBatch();
    bool isBatching() const ;

private:
    bool isClaimed(const ConnectionEndpoint& endpoint) const ;
    bool evaluateSocketClaims(SocketWidget* socket) const ;
    
    bool claim(SocketWidget* socket);
    void release(SocketWidget* socket);

    void runClaimLoop();

    static constexpr size_t maxIterations = 100 ;

#ifdef DEBUG_BUILD
    void validate() const ;
#endif
    
signals:
    void socketMappingChanged();

};


#endif // SOCKET_REGISTRY_HPP_