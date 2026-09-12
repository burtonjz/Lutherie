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

#ifndef __GUI_SOCKET_WIDGET_HPP_
#define __GUI_SOCKET_WIDGET_HPP_

#include <QGraphicsObject>
#include <QWidget>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <nlohmann/json.hpp>

#include "util/SocketSpec.hpp"

using json = nlohmann::json ;

class GraphNode ; // forward declaration

class SocketWidget : public QGraphicsObject {
    Q_OBJECT

private:
    SocketSpec spec_ ;
    GraphNode* parent_ ;
    bool isHovered_ = false ;
    int nConnections_ = false ;
    QColor getSocketColor(bool isHovered) const ;

public:
    SocketWidget(SocketSpec spec, GraphNode* parent = nullptr);

    enum { Type = UserType + 2 };
    int type() const override { return Type; }

    // QGraphicsItem interface
    QRectF boundingRect() const override ;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override ;

    // Getters/Setters
    const SocketSpec& getSpec() const { return spec_ ; }
    GraphNode* getParent() const { return parent_ ; }
    bool isHovered() const ;
    void setHovered(bool hovered);

    bool isInbound() const ;
    bool isOutbound() const ;
    
    QPointF getConnectionPoint() const ;

    json serialize() const ;
    void deserialize(const json& msg);
};


#endif // __GUI_SOCKET_WIDGET_HPP_