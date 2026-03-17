/*
 * Copyright (C) 2026 Jared Burton
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

#ifndef PERIPHERAL_NODE_HPP_
#define PERIPHERAL_NODE_HPP_

#include "graphics/GraphNode.hpp"

#include <QObject>

class PeripheralNode : public GraphNode {
    Q_OBJECT

private:
    int deviceId_ ;
    
public:
    explicit PeripheralNode(int deviceId, const QString& name, QGraphicsItem* parent = nullptr);

    int getId() const ;

    json serialize() const override ;
    virtual void deserialize(const json& node) override ;
    
};

#endif // PERIPHERAL_NODE_HPP_