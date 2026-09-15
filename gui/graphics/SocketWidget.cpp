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

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItem>
#include <spdlog/spdlog.h>

SocketWidget::SocketWidget(SocketSpec spec, SocketClaimBehavior claimBehavior, GraphNode* parent):
    QGraphicsObject(),
    spec_(spec),
    parent_(parent)
{
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setZValue(-0.2); // we want the sockets just behind the GraphNode, with room to place the cable between
    setToolTip(spec_.name());

    connect(
        this, &SocketWidget::positionChanged,
        parent, &GraphNode::socketPositionChanged
    );

    // socket registration
    SocketRegistry::instance()->registerSocket(this);

    switch(claimBehavior){
    case SocketClaimBehavior::Override:
        SocketRegistry::instance()->claim(this);
        break ;
    case SocketClaimBehavior::Reactive:
        connect(
            SocketRegistry::instance(), &SocketRegistry::socketMappingChanged,
            this, &SocketWidget::onMappingChanged
        );
        onMappingChanged();
        break ;
    default:
        SPDLOG_WARN("invalid socket claim behavior specified");
        break ;
    }
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

void SocketWidget::onMappingChanged(){
    auto* registry = SocketRegistry::instance();

    // if any of my endpoints are available, claim my endpoints
    for ( const auto& endpoint : spec_.endpoints() ){
        if ( registry->isClaimed(endpoint) ) continue ;
        registry->claim(this);
        return ;
    }
}
