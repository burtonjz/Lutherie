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
#include "managers/ConnectionManager.hpp"

#include <spdlog/spdlog.h>

ConnectionRenderer::ConnectionRenderer(
    QGraphicsScene* scene,
    ISocketLookup* socketLookup,
    QObject* parent
):
    QObject(parent),
    scene_(scene),
    socketLookup_(socketLookup),
    dragCable_(nullptr),
    dragFromSocket_(nullptr)
{
    connect(
        ConnectionManager::instance(), 
        &ConnectionManager::connectionAdded, 
        this, 
        &ConnectionRenderer::onConnectionAdded
    );

    connect(
        ConnectionManager::instance(), 
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
    if ( !dragCable_ || !dragFromSocket_ ){
        SPDLOG_DEBUG("drag connection not available. Unable to finish drag.");
        return ;
    }

    dragFromSocket_->ungrabMouse();

    SocketWidget* toSocket = socketLookup_->findSocketAt(scenePos);
    if ( !toSocket ){
        SPDLOG_DEBUG("No socket endpoint specified for drag cable. Cancelling connection.");
        cancelDrag();
        return ;
    }

    if ( dragCable_->isCompatible(toSocket) ){
        dragCable_->setToSocket(toSocket);
    } else {
        cancelDrag();
        return ;
    }

    const SocketSpec& outbound = dragCable_->getOutboundSocket()->getSpec();
    const SocketSpec& inbound = dragCable_->getInboundSocket()->getSpec();

    if ( inbound.type() == SocketType::ModulationInbound ){
        auto v = socketLookup_
            ->requestModulationParameter(dragCable_->getInboundSocket());

        if ( !v.endpoint.has_value() ){
            cancelDrag();
            return ;
        } 

        for ( const auto& endpoint : outbound.endpoints() ){
            ConnectionManager::instance()->requestConnectionEvent(
                endpoint, v.endpoint.value(),
                false, v.depth
            );
        }
        cancelDrag();
        return ;
    }
     
    ConnectionManager::instance()->requestConnectionEvent(
        outbound, inbound
    );
    cancelDrag();
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

void ConnectionRenderer::requestRemoveConnections(ConnectionCable* cable){
    if ( !cable ) return ;

    SocketWidget* fromSock = cable->getFromSocket();
    SocketWidget* toSock = cable->getToSocket();

    if ( !fromSock || !toSock ) return ;

    ConnectionManager::instance()->requestConnectionEvent(
        fromSock->getSpec(), toSock->getSpec(), 
        true, cable->modulatesDepth()
    );
}

void ConnectionRenderer::requestRemoveSocket(SocketWidget* s){
    if ( socketIsRemovable(s, true) ){
        emit canRemoveSocket(s);
    } else {
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

bool ConnectionRenderer::socketIsRemovable(SocketWidget* s, bool request){
    bool hasConnection = false ;
    for ( auto c: cables_ ){
        if ( c->getFromSocket() == s || c->getToSocket() == s ){
            hasConnection = true ;
            if ( request ){
                requestRemoveConnections(c);
            }
        }
    }
    
    return !hasConnection ;
}

void ConnectionRenderer::onComponentGroup(const std::vector<int>& componentIds){
    for ( auto cable : cables_ ){
        const SocketSpec& fromSpec = cable->getFromSocket()->getSpec();
        const SocketSpec& toSpec = cable->getToSocket()->getSpec();

        auto fromIds = fromSpec.componentIds();
        auto toIds = toSpec.componentIds();
        if ( 
            fromIds.size() == 0 && 
            toIds.size() == 0 
        ) continue ;

        for ( const auto& id : componentIds ){
            if ( fromIds.contains(id) ){
                cable->setVisible(true);
                cable->setFromSocket(socketLookup_->findVisibleSocket(fromSpec));
            }
            if ( toIds.contains(id) ){
                cable->setVisible(true);
                cable->setToSocket(socketLookup_->findVisibleSocket(toSpec));
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
    SocketWidget* outbound = socketLookup_->findVisibleSocket(req.inbound());
    SocketWidget*  inbound = socketLookup_->findVisibleSocket(req.outbound());

    if ( !outbound || !inbound ){
        SPDLOG_DEBUG("did not find sockets to draw connection cable. Please investigate");
        return ;
    }

    ConnectionCable* c = new ConnectionCable(outbound, inbound);
    
    if ( inbound->getSpec().type() == SocketType::ModulationInbound ){
        c->setModulatedParameter(req.inbound().modulatedParam().value(), req.modulatingDepth());
    }

    cables_.push_back(c);
    scene_->addItem(c);
    c->setZValue(std::max(inbound->zValue(), outbound->zValue()));
}

void ConnectionRenderer::onConnectionRemoved(const ConnectionRequest& req){
    const auto& outbound = req.outbound();
    const auto& inbound = req.inbound();

    // find cable with these endpoints
    ConnectionCable* match = nullptr ;
    for ( auto c : cables_ ){
        if ( 
            c->involvesEndpoint(outbound) &&
            c->involvesEndpoint(inbound)
        ){
            match = c ;
            break ;
        }
    }
    
    if ( !match ) return ;

    size_t nConnections = ConnectionManager::instance()
        ->getNumConnectionsMatchingEndpoints(outbound, inbound);

    if ( nConnections > 0 ) return ;

    
    cables_.erase(std::remove(
        cables_.begin(), cables_.end(), match), 
        cables_.end()
    );
    scene_->removeItem(match);
    delete match ;

    for ( auto s : socketsQueuedForRemoval_ ){
        if ( socketIsRemovable(s, false) ){
            emit canRemoveSocket(s);
        }
    }
}
