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

#include "views/ConnectionRenderer.hpp"

#include "graphics/GraphNode.hpp"

ConnectionRenderer::ConnectionRenderer(
    QGraphicsScene* scene,
    ConnectionManager* manager,
    ISocketLookup* socketLookup,
    QObject* parent
):
    QObject(parent),
    scene_(scene),
    manager_(manager),
    socketLookup_(socketLookup),
    dragCable_(nullptr),
    dragFromSocket_(nullptr)
{
    connect(
        manager_, 
        &ConnectionManager::connectionAdded, 
        this, 
        &ConnectionRenderer::onConnectionAdded
    );

    connect(
        manager_, 
        &ConnectionManager::connectionRemoved, 
        this, 
        &ConnectionRenderer::onConnectionRemoved
    );
}

// creating new cables
void ConnectionRenderer::startDrag(SocketWidget* fromSocket){
    if (!fromSocket || isDragging() ) return ;

    fromSocket->grabMouse();
    dragFromSocket_ = fromSocket ;
    dragCable_ = new ConnectionCable(fromSocket);
    scene_->addItem(dragCable_);
    dragCable_->setZValue(1e6); // on top of everything
}

void ConnectionRenderer::updateDrag(const QPointF& scenePos){
    if (dragCable_) dragCable_->setEndpoint(scenePos) ;
}

void ConnectionRenderer::finishDrag(const QPointF& scenePos){
    ConnectionRequest request ;
    
    if ( !dragCable_ || !dragFromSocket_ ){
        qWarning() << "drag connection not available. Unable to finish drag.";
        return ;
    }

    dragFromSocket_->ungrabMouse();

    SocketWidget* toSocket = socketLookup_->findSocketAt(scenePos);
    if ( !toSocket ){
        qInfo() << "No socket endpoint specified for drag cable. Cancelling connection." ;
        cancelDrag();
        return ;
    }
    dragCable_->setToSocket(toSocket);

    if ( !dragCable_->isCompatible(toSocket)){
        cancelDrag();
        return ;
    }

    if ( toSocket->getSpec().type == SocketType::ModulationInbound ){
        emit dragCableParameterNeeded(toSocket);
        return ;
    } else if ( dragFromSocket_->getSpec().type == SocketType::ModulationInbound ){
        emit dragCableParameterNeeded(dragFromSocket_);
        return ;
    } else {
        sendDragCableRequest();
        return ;
    }
}

void ConnectionRenderer::cancelDrag(){
    if ( dragCable_ ){
        scene_->removeItem(dragCable_);
        delete dragCable_ ;
        dragCable_ = nullptr ;
    }
    dragFromSocket_ = nullptr ;
}

bool ConnectionRenderer::isDragging() const {
    return dragCable_ != nullptr ;
}

void ConnectionRenderer::setDragCableParameter(ParameterType p, bool depth){
    dragCable_->setModulatedParameter(p, depth);
    sendDragCableRequest();
}

void ConnectionRenderer::requestRemoveConnection(ConnectionCable* cable){
    if ( !cable ) return ;
    auto req = cable->toConnectionRequest();
    req.remove = true ;
    manager_->requestConnectionEvent(req);
}

void ConnectionRenderer::requestRemoveSocketConnections(SocketWidget* s){
    for ( auto c : cables_ ){
        if ( c->getFromSocket() == s || c->getToSocket() == s ){
            requestRemoveConnection(c);
        }
    }
}

void ConnectionRenderer::requestRemoveSocket(SocketWidget* s){
    if ( socketIsRemovable(s, true) ){
        emit canRemoveSocket(s);
    } else {
        qDebug() << "queuing socket for removal...";
        socketsQueuedForRemoval_.insert(s);
    }
}

const std::vector<ConnectionCable*> ConnectionRenderer::getNodeConnections(GraphNode* node) const {
    std::vector<ConnectionCable*> c ;
    for ( auto cable : cables_ ) {
        if (cable->involvesWidget(node)) c.push_back(cable);
    }
    return c ;
}

const std::vector<ConnectionCable*> ConnectionRenderer::getSocketConnections(SocketWidget* socket) const {
    std::vector<ConnectionCable*> c ;
    for ( auto cable : cables_ ){
        if ( cable->involvesSocket(socket)) c.push_back(cable);
    }
    return c ;
}

void ConnectionRenderer::sendDragCableRequest(){
    ConnectionRequest request = dragCable_->toConnectionRequest();
    manager_->requestConnectionEvent(request);
    cancelDrag(); // destroy temporary cable
}

bool ConnectionRenderer::socketIsRemovable(SocketWidget* s, bool request){
    bool hasConnection = false ;
    for ( auto c: cables_ ){
        if ( c->getFromSocket() == s || c->getToSocket() == s ){
            hasConnection = true ;
            if ( request ){
                requestRemoveConnection(c);
            }
        }
    }
    
    return !hasConnection ;
}

void ConnectionRenderer::onComponentGroup(const std::unordered_set<int>& componentIds){
    for ( auto cable : cables_ ){
        SocketSpec fromSpec = cable->getFromSocket()->getSpec();
        SocketSpec toSpec = cable->getToSocket()->getSpec();
        bool hasFromId = fromSpec.componentId.has_value();
        bool hasToId = toSpec.componentId.has_value();
        if ( !hasFromId && !hasToId ) continue ;
        for ( const auto& id : componentIds ){
            if ( hasFromId && fromSpec.componentId.value() == id ){
                cable->setVisible(true);
                cable->setFromSocket(socketLookup_->findSocket(fromSpec));
            }
            if ( hasToId && toSpec.componentId.value() == id ){
                cable->setVisible(true);
                cable->setToSocket(socketLookup_->findSocket(toSpec));
            }
        }
    }
}
    
void ConnectionRenderer::onNodePositionChanged(){
    GraphNode* widget = dynamic_cast<GraphNode*>(sender());
    if (!widget) return ;
    
    for ( const auto& cable : cables_ ) {
        if (cable->involvesWidget(widget)){
            cable->updatePath();
        }
    }
}

void ConnectionRenderer::onSocketHidden(SocketWidget* socket){
    for ( auto c : cables_ ){
        if ( c->involvesSocket(socket) ){
            c->hide();
        }
    }
}

void ConnectionRenderer::onSocketUnhidden(SocketWidget* socket){
    for ( auto c : cables_ ){
        if ( c->involvesSocket(socket) ){
            SocketWidget* other ;
            if ( socket->isInbound() ){
                other = c->getOutboundSocket();
            } else {
                other = c->getInboundSocket();
            }
            if ( other->isVisible() ){
                c->show();
            }
        }
    }
}

void ConnectionRenderer::onConnectionAdded(const ConnectionRequest& req){
    SocketWidget* outbound = socketLookup_->findSocket({
        .type = req.outboundSocket, 
        .componentId = req.outboundID,
        .idx = req.outboundIdx
    });

    SocketWidget*  inbound = socketLookup_->findSocket({
        .type = req.inboundSocket, 
        .componentId = req.inboundID, 
        .idx = req.inboundIdx, 
    });

    if ( !outbound || !inbound ){
        qWarning() << "did not find sockets to draw connection cable. Please investigate" ;
        return ;
    }

    ConnectionCable* c = new ConnectionCable(outbound, inbound);
    
    if ( inbound->getSpec().type == SocketType::ModulationInbound ){
        c->setModulatedParameter(req.inboundParameter.value(), req.depthConnection);
    }

    cables_.push_back(c);
    scene_->addItem(c);
    c->setZValue(std::max(inbound->zValue(), outbound->zValue()));

    inbound->setConnnection(true);
    outbound->setConnnection(true);
}

void ConnectionRenderer::onConnectionRemoved(const ConnectionRequest& req){
    for ( auto c : cables_ ){
        if ( c->toConnectionRequest() == req ){
            c->getInboundSocket()->setConnnection(false);
            c->getOutboundSocket()->setConnnection(false);

            scene_->removeItem(c);
            delete c ;
            cables_.erase(std::remove(cables_.begin(), cables_.end(), c), cables_.end());
        }
    }

    for ( auto s : socketsQueuedForRemoval_ ){
        qDebug() << "checking socket for removal..." ;
        if ( socketIsRemovable(s, false) ){
            qDebug() << "removing queued socket" ;
            emit canRemoveSocket(s);
        }
    }
}
