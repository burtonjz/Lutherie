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

#ifndef __GUI_CONNECTION_MANAGER_HPP_
#define __GUI_CONNECTION_MANAGER_HPP_

#include "interfaces/ISocketLookup.hpp"
#include "requests/ConnectionRequest.hpp"
#include "util/SocketSpec.hpp"

#include <QObject>
#include <QGraphicsScene>
#include <vector>

using Connections = std::vector<ConnectionRequest> ; 
class ConnectionManager: public QObject {
    Q_OBJECT
private:
    Connections connections_ ;
    ISocketLookup* socketLookup_ ;

    explicit ConnectionManager(QObject* parent = nullptr);

public:
    static ConnectionManager* instance();

    ConnectionManager(const ConnectionManager&) = delete ;
    ConnectionManager& operator=(const ConnectionManager&) = delete ;
    ConnectionManager(ConnectionManager&&) = delete ;
    ConnectionManager& operator=(ConnectionManager&&) = delete ;

    // convenient connection lookups
    bool hasExternalConnections(const SocketSpec& spec) const ;

    bool hasModulationConnections(const ConnectionEndpoint& endpoint) const ;
    bool hasModulationDepthConnections(const ConnectionEndpoint& endpoint) const ;

    Connections getConnectionsMatchingEndpoint(const ConnectionEndpoint& endpoint) const ;
    size_t getNumConnectionsMatchingEndpoint(const ConnectionEndpoint& endpoint) const ;
    
    Connections getConnectionsMatchingEndpoints(
        const ConnectionEndpoint& outbound,
        const ConnectionEndpoint& inbound
    ) const ;
    size_t getNumConnectionsMatchingEndpoints(
        const ConnectionEndpoint& outbound,
        const ConnectionEndpoint& inbound
    ) const ;

    Connections getConnectionsMatchingSpec(const SocketSpec& spec) const ;
    size_t getNumConnectionsMatchingSpec(const SocketSpec& spec) const ;

    Connections getConnectionsMatchingSpecs(
        const SocketSpec& outbound, 
        const SocketSpec& inbound
    ) const ;
    size_t getNumConnectionsMatchingSpecs(
        const SocketSpec& outbound, 
        const SocketSpec& inbound
    ) const ;
    


    void requestConnectionEvent(
        const ConnectionRequest& req
    );

    void requestConnectionEvent(
        const ConnectionEndpoint& outbound, 
        const ConnectionEndpoint& inbound, 
        bool remove = false, bool depth = false
    ); 
    void requestConnectionEvent(
        const SocketSpec& outbound, 
        const SocketSpec& inbound, 
        bool remove = false, bool depth = false
    ); 

private:
    bool connectionExists(const ConnectionRequest& req) const ;
    
    void addConnection(const ConnectionRequest& req);
    void removeConnection(const ConnectionRequest& req);

private slots:
    void onControlMessageReceived(const json& json);

signals:
    void connectionAdded(const ConnectionRequest& req);
    void connectionRemoved(const ConnectionRequest& req);

};

#endif // __GUI_CONNECTION_MANAGER_HPP_