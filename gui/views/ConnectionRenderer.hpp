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

#ifndef CONNECTION_RENDERER_HPP_
#define CONNECTION_RENDERER_HPP_

#include <QObject>
#include <QGraphicsScene>
#include "graphics/ConnectionCable.hpp"
#include "graphics/GraphNode.hpp"
#include "requests/ConnectionRequest.hpp"

class ConnectionRenderer : public QObject {
    Q_OBJECT
private:
    QGraphicsScene* scene_ ;

    // dragging new cable
    ConnectionCable* dragCable_ ;
    SocketWidget* dragFromSocket_ ;

    std::vector<ConnectionCable*> cables_ ;
    std::unordered_map<ConnectionRequest, ConnectionCable*, ConnectionRequestHash> req2Cable_ ;

public:
    explicit ConnectionRenderer(
        QGraphicsScene* scene,
        QObject* parent = nullptr 
    );

    // cable drag
    void startDrag(SocketWidget* fromSocket);
    void updateDrag(const QPointF& scenePos);
    void finishDrag(const QPointF& scenePos);
    void cancelDrag();
    bool isDragging() const ;

    // cable management
    const std::vector<ConnectionCable*> getNodeConnections(GraphNode* node) const ;
    const std::vector<ConnectionCable*> getSocketConnections(SocketWidget* socket) const ;

private:
    struct ModulationParameter {
        std::optional<ConnectionEndpoint> endpoint = std::nullopt ;
        bool depth = false ;
    };

    bool cableHasConnections(ConnectionCable* cable) const ;
    void setCableVisibility(ConnectionCable* cable);

    ConnectionCable* registerCable(ConnectionCable* candidate);
    void findOrCreateCable(const ConnectionRequest& req);
    void deleteCable(ConnectionCable* cable);
    ModulationParameter requestModulationParameter(SocketWidget* socket);

public slots:
    void onSocketPositionChanged(SocketWidget* socket); 
    void onSocketVisibilityChanged(SocketWidget* socket);  
    void onSocketMappingChanged();

    void onConnectionAdded(const ConnectionRequest& req);
    void onConnectionRemoved(const ConnectionRequest& req);

};

#endif // CONNECTION_RENDERER_HPP_