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

ConnectionCable* ConnectionRenderer::createCable(
    SocketWidget* outbound, SocketWidget* inbound,
    std::optional<ParameterType> modParam, bool modDepth
){
    ConnectionCable* c = new ConnectionCable(outbound, inbound);
    if ( inbound->getSpec().type() == SocketType::ModulationInbound ){
        c->setModulatedParameter(modParam.value(), modDepth);
    }

    auto it = std::find(cables_.begin(), cables_.end(), c);

    if ( it != cables_.end() ){
        SPDLOG_WARN("will not create cable that is already rendered.");
        delete c ;
        return nullptr ;
    }

    cables_.push_back(c);
    scene_->addItem(c);
    c->setZValue(std::max(inbound->zValue(), outbound->zValue()));

    SPDLOG_DEBUG(
        "created new ConnectionCable ({}): (SocketWidget*={}, name={}) -> (SocketWidget*={}, name={})", 
        fmt::ptr(c),
        fmt::ptr(outbound), outbound->getSpec().name().toStdString(),
        fmt::ptr(inbound), inbound->getSpec().name().toStdString()
    );

    return c ;
}

void ConnectionRenderer::deleteCable(ConnectionCable* cable){
    if ( !cable ) return ;

    SPDLOG_DEBUG(
        "deleting ConnectionCable ({}): SocketWidget*={} -> SocketWidget*={}", 
        fmt::ptr(cable),
        fmt::ptr(cable->getFromSocket()),
        fmt::ptr(cable->getToSocket())
    );

    cables_.erase(std::remove(
        cables_.begin(), cables_.end(), cable), 
        cables_.end()
    );
    scene_->removeItem(cable);
    delete cable ;
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

void ConnectionRenderer::onSocketAdded(SocketWidget* socket){
    if ( !socket || !socket->isVisible() ){
        SPDLOG_WARN("socket add for null or not visible socket is not expected ({})", fmt::ptr(socket));
        return ;
    }
    const SocketSpec& spec = socket->getSpec();
    bool inbound = spec.type().isInbound();

    std::set<SocketWidget*> others ;
    std::set<ConnectionCable*> toDelete ;
    for ( auto* cable : cables_ ){
        for ( const auto& endpoint : spec.endpoints() ){
            if ( !cable->involvesEndpoint(endpoint) ) continue ;

            SocketWidget* other = nullptr ;
            if ( inbound ) other = cable->getOutboundSocket();
            else other = cable->getInboundSocket();
            
            if ( !other ){
                SPDLOG_WARN("a null socket was found on an active cable.");
                continue ;
            }

            // consolidate cables if already present
            if ( others.contains(other) ){
                toDelete.insert(cable);
                break ;
            }

            // otherwise edit this cable
            if ( inbound ) cable->setInboundSocket(socket);
            else           cable->setOutboundSocket(socket);

            SPDLOG_TRACE(
                "updated ConnectionCable*={} to use the following sockets: "
                "(SocketWidget*={}, name={}) -> (SocketWidget*={}, name={})", 
                fmt::ptr(cable),
                fmt::ptr(cable->getOutboundSocket()), 
                cable->getOutboundSocket()->getSpec().name().toStdString(),
                fmt::ptr(cable->getInboundSocket()),
                cable->getInboundSocket()->getSpec().name().toStdString()
            );

            cable->updatePath();
            others.insert(other);
            break ;
        }
    }

    while ( toDelete.size() > 0 ){
        auto it = toDelete.begin();
        SPDLOG_TRACE(
            "ConnectionCable* {} is now a duplicate., deleting...",
            fmt::ptr(*it)
        );
        deleteCable(*it);
        toDelete.erase(it);
    }
}

void ConnectionRenderer::onSocketRemoval(SocketWidget* socket){
    if ( !socket ){
        SPDLOG_WARN("socket removal passed a nullptr");
        return ;
    } 

    std::vector<ConnectionCable*> cables = cables_ ;
    for ( auto* cable : cables ){        
        SocketWidget* other ;
        if ( socket == cable->getInboundSocket() ){
            other = cable->getOutboundSocket();
        } else if ( socket == cable->getOutboundSocket() ){
            other = cable->getInboundSocket();
        } else {
            continue ;
        }

        if ( !other ){
            SPDLOG_WARN("a null socket was found on an active cable.");
            continue ;
        }

        // find the current socket(s)
        const SocketSpec& spec = socket->getSpec();
        std::set<SocketWidget*> resolvedSockets ;
        for ( const auto& endpoint : spec.endpoints() ){
            SocketWidget* resolved = socketLookup_->findVisibleSocket(endpoint);
            if ( !resolved ) continue ;
            resolvedSockets.insert(resolved);
        }

        SPDLOG_TRACE(
            "ConnectionCable*={} uses removed SocketWidget*={}. "
            "Socket Lookup found {} socket(s) for replacement.",
            fmt::ptr(cable), fmt::ptr(socket), resolvedSockets.size()
        );

        std::optional<ParameterType> p = cable->getModulatedParameter();
        bool depth = cable->modulatesDepth();
        deleteCable(cable);

        if ( resolvedSockets.size() == 0 ){
            SPDLOG_WARN(
                "no new visible Socket is available for the given connection: {}. "
                "The cable gets silently dropped as invalid in this condition.",
                cable->toText().toStdString()
            );
        }

        for ( auto* s : resolvedSockets ){
            createCable(other, s, p, depth);
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
    SocketWidget* outbound = socketLookup_->findVisibleSocket(req.outbound());
    SocketWidget*  inbound = socketLookup_->findVisibleSocket(req.inbound());

    if ( !outbound || !inbound ){
        SPDLOG_DEBUG("did not find sockets to draw connection cable. Please investigate");
        return ;
    }

    createCable(outbound, inbound, req.inbound().modulatedParam(), req.modulatingDepth());
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
    deleteCable(match);
}
