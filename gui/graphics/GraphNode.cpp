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

#include "graphics/GraphNode.hpp"
#include "graphics/SocketWidget.hpp"
#include "managers/ConnectionManager.hpp"
#include "app/Theme.hpp"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QPoint>
#include <spdlog/spdlog.h>

GraphNode::GraphNode(QString name, QGraphicsItem* parent): 
    QGraphicsObject(parent),
    name_(name)
{
    // configure widget
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setAcceptHoverEvents(true);

    titleText_ = new QGraphicsTextItem(name_, this);
    titleText_->setAcceptedMouseButtons(Qt::NoButton);

    titleText_->setDefaultTextColor(Theme::Theme::COMPONENT_TEXT);
    titleText_->setPos(Theme::COMPONENT_TEXT_PADDING,Theme::COMPONENT_TEXT_PADDING);
    titleText_->setTextWidth(Theme::COMPONENT_WIDTH - Theme::COMPONENT_TEXT_PADDING * 2);

}

GraphNode::~GraphNode(){
    removeSockets();
}

QRectF GraphNode::boundingRect() const {
    qreal delta = Theme::COMPONENT_HIGHLIGHT_BUFFER + Theme::COMPONENT_HIGHLIGHT_WIDTH ;
    return QRectF(0, 0, Theme::COMPONENT_WIDTH, height_)
        .adjusted(-delta, -delta, delta, delta);
}

void GraphNode::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget){
    Q_UNUSED(option)
    Q_UNUSED(widget)

    // draw background
    QRectF baseRect(0, 0,Theme::COMPONENT_WIDTH, height_);
    painter->setBrush(Theme::Theme::COMPONENT_BACKGROUND);
    painter->setPen(QPen(Theme::Theme::COMPONENT_BORDER,Theme::COMPONENT_BORDER_WIDTH));
    painter->drawRoundedRect(baseRect, Theme::COMPONENT_ROUNDED_RADIUS, Theme::COMPONENT_ROUNDED_RADIUS);

    // when selected, draw an indicator around the object
    if (isSelected()){
        painter->setPen(QPen(Theme::Theme::COMPONENT_BORDER_SELECTED, Theme::COMPONENT_HIGHLIGHT_WIDTH, Qt::SolidLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(
            baseRect.adjusted(-Theme::COMPONENT_HIGHLIGHT_BUFFER,-Theme::COMPONENT_HIGHLIGHT_BUFFER,Theme::COMPONENT_HIGHLIGHT_BUFFER,Theme::COMPONENT_HIGHLIGHT_BUFFER),
            Theme::COMPONENT_ROUNDED_RADIUS,Theme::COMPONENT_ROUNDED_RADIUS
        );
    }
}

const std::vector<SocketWidget*>& GraphNode::getSockets() const {
    return sockets_ ;
}

SocketWidget* GraphNode::getSingleSocketMatchingEndpoint(const ConnectionEndpoint& endpoint) const {
    for ( auto* socket : sockets_ ){
        const SocketSpec& spec = socket->getSpec();
        if ( spec.isGroup() ) continue ;
        if ( spec.endpoints()[0] == endpoint ) return socket ;
    }
    return nullptr ;
}

std::vector<SocketWidget*> GraphNode::getSocketsMatchingSpec(const SocketSpec& spec) const {
    std::vector<SocketWidget*> v ;
    for ( auto* socket : sockets_ ){
        if ( spec == socket->getSpec() ) v.push_back(socket);
    }
    return v ;
}

std::vector<SocketWidget*> GraphNode::getSocketsMatchingEndpoint(const ConnectionEndpoint& endpoint) const {
    std::vector<SocketWidget*> v ;
    for ( auto* socket : sockets_ ){
        const SocketSpec& spec = socket->getSpec();
        for ( const auto& e : spec.endpoints() ){
            if ( e == endpoint ){
                v.push_back(socket);
                continue ;
            }
        }
    }
    return v ;
}

std::vector<SocketWidget*> GraphNode::getVisibleSocketsMatchingEndpoint(const ConnectionEndpoint& endpoint) const {
    std::vector<SocketWidget*> v ;
    for ( auto socket : sockets_ ){
        if ( !socket->isVisible() ) continue ;
        const SocketSpec& spec = socket->getSpec();
        for ( const auto& e : spec.endpoints() ){
            if ( e == endpoint ){
                v.push_back(socket);
                continue ;
            }
        }
    }
    return v ;
}

SocketWidget* GraphNode::insertSocket(SocketSpec spec){
    SocketWidget* socket = new SocketWidget(spec, this);
    sockets_.push_back(socket);

    layoutSockets();
    reorderSockets();
    positionSockets(scenePos());

    return socket ;
}

void GraphNode::insertSockets(std::vector<SocketSpec> specs){
    for ( const auto& s : specs ){
        SocketWidget* socket = new SocketWidget(s, this);
        sockets_.push_back(socket);
    }

    layoutSockets();
    reorderSockets();
    positionSockets(scenePos());
}

void GraphNode::hide(){
    QGraphicsItem::hide();
    for ( auto& s : getSockets() ){
        s->hide();
    }
}

void GraphNode::show(){
    QGraphicsItem::show();
    for ( auto& s : getSockets() ){
        s->show();
    }
}

void GraphNode::addToScene(QGraphicsScene* scene){
    if ( !scene->items().contains(this) ){
        scene->addItem(this);
    }
    
    for ( auto* socket : sockets_ ){
        if ( !scene->items().contains(socket) ){
            scene->addItem(socket);
        }
    }
}

std::vector<SocketWidget*> GraphNode::getHiddenSockets() const {
    std::vector<SocketWidget*> output ;

    for ( const auto& s : sockets_ ){
        if ( !s->isVisible() ){
            const SocketSpec& spec = s->getSpec();
            if ( spec.isGroup() ){
                output.push_back(s);
                continue ;
            } 

            // only include if it isn't otherwise a member of a group
            bool isGrouped = false ;
            for ( auto* search : sockets_ ){
                const SocketSpec& searchSpec = search->getSpec();
                if ( !searchSpec.isGroup() ) continue ;
                if ( searchSpec.includes(spec.endpoints()[0]) ){
                    isGrouped = true ;
                    break ;
                }
            }
            if ( !isGrouped ) output.push_back(s);
        } 
    }
    return output ;
}

void GraphNode::unhideSocket(SocketWidget* socket){
    if ( !socket ) return ;
    auto it = std::find(sockets_.begin(), sockets_.end(), socket);
    if ( it == sockets_.end() ) return ;
    socket->show();
    reorderSockets();
    positionSockets(scenePos());
    emit socketUnhidden(socket);
    emit positionChanged();
}

void GraphNode::unhideAllSockets(){
    for ( auto s : sockets_ ){
        if ( !s->isVisible() ) unhideSocket(s);
    }
}

void GraphNode::hideSocket(SocketWidget* socket){
    if ( !socket ) return ;
    auto it = std::find(sockets_.begin(), sockets_.end(), socket);
    if ( it == sockets_.end() ) return ;
    if ( ! socket->isVisible() ) return ;

    socket->hide();
    reorderSockets();
    positionSockets(scenePos());
    emit socketHidden(socket);
    emit positionChanged();
}

void GraphNode::hideDisconnectedSockets(){
    for ( auto s : sockets_ ){
        bool noConnection = ConnectionManager::instance()
            ->getNumConnectionsMatchingSpec(s->getSpec()) == 0 ;
        
        if ( noConnection && s->isVisible() ){
            hideSocket(s);
        }
    }
}

void GraphNode::layoutSockets(){
    leftSockets_.clear();
    rightSockets_.clear();
    topSockets_.clear();
    bottomSockets_.clear();

    for (SocketWidget* socket : sockets_){
        switch( socket->getSpec().type() ){
        case SocketType::MidiInbound:
        case SocketType::SignalInbound:
        case SocketType::BufferInbound:
            leftSockets_.push_back(socket);
            break ;
        case SocketType::MidiOutbound:
        case SocketType::SignalOutbound:
        case SocketType::BufferOutbound:
            rightSockets_.push_back(socket);
            break ;
        case SocketType::ModulationInbound:
            bottomSockets_.push_back(socket);
            break ;
        case SocketType::ModulationOutbound:
            topSockets_.push_back(socket);
            break ;
        default:
            break ;
        }
    }
}

void GraphNode::positionSockets(QPointF newPos){
    QPointF scenePos = newPos;

    // left
    qreal height = Theme::COMPONENT_HEIGHT ;
    for ( size_t i = 0; i < leftSockets_.size(); ++i ){
        if ( leftSockets_[i]->isVisible() ){
            qreal ypos = Theme::SOCKET_WIDGET_MARGIN + 
                i * Theme::SOCKET_WIDGET_SPACING ;

            leftSockets_[i]->setPos(scenePos + QPointF(
                -Theme::SOCKET_WIDGET_RADIUS, 
                ypos 
            ));
            height = std::max(height, ypos + Theme::SOCKET_WIDGET_MARGIN);
        }
    }

    // right
    for ( size_t i = 0; i < rightSockets_.size(); ++i ){
        if ( rightSockets_[i]->isVisible() ){
            qreal ypos = Theme::SOCKET_WIDGET_MARGIN + 
                i * Theme::SOCKET_WIDGET_SPACING ;
            rightSockets_[i]->setPos(scenePos + QPointF(
                Theme::COMPONENT_WIDTH + Theme::SOCKET_WIDGET_RADIUS, 
                ypos
            ));
            height = std::max(height, ypos + Theme::SOCKET_WIDGET_MARGIN);
        }
            
    }

    // bottom
    int socketsPerHorizontalRow = 
        ( Theme::COMPONENT_WIDTH - Theme::SOCKET_WIDGET_MARGIN * 2 ) / 
        Theme::SOCKET_WIDGET_SPACING ;
    int count = 0 ;
    for ( size_t i = 0; i < bottomSockets_.size(); ++i ){
        if ( bottomSockets_[i]->isVisible() ){
            int col = count % socketsPerHorizontalRow ;
            int row = count / socketsPerHorizontalRow ;

            qreal xpos = Theme::SOCKET_WIDGET_MARGIN + col * Theme::SOCKET_WIDGET_SPACING ;
            qreal ypos = height + Theme::SOCKET_WIDGET_RADIUS
                + row * Theme::SOCKET_WIDGET_SPACING ; 

            bottomSockets_[i]->setPos(scenePos + QPointF(xpos,ypos));
            ++count ;
        }
    }

    // top
    count = 0 ;
    for ( size_t i = 0; i < topSockets_.size(); ++i ){
        if ( topSockets_[i]->isVisible() ){
            int col = count % socketsPerHorizontalRow ;
            int row = count / socketsPerHorizontalRow ;

            qreal xpos = Theme::SOCKET_WIDGET_MARGIN + Theme::COMPONENT_WIDTH - 
                Theme::SOCKET_WIDGET_RADIUS - col * Theme::SOCKET_WIDGET_SPACING ;
            qreal ypos = -Theme::SOCKET_WIDGET_RADIUS - 
                row * Theme::SOCKET_WIDGET_SPACING ; 

            topSockets_[i]->setPos(scenePos + QPointF(xpos,ypos));
            ++count ;
        }
    }

    if ( height_ == height ) return ;

    if ( height_ > height ){
        prepareGeometryChange();    
    }

    height_ = height ;
    emit positionChanged();
}

void GraphNode::removeSockets(){
    for ( auto* socket : sockets_ ){
        scene()->removeItem(socket);
        socket->deleteLater();
    }
    sockets_.clear();
    leftSockets_.clear();
    rightSockets_.clear();
    topSockets_.clear();
    bottomSockets_.clear();
    
    update();
}

QVariant GraphNode::itemChange(GraphicsItemChange change, const QVariant& value ){
    if ( change == ItemPositionChange ){
        positionSockets(value.toPointF());
        emit positionChanged() ;
    }

    // if the item is selected, we need to inform graph panel to move it to the top
    if ( change == ItemSelectedChange ){
        if ( value.toBool()) emit needsZUpdate();
    }
    return QGraphicsObject::itemChange(change, value);
}

void GraphNode::reorderSockets(){
    /* order priority:
    1. Has Connection
    2. Socket Type
    2. Socket Name
    2. Hidden Socket
    */
    auto socketSortLR = [](const SocketWidget* a, const SocketWidget* b){
        bool aConnect = ConnectionManager::instance()
            ->getNumConnectionsMatchingSpec(a->getSpec()) > 0 ;
        bool bConnect = ConnectionManager::instance()
            ->getNumConnectionsMatchingSpec(b->getSpec()) > 0 ;

        if ( aConnect != bConnect ){
            return aConnect ;
        }

        bool aVisible = a->isVisible();
        bool bVisible = b->isVisible();

        if ( aVisible != bVisible ){
            return aVisible ;
        }

        const auto& aSpec = a->getSpec();
        const auto& bSpec = b->getSpec();

        if ( aSpec.type() != bSpec.type() ){
            return aSpec.type() < bSpec.type() ;
        }

        return aSpec.name() < bSpec.name() ;
    };

    // top/bottom are reversed due to draw order
    auto socketSortTB = [&socketSortLR](const SocketWidget* a, const SocketWidget* b){
        return socketSortLR(b,a);
    };

    std::ranges::sort(topSockets_, socketSortTB);
    std::ranges::sort(bottomSockets_, socketSortTB);
    std::ranges::sort(leftSockets_, socketSortLR);
    std::ranges::sort(rightSockets_, socketSortLR);
}

json GraphNode::serialize() const {
    json msg ;
    msg["node_type"] = "GraphNode" ;
    msg["name"] = name_.toStdString() ;
    msg["xpos"] = pos().x() ;
    msg["ypos"] = pos().y() ;
    msg["visible"] = isVisible();
    
    json sockets ;
    for ( const auto& socket : sockets_ ){
        sockets.push_back(socket->serialize());
    }
       
    return msg ;
}

void GraphNode::deserialize(const json& node){
    if ( node.contains("name") && node.at("name").is_string() ){
        onRename(QString::fromStdString(node.at("name")));
    } 

    if ( 
        node.contains("xpos") && node.at("xpos").is_number() &&
        node.contains("ypos") && node.at("ypos").is_number()
    ){
        setPos(node.at("xpos"), node.at("ypos"));
    }
    if ( node.contains("visible") && node.at("visible").is_boolean() ){
        setVisible(node.at("visible"));
    }

    if ( node.contains("sockets") && node.at("sockets").is_array() ){
        for ( const auto& s : node.at("sockets") ){
            std::optional<SocketSpec> spec = std::nullopt ;
            try {
                spec = s.at("spec") ;
            } catch ( const std::exception& e){
                SPDLOG_WARN("sockets did not contain a valid socket spec.");
                continue ;
            }

            const auto& sockets = getSocketsMatchingSpec(*spec);
            if ( sockets.size() == 0 ){
                if ( spec->isGroup() ){
                    SPDLOG_DEBUG("Creating new grouped socket.");
                    SocketWidget* sock = insertSocket(spec.value());
                    sock->deserialize(s);
                    continue ;
                }

                SPDLOG_WARN(
                    "A single SocketSpec is not already present in the component. This shouldn't happen."
                );
                continue ;
            } else {
                SPDLOG_DEBUG("deserializing {} sockets.", sockets.size());
                for ( auto* socket : sockets ){
                    socket->deserialize(s);
                }
            }
        }
    }
    update();
}

void GraphNode::onRename(QString name){
    name_ = name ;
    titleText_->setPlainText(name);
}

void GraphNode::removeSocket(SocketWidget* socket){
    if ( !socket || socket->getParent() != this ) return ;
    
    sockets_.erase(std::remove(
        sockets_.begin(), sockets_.end(), socket), 
        sockets_.end()
    );

    scene()->removeItem(socket);
    delete socket ;

    layoutSockets();
    reorderSockets();
    positionSockets(scenePos());
}