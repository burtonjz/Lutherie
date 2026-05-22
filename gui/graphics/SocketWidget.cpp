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

#include <QGraphicsSceneMouseEvent>
#include <qgraphicsitem.h>

SocketWidget::SocketWidget(SocketSpec spec, GraphNode* parent):
    QGraphicsObject(),
    spec_(spec),
    parent_(parent)
{
    if ( spec.idx.has_value() ){
        setData(Qt::UserRole, QVariant::fromValue(spec.idx.value()));
    }
    
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setZValue(-0.2); // we want the sockets just behind the GraphNode, with room to place the cable between

    setToolTip(spec_.name);
    show();
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

QColor SocketWidget::getSocketColor(bool isHovered) const {
    switch(spec_.type){
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

bool SocketWidget::isOutbound() const {
    return spec_.type == SocketType::ModulationOutbound || 
           spec_.type == SocketType::SignalOutbound ||
           spec_.type == SocketType::BufferOutbound ||
           spec_.type == SocketType::MidiOutbound
    ;
}

bool SocketWidget::isInbound() const {
    return !isOutbound() ;
}

bool SocketWidget::hasConnection() const {
    return nConnections_ > 0 ;
}

void SocketWidget::setConnnection(bool newConnection){
    if ( newConnection ){
        ++nConnections_ ;
    } else {
        --nConnections_ ;
    }
}

void SocketWidget::syncConnection(SocketWidget* other){
    nConnections_ = other->nConnections_ ;
}

QPointF SocketWidget::getConnectionPoint() const {
    return mapToScene(0,0);
}

bool SocketWidget::matches(SocketSpec spec) const {
    return spec == spec_ ;
}
