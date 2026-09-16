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

#include "graphics/SocketWidget.hpp"
#include "graphics/GraphNode.hpp"
#include "app/Theme.hpp"
#include "managers/SocketRegistry.hpp"
#include "managers/ComponentManager.hpp"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItem>
#include <spdlog/spdlog.h>

SocketWidget::SocketWidget(SocketSpec spec, SocketPriority p, GraphNode* parent):
    QGraphicsObject(),
    spec_(spec),
    parent_(parent),
    priority_(p)
{
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setZValue(-0.2); // we want the sockets just behind the GraphNode, with room to place the cable between
    setToolTip(spec_.name());

    // socket registration
    SocketRegistry::instance()->registerSocket(this, priority());

    // other connections
    connect(
        this, &SocketWidget::positionChanged,
        parent, &GraphNode::socketPositionChanged
    );
    connect(
        ComponentManager::instance(), &ComponentManager::componentRenamed,
        this, [this](int componentId){
            if ( getSpec().componentIds().contains(componentId) ){
                spec_.updateName();
            }
        }
    );
}

SocketWidget::~SocketWidget(){
    SocketRegistry::instance()->unregisterSocket(this);
}

QRectF SocketWidget::boundingRect() const {
    auto radius = Theme::SOCKET_WIDGET_RADIUS ;
    return QRectF(-radius, -radius, radius * 2, radius * 2); 
}

void SocketWidget::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget){
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->setRenderHint(QPainter::Antialiasing);
    
    // Draw circle
    QColor socketColor = getSocketColor(isHovered_);
    
    painter->setBrush(socketColor);
    painter->setPen(QPen(Qt::black, 2));
    painter->drawEllipse(boundingRect());

    // Draw an indicator for input vs output
    if (isOutbound()){
        painter->setBrush(Qt::white);
    } else {
        painter->setBrush(Qt::black);
    }
    painter->drawEllipse(-2,-2,4,4);

}

QVariant SocketWidget::itemChange(GraphicsItemChange change, const QVariant& value ){
    if ( change == ItemPositionHasChanged && hasClaims() ){
        emit positionChanged(this);
    }

    return QGraphicsObject::itemChange(change, value);
}

QColor SocketWidget::getSocketColor(bool isHovered) const {
    switch( spec_.type() ){
        case SocketType::ModulationInbound:
        case SocketType::ModulationOutbound:
            return isHovered ? Theme::SOCKET_MODULATION_LIGHT : Theme::SOCKET_MODULATION ;
        case SocketType::SignalInbound:
        case SocketType::SignalOutbound:
            return isHovered ? Theme::SOCKET_SIGNAL_LIGHT : Theme::SOCKET_SIGNAL ;
        case SocketType::BufferInbound:
        case SocketType::BufferOutbound:
            return isHovered ? Theme::SOCKET_BUFFER_LIGHT : Theme::SOCKET_BUFFER ;
        case SocketType::MidiInbound:
        case SocketType::MidiOutbound:
            return isHovered ? Theme::SOCKET_MIDI_LIGHT : Theme::SOCKET_MIDI ;
        default: 
            return Qt::gray ;
    }
}

bool SocketWidget::isHovered() const {
    return isHovered_ ;
}

void SocketWidget::setHovered(bool hovered){
    isHovered_ = hovered ;
    update();
}

bool SocketWidget::hasClaims() const {
    return hasClaims_ ;
}

void SocketWidget::setHasClaims(bool b){
    if ( hasClaims() == b ){
        return ;
    }
    hasClaims_ = b ;

    if ( !hasClaims() ){
        setVisible(false);
        emit visibilityChanged(this);
    } else if ( userHidden() == isVisible() ){
        setVisible(!userHidden());
        emit visibilityChanged(this);
    }
    emit claimStatusChanged();
}

bool SocketWidget::userHidden() const {
    return userHidden_ ;
}

void SocketWidget::setUserHidden(bool hidden){
    if ( userHidden() == hidden ) return ;
    userHidden_ = hidden ;

    if ( !hasClaims() ) return ;
    
    if ( userHidden() == isVisible() ){
        setVisible(!userHidden());
        emit visibilityChanged(this);
    }
}

bool SocketWidget::isOutbound() const {
    return !spec_.type().isInbound();
}

bool SocketWidget::isInbound() const {
    return spec_.type().isInbound();
}

SocketPriority SocketWidget::priority() const {
    return priority_ ;
}

QPointF SocketWidget::getConnectionPoint() const {
    return mapToScene(0,0);
}

json SocketWidget::serialize() const {
    json msg ;
    msg["spec"] = spec_ ;
    msg["hidden"] = userHidden_ ;

    return msg ;
}

void SocketWidget::deserialize(const json& msg){
    if ( !msg.contains("spec") ){
        throw std::runtime_error("serialization message does not include 'spec'.");
    }

    SocketSpec spec = msg.at("spec");
    if ( spec != spec_ ){
        SPDLOG_ERROR("deserialization spec does not match socket spec. Exiting.");
        return ;
    }

    if ( msg.contains("hidden") && msg.at("hidden").is_boolean() ){
        setUserHidden(msg.at("hidden").get<bool>());
    }
}
