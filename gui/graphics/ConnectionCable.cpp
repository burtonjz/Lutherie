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

#include "graphics/ConnectionCable.hpp"
#include "graphics/SocketWidget.hpp"
#include "graphics/GraphNode.hpp"
#include "app/Theme.hpp"

#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <cmath>

ConnectionCable::ConnectionCable(SocketWidget* fromSocket, SocketWidget* toSocket): 
    QGraphicsPathItem(nullptr), 
    fromSocket_(fromSocket), 
    toSocket_(toSocket)
{
    setZValue(-0.1); // above socket, below container
    setPen(QPen(getCableColor(), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    if (!toSocket) endpoint_ = fromSocket_->getConnectionPoint() + QPoint(5,5); // set initial endpoint on creation, offset slightly so visible
    updatePath();
}

bool ConnectionCable::operator==(const ConnectionCable& other) const {
    return toConnectionRequest() == other.toConnectionRequest();
}
bool ConnectionCable::operator==(const ConnectionRequest& req) const {
    return toConnectionRequest() == req ;
}

SocketWidget* ConnectionCable::getInboundSocket() const {
    if ( fromSocket_ && fromSocket_->isInbound() ) return fromSocket_ ;
    if ( toSocket_ && toSocket_->isInbound() ) return toSocket_ ;
    return nullptr ;
}

SocketWidget* ConnectionCable::getOutboundSocket() const {
    if ( fromSocket_ && fromSocket_->isOutbound() ) return fromSocket_ ;
    if ( toSocket_ && toSocket_->isOutbound() ) return toSocket_ ;
    return nullptr ;
}

std::optional<ParameterType> ConnectionCable::getModulatedParameter(bool depth) const {
    if ( depth != isDepthModulation_ ){
        return std::nullopt ;
    }

    return modulated_ ;
}

void ConnectionCable::setModulatedParameter(ParameterType p, bool depth){
    modulated_ = p ;
    isDepthModulation_ = depth ;
}

void ConnectionCable::setFromSocket(SocketWidget* socket){
    fromSocket_ = socket ;
    updatePath();
}

void ConnectionCable::setToSocket(SocketWidget* socket){
    toSocket_ = socket ;
    updatePath();
}

void ConnectionCable::setEndpoint(const QPointF& point){
    endpoint_  = point ;
    updatePath();
}

bool ConnectionCable::isCompatible(SocketWidget* socket) const {
    if (!fromSocket_ || !socket) return false ;
    
    // Can't connect socket to itself
    if (fromSocket_ == socket) return false ;
    
    // Can't connect sockets from the same container widget
    // if (fromSocket_->getParent() == socket->getParent()) return false ;
    
    // Must be compatible types
    SocketType fromType = fromSocket_->getSpec().type;
    SocketType toType = socket->getSpec().type;

    switch ( fromType ){
    case SocketType::MidiInbound:
        return toType == SocketType::MidiOutbound ;
    case SocketType::MidiOutbound:
        return toType == SocketType::MidiInbound ;
    case SocketType::ModulationInbound:
        return toType == SocketType::ModulationOutbound ;
    case SocketType::ModulationOutbound:
        return toType == SocketType::ModulationInbound ;
    case SocketType::SignalInbound:
        return toType == SocketType::SignalOutbound ;
    case SocketType::SignalOutbound:
        return toType == SocketType::SignalInbound ;
    case SocketType::BufferInbound:
        return toType == SocketType::BufferOutbound ;
    case SocketType::BufferOutbound:
        return toType == SocketType::BufferInbound ;
    default:
        return false ;
    }
}

bool ConnectionCable::involvesWidget(GraphNode* widget) const {
    return ( fromSocket_ && fromSocket_->getParent() == widget ) ||
           ( toSocket_   &&   toSocket_->getParent() == widget ) ;
}

bool ConnectionCable::involvesSocket(SocketWidget* socket) const {
    return ( fromSocket_ && fromSocket_ == socket ) ||
           ( toSocket_ && toSocket_ == socket ) ;
}

void ConnectionCable::updatePath(){
    if (!fromSocket_) return ;
    
    QPointF startPoint = fromSocket_->getConnectionPoint();
    QPointF endPoint ;
    
    if ( toSocket_ ) {
        endPoint = toSocket_->getConnectionPoint();
    } else {
        endPoint = endpoint_ ;
    }
    
    QPainterPath newPath = createAdaptiveBezierPath(startPoint, endPoint);
    setPath(newPath);
    
    // Update pen color
    setPen(QPen(getCableColor(), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
}

ConnectionRequest ConnectionCable::toConnectionRequest() const {
    ConnectionRequest r ;
    
    auto outboundSocket = getOutboundSocket();
    auto inboundSocket = getInboundSocket();
    
    if ( inboundSocket ){  
        r.inboundID = inboundSocket->getSpec().componentId ;
        r.inboundSocket = inboundSocket->getSpec().type ;
        r.inboundIdx = inboundSocket->getSpec().idx ;
        if ( 
            inboundSocket->getSpec().type == SocketType::ModulationInbound &&
            modulated_.has_value()
        ){
            r.inboundParameter = modulated_.value() ;
        }
    }
        
    if ( outboundSocket ){
        r.outboundID = outboundSocket->getSpec().componentId ;
        r.outboundSocket = outboundSocket->getSpec().type ;
        r.outboundIdx = outboundSocket->getSpec().idx ;
    }

    if ( isDepthModulation_ ){
        r.depthConnection = true ;
    }
    
    return r ;
};

QString ConnectionCable::toText() const {
    QString fromText = getFromSocket()
        ? QString("%1 %2")
            .arg(getFromSocket()->getParent()->getName())
            .arg(getFromSocket()->getSpec().name)
        : "null" ;

    QString toText = getToSocket()
        ? QString("%1 %2")
            .arg(getToSocket()->getParent()->getName())
            .arg(getToSocket()->getSpec().name)
        : "null" ;
    return fromText + "->" + toText ;
}

QPainterPath ConnectionCable::createBezierPath(const QPointF& start, const QPointF& end){
    QPainterPath path ;
    path.moveTo(start) ;
    
    // Calculate control points for a nice curved cable
    qreal dx = end.x() - start.x();
    qreal dy = end.y() - start.y();
    
    // Make curves more pronounced for longer distances
    qreal controlOffset = std::max(50.0, std::abs(dx) * 0.5);
    
    QPointF control1, control2 ;
    
    if ( dx >= 0 ){
        control1 = start + QPointF(controlOffset, 0);
        control2 = end - QPointF(controlOffset, 0);
    } else {
        control1 = start - QPointF(controlOffset, 0);
        control2 = end + QPointF(controlOffset, 0);
    }

    path.cubicTo(control1, control2, end);
    return path;
}

QColor ConnectionCable::getCableColor() const
{
    if (!fromSocket_) return Qt::gray;
    
    switch (fromSocket_->getSpec().type) {
        case SocketType::ModulationInbound:
        case SocketType::ModulationOutbound:
            return Theme::CABLE_MODULATION ;
        case SocketType::SignalInbound:
        case SocketType::SignalOutbound:
            return Theme::CABLE_SIGNAL ;
        case SocketType::BufferInbound:
        case SocketType::BufferOutbound:
            return Theme::CABLE_BUFFER ;
        case SocketType::MidiInbound:
        case SocketType::MidiOutbound:
            return Theme::CABLE_MIDI ;
        default:
            return Qt::gray ;
    }
}

void ConnectionCable::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    painter->setRenderHint(QPainter::Antialiasing);
    
    // Draw a subtle shadow first
    QPen shadowPen = pen();
    shadowPen.setColor(Theme::CABLE_SHADOW);
    shadowPen.setWidth(pen().width() + 2);
    painter->setPen(shadowPen);
    painter->drawPath(path().translated(1, 1));
    
    // Draw the main cable
    QGraphicsPathItem::paint(painter, option, widget);
}

QPainterPath ConnectionCable::createAdaptiveBezierPath(const QPointF& start, const QPointF& end){
    QPainterPath path;
    path.moveTo(start);

    const QPointF start_dv = getSocketDirectionVector(getFromSocket());
    const QPointF end_dv   = -start_dv ; // always opposite side socket as start

    QPointF delta = end - start;
    qreal dist = QLineF(start, end).length();
    qreal forwardProgress = QPointF::dotProduct(start_dv, delta);
    qreal cycleStrength = std::min(std::max(0.0, -forwardProgress / dist), 1.0);

    qreal stemLength = std::max(
        Theme::CABLE_STEM_LENGTH_MAX, 
        dist * Theme::CABLE_STEM_LENGTH_FACTOR
    );

    // vertical doesn't need as long of a stem
    if ( start_dv.x() == 0.0 ){
        stemLength *= 0.5 ;
    }

    if ( cycleStrength > Theme::CABLE_CYCLE_THRESHOLD ){
        // two - segment curve for cycling connections
        QPointF flexDirection ;
        if ( std::abs(start_dv.x()) > std::abs(start_dv.y()) ){
            flexDirection = QPointF(0,1); // horizontal socket, bulge down
        } else {
            flexDirection = QPointF(1, 0); // vertical socket, bulge right
        }

        qreal flexStrength = std::max(
            Theme::CABLE_SIDE_BEND_MAX,
            dist * Theme::CABLE_SIDE_BEND_FACTOR
        );

        // define control points for both curves
        const QPointF cp1 = start + start_dv * stemLength ;
        const QPointF cp4 = end + end_dv * stemLength ;
        QPointF cp2, cp3, midpoint ;

        if ( start_dv.y() == 0 ){ // horizontal socket
            // set midpoint based on vertical distance
            if ( std::abs(end.y() - start.y()) > stemLength ){
                midpoint = (start + end) * 0.5 ; // s-curve
            } else {
                midpoint = (start + end) * 0.5 + flexDirection * flexStrength ; // flex out
            }
            // force horizontal midpoint tangent
            cp2 = QPointF(cp1.x(), midpoint.y());
            cp3 = QPointF(cp4.x(), midpoint.y());
        } else { // vertical socket
            // set midpoint based on horizontal distance
            if ( std::abs(end.x() - start.x()) > stemLength ){
                midpoint = (start + end) * 0.5 ; // s-curve
            } else {
                midpoint = (start + end) * 0.5 + flexDirection * flexStrength ;
            }
            // force vertical midpoint tangent
            cp2 = QPointF(midpoint.x(), cp1.y());
            cp3 = QPointF(midpoint.x(), cp4.y());
        }

        path.cubicTo(cp1, cp2, midpoint);
        path.cubicTo(cp3, cp4, end);
    } else {
        // normal single bezier curve
        qreal controlOffset = std::max(
            Theme::CABLE_SIDE_BEND_MAX, 
            std::abs(delta.x()) * Theme::CABLE_SIDE_BEND_FACTOR 
        );

        QPointF cp1, cp2 ;
        if ( delta.x() >= 0 ){
            cp1 = start + QPointF(controlOffset, 0);
            cp2 = end - QPointF(controlOffset, 0);
        } else {
            cp1 = start - QPointF(controlOffset, 0);
            cp2 = end + QPointF(controlOffset, 0);
        }

        path.cubicTo(cp1, cp2, end);
    }

    // add arrows to path
    size_t nArrows = path.length() / 100 ;
    for (int i = 0; i < nArrows ; ++i ){
        double perc = static_cast<double>(i) / nArrows ;
        if ( perc > 0 && perc < 100 ){
            drawCableArrow(path, perc);
        }
    }

    return path;
}

void ConnectionCable::drawCableArrow(QPainterPath& path, qreal atPercent){
    const qreal height = Theme::CABLE_ARROW_HEIGHT ;
    const qreal width = Theme::CABLE_ARROW_BASE_WIDTH ; 

    const qreal angle = path.angleAtPercent(atPercent) * M_PI / 180.0 ;
    const QPointF midpoint = path.pointAtPercent(atPercent);

    // arrow direction based on input vs output direction
    QPointF baseMid, tip ;
    const QPointF heightVector = QPointF(
        std::sin(angle + M_PI_2) * height / 2,
        std::cos(angle + M_PI_2) * height / 2  
    );
    if ( fromSocket_->isOutbound() ){
        tip = midpoint + heightVector ;
        baseMid = midpoint - heightVector ;
    } else {
        baseMid = midpoint + heightVector ;
        tip = midpoint - heightVector ;
    }

    QPointF dv = normalizePoint(heightVector);
    QPointF perp(-dv.y(), dv.x());

    const qreal width2 = width / 2 ;
    const QPointF base1 = baseMid + perp * width2 ;
    const QPointF base2 = baseMid - perp * width2 ;

    path.addPolygon(QPolygonF({tip, base1, base2, tip}));
}

QPointF ConnectionCable::getSocketDirectionVector(SocketWidget* socket){
    if ( ! socket ) return QPointF(0.0,0.0);
    
    switch(socket->getSpec().type){
        case SocketType::SignalInbound: // left
        case SocketType::MidiInbound: 
        case SocketType::BufferInbound:
            return QPointF(-1.0, 0);
        case SocketType::SignalOutbound: // right
        case SocketType::MidiOutbound:
        case SocketType::BufferOutbound:
            return QPointF(1.0, 0);
        case SocketType::ModulationInbound: // bottom
            return QPointF(0, 1.0);
        case SocketType::ModulationOutbound: // top
            return QPointF(0, -1.0);
        default:
            return QPointF(0.0,0.0);
    }
}

QPointF ConnectionCable::normalizePoint(const QPointF& p) const {
    qreal len = std::sqrt(p.x() * p.x() + p.y() * p.y() );
    return len > 0 ? QPointF(p.x() / len, p.y() / len) : QPointF(0,0);
}