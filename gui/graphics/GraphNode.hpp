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

#ifndef GRAPH_NODE_HPP_
#define GRAPH_NODE_HPP_

#include <QGraphicsObject>
#include <QPainter>
#include <QString>
#include <QStyleOptionGraphicsItem>
#include <vector>
#include <nlohmann/json.hpp>

#include "graphics/SocketWidget.hpp"
#include "types/ConnectionEndpoint.hpp"
#include "util/SocketSpec.hpp"

using json = nlohmann::json ;

class GraphNode :  public QGraphicsObject {
    Q_OBJECT

private:
    SocketPriority socketPriority_ ;
    bool isDragging_ = false ;
    QPointF dragStartPos_ ;
    QString name_ ;
    qreal height_ ;

    std::vector<SocketWidget*> leftSockets_ ; // audio / midi inputs
    std::vector<SocketWidget*> rightSockets_ ; // audio / midi outputs
    std::vector<SocketWidget*> bottomSockets_ ; // modulatable inputs
    std::vector<SocketWidget*> topSockets_ ; // modulatable outputs

protected:
    std::vector<SocketWidget*> sockets_ ; 
    std::set<SocketWidget*> native_ ;
    std::set<SocketWidget*> derived_ ;
    std::map<SocketType, std::vector<SocketWidget*>> socketsByType_ ;

    QGraphicsTextItem* titleText_ ;

public:
    explicit GraphNode(
        QString name, 
        SocketPriority priority, 
        QGraphicsItem* parent = nullptr
    );
    virtual ~GraphNode();

    enum { Type = UserType + 1 };
    int type() const override { return Type; }
    
    QRectF boundingRect() const override ;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr ) override ;

    const std::vector<SocketWidget*>& getSockets() const ;

    SocketWidget* getSocketFromSpec(const SocketSpec& spec) const ;

    const QString& getName() const { return name_ ; }
    QGraphicsTextItem* getNameItem() const { return titleText_ ; }

    // for sockets considered "native" to the node
    SocketWidget* insertSocket(const SocketSpec& spec);
    void insertSockets(const std::vector<SocketSpec>& specs );

    // for groups derived from native sockets
    SocketWidget* createGroupSocketFromSpec(const SocketSpec& spec);
    SocketWidget* createGroupSocket(const std::vector<SocketWidget*>& sockets);
    void createAllValidGroupSockets();

    void removeSocket(SocketSpec spec);
    void removeSockets(const std::vector<SocketSpec>& specs);

    bool hasDerivedSockets() const ;
    void removeDerivedSockets();

    void hide();
    void show();
    
    virtual json serialize() const ;
    virtual void deserialize(const json& node);

protected:
    // Graphics overrides
    QVariant itemChange(GraphicsItemChange change, const QVariant& value ) override ; // for tracking module position changes

    bool validateGroupSocketSpec(const SocketSpec& spec) const ;
    SocketWidget* createSocket(const SocketSpec& spec, std::optional<SocketPriority> priority = std::nullopt );

    void layoutSockets();
    void reorderSockets();
    void positionSockets(QPointF newPos); 

public slots:
    void onRename(QString name);
    void removeSockets();
    void removeSocket(SocketWidget* socket);
    void socketClaimStatusChanged();

signals:
    void needsZUpdate();
    void socketPositionChanged(SocketWidget* socket);
    void socketVisibilityChanged(SocketWidget* socket);
    
};

#endif // GRAPH_NODE_HPP_