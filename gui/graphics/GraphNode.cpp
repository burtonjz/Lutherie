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
#include "app/Theme.hpp"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QPoint>

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
    for ( auto socket : sockets_ ){
        socket->deleteLater();
    }
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

SocketWidget* GraphNode::getSocket(SocketSpec s) const {
    for ( auto socket : sockets_ ){
        if ( socket->matches(s) ) return socket ;
    }
    return nullptr ;
}

void GraphNode::requestRename(const QString& name){
    // default case no request necessary, just rename
    onRename(name);
}

void GraphNode::createSockets(std::initializer_list<SocketSpec> specs ){
    for ( const auto& s : specs ){
        SocketWidget* socket = new SocketWidget(s, this);
        sockets_.push_back(socket);
    }

    layoutSockets();
    reorderSockets();
    positionSockets(scenePos());
}

void GraphNode::createSockets(std::vector<SocketSpec> specs){
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
    scene->addItem(this);
    for ( auto* socket : sockets_ ) scene->addItem(socket);
}

std::vector<SocketWidget*> GraphNode::getHiddenSockets() const {
    std::vector<SocketWidget*> output ;
    for ( const auto& s : sockets_ ){
        if ( ! s->isVisible() ) output.push_back(s);
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
        if ( ! s->hasConnection() && s->isVisible() ){
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
        switch(socket->getSpec().type){
        case SocketType::MidiInbound:
        case SocketType::SignalInbound:
            leftSockets_.push_back(socket);
            break ;
        case SocketType::MidiOutbound:
        case SocketType::SignalOutbound:
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
    for ( int i = 0; i < leftSockets_.size(); ++i ){
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
    for ( int i = 0; i < rightSockets_.size(); ++i ){
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
    for ( int i = 0; i < bottomSockets_.size(); ++i ){
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
    for ( int i = 0; i < topSockets_.size(); ++i ){
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
        bool aConnect = a->hasConnection();
        bool bConnect = b->hasConnection();

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

        if ( aSpec.type != bSpec.type ){
            return aSpec.type < bSpec.type ;
        }

        return aSpec.name < bSpec.name ;
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
    
    // don't include hidden sockets if the whole node is invisible (as it is when it is grouped)
    if ( ! isVisible() ) return msg ;

    auto& hidden = msg["hidden_sockets"] ;
    for ( const auto& s : sockets_ ){
        if ( ! s->isVisible() ){
            hidden.push_back(s->getSpec());
        }
    }

    return msg ;
}

void GraphNode::deserialize(const json& node){
    if ( node.contains("name") && node.at("name").is_string() ){
        requestRename(QString::fromStdString(node.at("name")));
    } 
    if ( 
        node.contains("xpos") && node.at("xpos").is_number() &&
        node.contains("ypos") && node.at("ypos").is_number()
    ){
        setPos(node.at("xpos"), node.at("ypos"));
    }
    if ( node.contains("hidden_sockets") && node.at("hidden_sockets").is_array() ){
        for ( const auto& s : node.at("hidden_sockets") ){
            try {
                SocketSpec spec = s ;
                SocketWidget* s = getSocket(spec);
                hideSocket(s);
            } catch ( const std::exception& e){
                qWarning() << "hidden sockets did not contain a valid socket spec." ;
            }
        }
    }
    update();
}

void GraphNode::onRename(QString name){
    name_ = name ;
    titleText_->setPlainText(name);
}
