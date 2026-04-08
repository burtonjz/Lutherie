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

#ifndef COMPONENT_NODE_HPP_
#define COMPONENT_NODE_HPP_

#include <QGraphicsObject>
#include <QPainter>
#include <QString>
#include <QStyleOptionGraphicsItem>
#include <qnamespace.h>

#include "models/ComponentModel.hpp"
#include "graphics/GraphNode.hpp"
#include "graphics/SocketWidget.hpp"

class ComponentNode :  public GraphNode {
    Q_OBJECT

private:
    ComponentModel* model_ ;
    std::vector<SocketSpec> specs_ ;

public:
    explicit ComponentNode(ComponentModel* model, const QString& name, QGraphicsItem* parent = nullptr);
    ~ComponentNode() = default ;

    ComponentModel* getModel() const ;
    const std::vector<SocketSpec>& getSpecs() const ;

    json serialize() const override ;
    virtual void deserialize(const json& node) override ;

    void requestRename(const QString& name ) override ;

signals:
    void requestComponentRename(int componentId, QString name);
};

#endif // COMPONENT_NODE_HPP_