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
#include <optional>
#include <nlohmann/json.hpp>

#include "types/SocketType.hpp"

using json = nlohmann::json ;

class GraphNode ; // forward declaration

struct SocketSpec {
    SocketType type ;
    QString name ;
    std::optional<int> componentId ;
    std::optional<size_t> idx ;

    bool operator==(const SocketSpec& other){
        return other.type == type &&
               other.componentId == componentId &&
               other.idx == idx
            ;
    }
};

inline void to_json(json& j, const SocketSpec& spec){
    j["type"] = spec.type ;
    j["name"] = spec.name.toStdString();
    if ( spec.componentId.has_value() ){
        j["componentId"] = spec.componentId.value();
    }
    if ( spec.idx.has_value() ){
        j["index"] = spec.idx.value();
    }
}

inline void from_json(const json& j, SocketSpec& spec){
    spec.type = static_cast<SocketType>(j.at("type"));
    spec.name = QString::fromStdString(j.at("name")) ;
    if ( j.contains("componentId") ){
        spec.componentId = j.at("componentId") ;
    }
    if ( j.contains("index") ){
        spec.idx = j.at("index");
    }
}

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

    bool hasConnection() const ;
    void setConnnection(bool newConnection);
    void syncConnection(SocketWidget* other);
    
    QPointF getConnectionPoint() const ;

    bool matches(SocketSpec spec) const ;

};

#endif // __GUI_SOCKET_WIDGET_HPP_