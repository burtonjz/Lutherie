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
#include "managers/ComponentManager.hpp"
#include "managers/SocketRegistry.hpp"
#include "widgets/ToastNotification.hpp"

#include <QMenu>

#include <spdlog/spdlog.h>

ConnectionRenderer::ConnectionRenderer(
    QGraphicsScene* scene,
    QObject* parent
):
    QObject(parent),
    scene_(scene),
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

    connect(
        SocketRegistry::instance(), &SocketRegistry::socketMappingChanged,
        this, &ConnectionRenderer::onSocketMappingChanged
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

    SocketWidget* toSocket = SocketRegistry::instance()->findSocketAt(scenePos);
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
        auto v = requestModulationParameter(dragCable_->getInboundSocket());

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
    for ( auto* cable : cables_ ) {
        if ( cable->involvesWidget(node) ) c.push_back(cable);
    }
    return c ;
}

const std::vector<ConnectionCable*> ConnectionRenderer::getSocketConnections(SocketWidget* socket) const {
    std::vector<ConnectionCable*> c ;
    for ( auto* cable : cables_ ){
        if ( cable->involvesSocket(socket)) c.push_back(cable);
    }
    return c ;
}


void ConnectionRenderer::deleteCable(ConnectionCable* cable){
    if ( !cable ) return ;

    SPDLOG_TRACE(
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

void ConnectionRenderer::onSocketPositionChanged(SocketWidget* socket){
    for ( const auto& cable : cables_ ) {
        if ( !cable->involvesSocket(socket) ) continue ;
        cable->updatePath();
    }
}

void ConnectionRenderer::onSocketVisibilityChanged(SocketWidget* socket){
    if ( !socket->hasClaims() ) return ;

    for ( const auto& cable : cables_ ){
        if ( !cable->involvesSocket(socket) ) continue ;
        setCableVisibility(cable);
    }
}

void ConnectionRenderer::onSocketMappingChanged(){
    SocketRegistry* registry = SocketRegistry::instance();
    for ( auto& [req, cableRef] : req2Cable_ ){
        if ( !cableRef ){
            SPDLOG_WARN("found null cable during mapping for request: {}", req.toString());
            findOrCreateCable(req);
            continue ;
        } 

        ConnectionCable* cable = cableRef ; // copy out from map

        SocketWidget* outbound = registry->findSocket(req.outbound());
        SocketWidget* inbound = registry->findSocket(req.inbound());        
        
        bool outboundMatch = cable->getOutboundSocket() == outbound ;
        bool inboundMatch = cable->getInboundSocket() == inbound ;
        if ( inboundMatch && outboundMatch ) continue ;

        // cable doesn't match
        req2Cable_.at(req) = nullptr ;
        if ( !cableHasConnections(cable) ) deleteCable(cable);
        findOrCreateCable(req);
    }
}

ConnectionCable* ConnectionRenderer::registerCable(ConnectionCable* candidate){
    // if candidate matches existing, delete and replace with existing
    ConnectionCable* cable = candidate ;
    for ( auto* c : cables_ ){
        if ( candidate == c ){
            delete candidate ;
            return c ;
        }
    }

    scene_->addItem(cable);
    cable->setZValue(std::max(
        cable->getInboundSocket()->zValue(), 
        cable->getOutboundSocket()->zValue())
    );
    
    SPDLOG_TRACE(
        "registered new ConnectionCable ({}): (SocketWidget*={}, name={}) -> (SocketWidget*={}, name={})", 
        fmt::ptr(cable),
        fmt::ptr(cable->getOutboundSocket()), 
        cable->getOutboundSocket()->getSpec().name().toStdString(),
        fmt::ptr(cable->getInboundSocket()), 
        cable->getInboundSocket()->getSpec().name().toStdString()
    );

    setCableVisibility(cable);
    cables_.push_back(cable);
    return cable ;
}

void ConnectionRenderer::findOrCreateCable(const ConnectionRequest& req){
    SocketWidget* outbound = SocketRegistry::instance()->findSocket(req.outbound());
    SocketWidget*  inbound = SocketRegistry::instance()->findSocket(req.inbound());

    if ( !outbound || !inbound ){
        SPDLOG_WARN("did not find sockets to draw connection cable. Please investigate");
        req2Cable_[req] = nullptr ;
        return ;
    }
    
    ConnectionCable* candidate = new ConnectionCable(outbound, inbound);
    if ( inbound->getSpec().type() == SocketType::ModulationInbound ){
        candidate->setModulatedParameter(
            req.inbound().modulatedParam().value(), 
            req.modulatingDepth()
        );
    }

    ConnectionCable* cable = registerCable(candidate);
    req2Cable_[req] = cable ;
}

bool ConnectionRenderer::cableHasConnections(ConnectionCable* cable) const {
    for ( const auto& [req, c] : req2Cable_ ){
        if ( cable == c ){
            return true ;
        }
    }
    return false ;
}

void ConnectionRenderer::setCableVisibility(ConnectionCable* cable){
    cable->setVisible(
        cable->getOutboundSocket()->isVisible() &&
        cable->getInboundSocket()->isVisible() 
    );
}

void ConnectionRenderer::onConnectionAdded(const ConnectionRequest& req){
    if ( req2Cable_.contains(req) ){
        SPDLOG_WARN(
            "Received connection add request already present in renderer",
            "this suggests a desync has occurred. Please investigate. ConnectionRequest={}",
            req.toString()
        );
        return ;
    }

    findOrCreateCable(req);
}

void ConnectionRenderer::onConnectionRemoved(const ConnectionRequest& req){
    if ( !req2Cable_.contains(req) ){
        SPDLOG_WARN(
            "The removed connection is not present in request map."
            "This suggests this client is out of sync. ConnectionRequest={}",
            req.toString()
        );
        return ;
    }

    ConnectionCable* cable = req2Cable_.at(req);
    req2Cable_.erase(req);

    if ( !cableHasConnections(cable) ){
        deleteCable(cable);
    }
}

ConnectionRenderer::ModulationParameter ConnectionRenderer::requestModulationParameter(SocketWidget* socket){
    ModulationParameter output ; 

    if ( !socket ) return output ;
    
    const SocketSpec& spec = socket->getSpec();

    // create user menu
    QMenu menu ;
    QAction* header = menu.addAction("Select Parameter");
    header->setEnabled(false);
    menu.addSeparator();

    bool multipleIds = spec.componentIds().size() > 1 ;
    auto createActionName = [&](int id, ParameterType p, bool depth = false){
        QString name = "";
        if ( multipleIds ){
            name = ComponentManager::instance()
                ->getModel(id)->getName()
                + ": ";
        }
        name = name + QString::fromStdString(
            std::string(GET_PARAMETER_TRAIT_MEMBER(p, name))
        );
        if ( depth ){
            name = name + " depth" ;
        }
        return name ;
    };
    
    bool hasActions = false ;
    for ( const auto& endpoint : spec.endpoints() ){
        bool exists = ConnectionManager::instance()
            ->hasModulationConnections(endpoint);
        if ( !exists ){
            int id = endpoint.componentId().value();
            ParameterType p = endpoint.modulatedParam().value();
            QAction* param = menu.addAction(createActionName(id, p));
            connect(
                param, &QAction::triggered,
                [&output, endpoint](){
                    output = {
                        .endpoint = endpoint
                    };
                }
            );
            hasActions = true ;
            continue ;
        }

        bool depthExists = ConnectionManager::instance()
            ->hasModulationDepthConnections(endpoint);
        if ( !depthExists ){
            int id = endpoint.componentId().value();
            ParameterType p = endpoint.modulatedParam().value();
            QAction* param = menu.addAction(createActionName(id, p, true));
            connect(
                param, &QAction::triggered,
                [&output, endpoint](){
                    output = {
                        .endpoint = endpoint,
                        .depth = true 
                    };
                }
            );
            hasActions = true ;
        }
    }

    if ( ! hasActions ){ 
        ToastNotification::show("All modulation slots are full.");
        return output ;
    }

    menu.exec(QCursor::pos());
    return output ;
}