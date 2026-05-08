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
#include <initializer_list>
#include <vector>
#include <nlohmann/json.hpp>

#include "graphics/SocketWidget.hpp"

using json = nlohmann::json ;

class GraphNode :  public QGraphicsObject {
    Q_OBJECT

private:
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
    QGraphicsTextItem* titleText_ ;

public:
    explicit GraphNode(QString name, QGraphicsItem* parent = nullptr);
    virtual ~GraphNode();

    enum { Type = UserType + 1 };
    int type() const override { return Type; }
    
    QRectF boundingRect() const override ;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr ) override ;

    SocketWidget* getSocket(SocketSpec s) const ;
    const std::vector<SocketWidget*>& getSockets() const { return sockets_ ; }
    const QString& getName() const { return name_ ; }
    QGraphicsTextItem* getNameItem() const { return titleText_ ; }

    virtual void requestRename(const QString& name );

    void createSockets(std::initializer_list<SocketSpec> specs );
    void createSockets(const std::vector<SocketSpec> specs );
    void clearSockets();

    void hide();
    void show();
    void addToScene(QGraphicsScene* scene);

    std::vector<SocketWidget*> getHiddenSockets() const ;
    void unhideSocket(SocketWidget* socket);
    void unhideAllSockets();
    void hideSocket(SocketWidget* socket);
    void hideDisconnectedSockets();

    virtual json serialize() const ;
    virtual void deserialize(const json& node);

protected:
    // Graphics overrides
    QVariant itemChange(GraphicsItemChange change, const QVariant& value ) override ; // for tracking module position changes

    void layoutSockets();
    void reorderSockets();
    void positionSockets(QPointF newPos); 

public slots:
    void onRename(QString name);

signals:
    void positionChanged();
    void needsZUpdate();
    void socketHidden(SocketWidget* socket);
    void socketUnhidden(SocketWidget* socket);

};

#endif // GRAPH_NODE_HPP_