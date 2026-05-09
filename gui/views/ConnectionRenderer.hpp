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
#include "managers/ConnectionManager.hpp"
#include "interfaces/ISocketLookup.hpp"
#include "graphics/ConnectionCable.hpp"
#include "graphics/GraphNode.hpp"

class ConnectionRenderer : public QObject {
    Q_OBJECT
private:
    QGraphicsScene* scene_ ;
    ConnectionManager* manager_ ;
    ISocketLookup* socketLookup_ ;

    // dragging new cable
    ConnectionCable* dragCable_ ;
    SocketWidget* dragFromSocket_ ;

    std::vector<ConnectionCable*> cables_ ;
    std::set<SocketWidget*> socketsQueuedForRemoval_ ;

public:
    explicit ConnectionRenderer(
        QGraphicsScene* scene,
        ConnectionManager* manager,
        ISocketLookup* socketLookup,
        QObject* parent = nullptr 
    );

    // cable drag
    void startDrag(SocketWidget* fromSocket);
    void updateDrag(const QPointF& scenePos);
    void finishDrag(const QPointF& scenePos);
    void cancelDrag();
    bool isDragging() const ;
    void setDragCableParameter(ParameterType p, bool depth = false); 

    // cable management
    const std::vector<ConnectionCable*> getNodeConnections(GraphNode* node) const ;
    const std::vector<ConnectionCable*> getSocketConnections(SocketWidget* socket) const ;
    void requestRemoveConnection(ConnectionCable* cable);
    void requestRemoveSocketConnections(SocketWidget* s);
    void requestRemoveSocket(SocketWidget* s);

    void onComponentGroup(const std::unordered_set<int>& componentIds);

private:
    void sendDragCableRequest();
    bool socketIsRemovable(SocketWidget* s, bool request = false );

signals:
    void dragCableParameterNeeded(SocketWidget* socket);
    void canRemoveSocket(SocketWidget* socket);

public slots:
    void onNodePositionChanged(); 
    void onSocketHidden(SocketWidget* socket);
    void onSocketUnhidden(SocketWidget* socket);

    void onConnectionAdded(const ConnectionRequest& req);
    void onConnectionRemoved(const ConnectionRequest& req);

};

#endif // CONNECTION_RENDERER_HPP_